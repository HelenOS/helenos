/*
 * Copyright (c) 2011 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>
#include "vhcd.h"
#include "hub/virthub.h"

static bool is_set_address_transfer(vhc_transfer_t *transfer)
{
	if (transfer->batch->ep->endpoint != 0) {
		return false;
	}
	if (transfer->batch->ep->transfer_type != USB_TRANSFER_CONTROL) {
		return false;
	}
	if (usb_transfer_batch_direction(transfer->batch) != USB_DIRECTION_OUT) {
		return false;
	}
	const usb_device_request_setup_packet_t *setup =
	    (void*)transfer->batch->setup_buffer;
	if (setup->request_type != 0) {
		return false;
	}
	if (setup->request != USB_DEVREQ_SET_ADDRESS) {
		return false;
	}

	return true;
}

static errno_t process_transfer_local(usb_transfer_batch_t *batch,
    usbvirt_device_t *dev, size_t *actual_data_size)
{
	errno_t rc;
	
	const usb_direction_t dir = usb_transfer_batch_direction(batch);

	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_control_read(dev,
			    batch->setup_buffer, batch->setup_size,
			    batch->buffer, batch->buffer_size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_control_write(dev,
			    batch->setup_buffer, batch->setup_size,
			    batch->buffer, batch->buffer_size);
		}
	} else {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_data_in(dev, batch->ep->transfer_type,
			    batch->ep->endpoint,
			    batch->buffer, batch->buffer_size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_data_out(dev, batch->ep->transfer_type,
			    batch->ep->endpoint,
			    batch->buffer, batch->buffer_size);
		}
	}

	return rc;
}

static errno_t process_transfer_remote(usb_transfer_batch_t *batch,
    async_sess_t *sess, size_t *actual_data_size)
{
	errno_t rc;

	const usb_direction_t dir = usb_transfer_batch_direction(batch);

	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_control_read(sess,
			    batch->setup_buffer, batch->setup_size,
			    batch->buffer, batch->buffer_size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_control_write(sess,
			    batch->setup_buffer, batch->setup_size,
			    batch->buffer, batch->buffer_size);
		}
	} else {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_data_in(sess, batch->ep->endpoint,
			    batch->ep->transfer_type,
			    batch->buffer, batch->buffer_size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_data_out(sess, batch->ep->endpoint,
			    batch->ep->transfer_type,
			    batch->buffer, batch->buffer_size);
		}
	}

	return rc;
}

static vhc_transfer_t *dequeue_first_transfer(vhc_virtdev_t *dev)
{
	assert(fibril_mutex_is_locked(&dev->guard));
	assert(!list_empty(&dev->transfer_queue));

	vhc_transfer_t *transfer = list_get_instance(
	    list_first(&dev->transfer_queue), vhc_transfer_t, link);
	list_remove(&transfer->link);

	return transfer;
}

static void execute_transfer_callback_and_free(vhc_transfer_t *transfer,
    size_t data_transfer_size, errno_t outcome)
{
	assert(outcome != ENAK);
	assert(transfer);
	assert(transfer->batch);
	usb_transfer_batch_finish_error(transfer->batch, NULL,
	    data_transfer_size, outcome);
	usb_transfer_batch_destroy(transfer->batch);
	free(transfer);
}

errno_t vhc_init(vhc_data_t *instance)
{
	assert(instance);
	list_initialize(&instance->devices);
	fibril_mutex_initialize(&instance->guard);
	instance->magic = 0xDEADBEEF;
	return virthub_init(&instance->hub, "root hub");
}

errno_t vhc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	assert(hcd);
	assert(batch);
	vhc_data_t *vhc = hcd_get_driver_data(hcd);
	assert(vhc);

	vhc_transfer_t *transfer = malloc(sizeof(vhc_transfer_t));
	if (!transfer)
		return ENOMEM;
	link_initialize(&transfer->link);
	transfer->batch = batch;

	fibril_mutex_lock(&vhc->guard);

	int targets = 0;

	list_foreach(vhc->devices, link, vhc_virtdev_t, dev) {
		fibril_mutex_lock(&dev->guard);
		if (dev->address == transfer->batch->ep->address) {
			if (!targets) {
				list_append(&transfer->link, &dev->transfer_queue);
			}
			++targets;
		}
		fibril_mutex_unlock(&dev->guard);
	}

	fibril_mutex_unlock(&vhc->guard);
	
	if (targets > 1)
		usb_log_warning("Transfer would be accepted by more devices!\n");

	return targets ? EOK : ENOENT;
}

errno_t vhc_transfer_queue_processor(void *arg)
{
	vhc_virtdev_t *dev = arg;
	fibril_mutex_lock(&dev->guard);
	while (dev->plugged) {
		if (list_empty(&dev->transfer_queue)) {
			fibril_mutex_unlock(&dev->guard);
			async_usleep(10 * 1000);
			fibril_mutex_lock(&dev->guard);
			continue;
		}

		vhc_transfer_t *transfer = dequeue_first_transfer(dev);
		fibril_mutex_unlock(&dev->guard);

		errno_t rc = EOK;
		size_t data_transfer_size = 0;
		if (dev->dev_sess) {
			rc = process_transfer_remote(transfer->batch,
			    dev->dev_sess, &data_transfer_size);
		} else if (dev->dev_local != NULL) {
			rc = process_transfer_local(transfer->batch,
			    dev->dev_local, &data_transfer_size);
		} else {
			usb_log_warning("Device has no remote phone nor local node.\n");
			rc = ESTALL;
		}

		usb_log_debug2("Transfer %p processed: %s.\n",
		    transfer, str_error(rc));

		fibril_mutex_lock(&dev->guard);
		if (rc == EOK) {
			if (is_set_address_transfer(transfer)) {
				usb_device_request_setup_packet_t *setup =
				    (void*) transfer->batch->setup_buffer;
				dev->address = setup->value;
				usb_log_debug2("Address changed to %d\n",
				    dev->address);
			}
		}
		if (rc == ENAK) {
			// FIXME: this will work only because we do
			// not NAK control transfers but this is generally
			// a VERY bad idea indeed
			list_append(&transfer->link, &dev->transfer_queue);
		}
		fibril_mutex_unlock(&dev->guard);

		if (rc != ENAK) {
			execute_transfer_callback_and_free(transfer,
			    data_transfer_size, rc);
		}

		async_usleep(1000 * 100);
		fibril_mutex_lock(&dev->guard);
	}

	/* Immediately fail all remaining transfers. */
	while (!list_empty(&dev->transfer_queue)) {
		vhc_transfer_t *transfer = dequeue_first_transfer(dev);
		execute_transfer_callback_and_free(transfer, 0, EBADCHECKSUM);
	}

	fibril_mutex_unlock(&dev->guard);

	return EOK;
}
