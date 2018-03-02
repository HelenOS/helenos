/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2018 Ondrej Hlavaty
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
#include <usb/host/bandwidth.h>
#include <usb/host/endpoint.h>
#include <usb/host/usb_transfer_batch.h>
#include <usbvirt/ipc.h>
#include "vhcd.h"
#include "hub/virthub.h"

static bool is_set_address_transfer(vhc_transfer_t *transfer)
{
	if (transfer->batch.target.endpoint != 0) {
		return false;
	}
	if (transfer->batch.ep->transfer_type != USB_TRANSFER_CONTROL) {
		return false;
	}
	if (transfer->batch.dir != USB_DIRECTION_OUT) {
		return false;
	}
	const usb_device_request_setup_packet_t *setup =
	    &transfer->batch.setup.packet;
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

	const usb_direction_t dir = batch->dir;

	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_control_read(dev,
			    batch->setup.buffer, USB_SETUP_PACKET_SIZE,
			    batch->dma_buffer.virt, batch->size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_control_write(dev,
			    batch->setup.buffer, USB_SETUP_PACKET_SIZE,
			    batch->dma_buffer.virt, batch->size);
		}
	} else {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_data_in(dev, batch->ep->transfer_type,
			    batch->ep->endpoint,
			    batch->dma_buffer.virt, batch->size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_data_out(dev, batch->ep->transfer_type,
			    batch->ep->endpoint,
			    batch->dma_buffer.virt, batch->size);
		}
	}

	return rc;
}

static errno_t process_transfer_remote(usb_transfer_batch_t *batch,
    async_sess_t *sess, size_t *actual_data_size)
{
	errno_t rc;

	const usb_direction_t dir = batch->dir;

	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_control_read(sess,
			    batch->setup.buffer, USB_SETUP_PACKET_SIZE,
			    batch->dma_buffer.virt, batch->size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_control_write(sess,
			    batch->setup.buffer, USB_SETUP_PACKET_SIZE,
			    batch->dma_buffer.virt, batch->size);
		}
	} else {
		if (dir == USB_DIRECTION_IN) {
			rc = usbvirt_ipc_send_data_in(sess, batch->ep->endpoint,
			    batch->ep->transfer_type,
			    batch->dma_buffer.virt, batch->size,
			    actual_data_size);
		} else {
			assert(dir == USB_DIRECTION_OUT);
			rc = usbvirt_ipc_send_data_out(sess, batch->ep->endpoint,
			    batch->ep->transfer_type,
			    batch->dma_buffer.virt, batch->size);
		}
	}

	return rc;
}

static vhc_transfer_t *dequeue_first_transfer(vhc_virtdev_t *dev)
{
	assert(fibril_mutex_is_locked(&dev->guard));
	assert(!list_empty(&dev->transfer_queue));

	vhc_transfer_t *transfer =
	    list_get_instance(list_first(&dev->transfer_queue),
	    vhc_transfer_t, link);
	list_remove(&transfer->link);

	return transfer;
}

static void execute_transfer_callback_and_free(vhc_transfer_t *transfer,
    size_t data_transfer_size, errno_t outcome)
{
	assert(outcome != ENAK);
	assert(transfer);
	transfer->batch.error = outcome;
	transfer->batch.transferred_size = data_transfer_size;
	usb_transfer_batch_finish(&transfer->batch);
}

static usb_transfer_batch_t *batch_create(endpoint_t *ep)
{
	vhc_transfer_t *transfer = calloc(1, sizeof(vhc_transfer_t));
	usb_transfer_batch_init(&transfer->batch, ep);
	link_initialize(&transfer->link);
	return &transfer->batch;
}

static int device_enumerate(device_t *device)
{
	vhc_data_t *vhc = bus_to_vhc(device->bus);
	return usb2_bus_device_enumerate(&vhc->bus_helper, device);
}

static int endpoint_register(endpoint_t *endpoint)
{
	vhc_data_t *vhc = bus_to_vhc(endpoint->device->bus);
	return usb2_bus_endpoint_register(&vhc->bus_helper, endpoint);
}

static void endpoint_unregister(endpoint_t *endpoint)
{
	vhc_data_t *vhc = bus_to_vhc(endpoint->device->bus);
	usb2_bus_endpoint_unregister(&vhc->bus_helper, endpoint);

	// TODO: abort transfer?
}

static const bus_ops_t vhc_bus_ops = {
	.batch_create = batch_create,
	.batch_schedule = vhc_schedule,

	.device_enumerate = device_enumerate,
	.endpoint_register = endpoint_register,
	.endpoint_unregister = endpoint_unregister,
};

errno_t vhc_init(vhc_data_t *instance)
{
	assert(instance);
	list_initialize(&instance->devices);
	fibril_mutex_initialize(&instance->guard);
	bus_init(&instance->bus, sizeof(device_t));
	usb2_bus_helper_init(&instance->bus_helper, &bandwidth_accounting_usb11);
	instance->bus.ops = &vhc_bus_ops;
	return virthub_init(&instance->hub, "root hub");
}

errno_t vhc_schedule(usb_transfer_batch_t *batch)
{
	assert(batch);
	vhc_transfer_t *transfer = (vhc_transfer_t *) batch;
	vhc_data_t *vhc = bus_to_vhc(endpoint_get_bus(batch->ep));
	assert(vhc);

	fibril_mutex_lock(&vhc->guard);

	int targets = 0;

	list_foreach(vhc->devices, link, vhc_virtdev_t, dev) {
		fibril_mutex_lock(&dev->guard);
		if (dev->address == transfer->batch.target.address) {
			if (!targets) {
				list_append(&transfer->link, &dev->transfer_queue);
			}
			++targets;
		}
		fibril_mutex_unlock(&dev->guard);
	}

	fibril_mutex_unlock(&vhc->guard);

	if (targets > 1)
		usb_log_warning("Transfer would be accepted by more devices!");

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
			rc = process_transfer_remote(&transfer->batch,
			    dev->dev_sess, &data_transfer_size);
		} else if (dev->dev_local != NULL) {
			rc = process_transfer_local(&transfer->batch,
			    dev->dev_local, &data_transfer_size);
		} else {
			usb_log_warning("Device has no remote phone "
			    "nor local node.");
			rc = ESTALL;
		}

		usb_log_debug2("Transfer %p processed: %s.",
		    transfer, str_error(rc));

		fibril_mutex_lock(&dev->guard);
		if (rc == EOK) {
			if (is_set_address_transfer(transfer)) {
				usb_device_request_setup_packet_t *setup =
				    (void *) transfer->batch.setup.buffer;
				dev->address = setup->value;
				usb_log_debug2("Address changed to %d",
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
