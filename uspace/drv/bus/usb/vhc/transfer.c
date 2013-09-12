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
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>
#include "vhcd.h"

vhc_transfer_t *vhc_transfer_create(usb_address_t address, usb_endpoint_t ep,
    usb_direction_t dir, usb_transfer_type_t tr_type,
    ddf_fun_t *fun, void *callback_arg)
{
	vhc_transfer_t *result = malloc(sizeof(vhc_transfer_t));
	if (result == NULL) {
		return NULL;
	}
	link_initialize(&result->link);
	result->address = address;
	result->endpoint = ep;
	result->direction = dir;
	result->transfer_type = tr_type;
	result->setup_buffer = NULL;
	result->setup_buffer_size = 0;
	result->data_buffer = NULL;
	result->data_buffer_size = 0;
	result->ddf_fun = fun;
	result->callback_arg = callback_arg;
	result->callback_in = NULL;
	result->callback_out = NULL;

	usb_log_debug2("Created transfer %p (%d.%d %s %s)\n", result,
	    address, ep, usb_str_transfer_type_short(tr_type),
	    dir == USB_DIRECTION_IN ? "in" : "out");

	return result;
}

static bool is_set_address_transfer(vhc_transfer_t *transfer)
{
	if (transfer->endpoint != 0) {
		return false;
	}
	if (transfer->transfer_type != USB_TRANSFER_CONTROL) {
		return false;
	}
	if (transfer->direction != USB_DIRECTION_OUT) {
		return false;
	}
	if (transfer->setup_buffer_size != sizeof(usb_device_request_setup_packet_t)) {
		return false;
	}
	usb_device_request_setup_packet_t *setup = transfer->setup_buffer;
	if (setup->request_type != 0) {
		return false;
	}
	if (setup->request != USB_DEVREQ_SET_ADDRESS) {
		return false;
	}

	return true;
}

int vhc_virtdev_add_transfer(vhc_data_t *vhc, vhc_transfer_t *transfer)
{
	fibril_mutex_lock(&vhc->guard);

	bool target_found = false;
	list_foreach(vhc->devices, link, vhc_virtdev_t, dev) {
		fibril_mutex_lock(&dev->guard);
		if (dev->address == transfer->address) {
			if (target_found) {
				usb_log_warning("Transfer would be accepted by more devices!\n");
				goto next;
			}
			target_found = true;
			list_append(&transfer->link, &dev->transfer_queue);
		}
next:
		fibril_mutex_unlock(&dev->guard);
	}

	fibril_mutex_unlock(&vhc->guard);

	if (target_found) {
		return EOK;
	} else {
		return ENOENT;
	}
}

static int process_transfer_local(vhc_transfer_t *transfer,
    usbvirt_device_t *dev, size_t *actual_data_size)
{
	int rc;

	if (transfer->transfer_type == USB_TRANSFER_CONTROL) {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_control_read(dev,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_control_write(dev,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	} else {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_data_in(dev, transfer->transfer_type,
			    transfer->endpoint,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_data_out(dev, transfer->transfer_type,
			    transfer->endpoint,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	}

	return rc;
}

static int process_transfer_remote(vhc_transfer_t *transfer,
    async_sess_t *sess, size_t *actual_data_size)
{
	int rc;

	if (transfer->transfer_type == USB_TRANSFER_CONTROL) {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_control_read(sess,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_control_write(sess,
			    transfer->setup_buffer, transfer->setup_buffer_size,
			    transfer->data_buffer, transfer->data_buffer_size);
		}
	} else {
		if (transfer->direction == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_data_in(sess, transfer->endpoint,
			    transfer->transfer_type,
			    transfer->data_buffer, transfer->data_buffer_size,
			    actual_data_size);
		} else {
			assert(transfer->direction == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_data_out(sess, transfer->endpoint,
			    transfer->transfer_type,
			    transfer->data_buffer, transfer->data_buffer_size);
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
    size_t data_transfer_size, int outcome)
{
	assert(outcome != ENAK);

	usb_log_debug2("Transfer %p ended: %s.\n",
	    transfer, str_error(outcome));

	if (transfer->direction == USB_DIRECTION_IN) {
		transfer->callback_in(transfer->ddf_fun, outcome,
		    data_transfer_size, transfer->callback_arg);
	} else {
		assert(transfer->direction == USB_DIRECTION_OUT);
		transfer->callback_out(transfer->ddf_fun, outcome,
		    transfer->callback_arg);
	}

	free(transfer);
}

int vhc_transfer_queue_processor(void *arg)
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

		int rc = EOK;
		size_t data_transfer_size = 0;
		if (dev->dev_sess) {
			rc = process_transfer_remote(transfer, dev->dev_sess,
			    &data_transfer_size);
		} else if (dev->dev_local != NULL) {
			rc = process_transfer_local(transfer, dev->dev_local,
			    &data_transfer_size);
		} else {
			usb_log_warning("Device has no remote phone nor local node.\n");
			rc = ESTALL;
		}

		usb_log_debug2("Transfer %p processed: %s.\n",
		    transfer, str_error(rc));

		fibril_mutex_lock(&dev->guard);
		if (rc == EOK) {
			if (is_set_address_transfer(transfer)) {
				usb_device_request_setup_packet_t *setup
				    = transfer->setup_buffer;
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

