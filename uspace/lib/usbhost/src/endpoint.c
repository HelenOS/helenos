/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 * @brief UHCI host controller driver structure
 */

#include <assert.h>
#include <atomic.h>
#include <mem.h>
#include <stdlib.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/host/hcd.h>
#include <usb/host/utility.h>

#include "usb_transfer_batch.h"
#include "bus.h"

#include "endpoint.h"

/**
 * Initialize provided endpoint structure.
 */
void endpoint_init(endpoint_t *ep, device_t *dev, const usb_endpoint_descriptors_t *desc)
{
	memset(ep, 0, sizeof(endpoint_t));

	assert(dev);
	ep->device = dev;

	atomic_set(&ep->refcnt, 0);
	fibril_mutex_initialize(&ep->guard);
	fibril_condvar_initialize(&ep->avail);

	ep->endpoint = USB_ED_GET_EP(desc->endpoint);
	ep->direction = USB_ED_GET_DIR(desc->endpoint);
	ep->transfer_type = USB_ED_GET_TRANSFER_TYPE(desc->endpoint);
	ep->max_packet_size = USB_ED_GET_MPS(desc->endpoint);
	ep->packets_per_uframe = USB_ED_GET_ADD_OPPS(desc->endpoint) + 1;

	/** Direction both is our construct never present in descriptors */
	if (ep->transfer_type == USB_TRANSFER_CONTROL)
		ep->direction = USB_DIRECTION_BOTH;

	ep->max_transfer_size = ep->max_packet_size * ep->packets_per_uframe;

	ep->bandwidth = endpoint_count_bw(ep, ep->max_transfer_size);
}

/**
 * Get the bus endpoint belongs to.
 */
static inline const bus_ops_t *get_bus_ops(endpoint_t *ep)
{
	return ep->device->bus->ops;
}

/**
 * Increase the reference count on endpoint.
 */
void endpoint_add_ref(endpoint_t *ep)
{
	atomic_inc(&ep->refcnt);
}

/**
 * Call the desctruction callback. Default behavior is to free the memory directly.
 */
static inline void endpoint_destroy(endpoint_t *ep)
{
	const bus_ops_t *ops = BUS_OPS_LOOKUP(get_bus_ops(ep), endpoint_destroy);
	if (ops) {
		ops->endpoint_destroy(ep);
	} else {
		assert(ep->active_batch == NULL);

		/* Assume mostly the eps will be allocated by malloc. */
		free(ep);
	}
}

/**
 * Decrease the reference count.
 */
void endpoint_del_ref(endpoint_t *ep)
{
	if (atomic_predec(&ep->refcnt) == 0) {
		endpoint_destroy(ep);
	}
}

/**
 * Wait until the endpoint have no transfer scheduled.
 */
void endpoint_wait_timeout_locked(endpoint_t *ep, suseconds_t timeout)
{
	assert(fibril_mutex_is_locked(&ep->guard));

	if (ep->active_batch != NULL)
		fibril_condvar_wait_timeout(&ep->avail, &ep->guard, timeout);

	while (timeout == 0 && ep->active_batch != NULL)
		fibril_condvar_wait_timeout(&ep->avail, &ep->guard, timeout);
}

/**
 * Mark the endpoint as active and block access for further fibrils. If the
 * endpoint is already active, it will block on ep->avail condvar.
 *
 * Call only under endpoint guard. After you activate the endpoint and release
 * the guard, you must assume that particular transfer is already finished/aborted.
 *
 * @param ep endpoint_t structure.
 * @param batch Transfer batch this endpoint is bocked by.
 */
void endpoint_activate_locked(endpoint_t *ep, usb_transfer_batch_t *batch)
{
	assert(ep);
	assert(batch);
	assert(batch->ep == ep);

	endpoint_wait_timeout_locked(ep, 0);
	ep->active_batch = batch;
}

/**
 * Mark the endpoint as inactive and allow access for further fibrils.
 *
 * @param ep endpoint_t structure.
 */
void endpoint_deactivate_locked(endpoint_t *ep)
{
	assert(ep);
	assert(fibril_mutex_is_locked(&ep->guard));
	ep->active_batch = NULL;
	fibril_condvar_signal(&ep->avail);
}

/**
 * Call the bus operation to count bandwidth.
 *
 * @param ep Endpoint on which the transfer will take place.
 * @param size The payload size.
 */
ssize_t endpoint_count_bw(endpoint_t *ep, size_t size)
{
	assert(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(get_bus_ops(ep), endpoint_count_bw);
	if (!ops)
		return 0;

	return ops->endpoint_count_bw(ep, size);
}

/**
 * Initiate a transfer on an endpoint. Creates a transfer batch, checks the
 * bandwidth requirements and schedules the batch.
 *
 * @param endpoint Endpoint for which to send the batch
 * @param target The target of the transfer.
 * @param direction A direction of the transfer.
 * @param data A pointer to the data buffer.
 * @param size Size of the data buffer.
 * @param setup_data Data to use in the setup stage (Control communication type)
 * @param on_complete Callback which is called after the batch is complete
 * @param arg Callback parameter.
 * @param name Communication identifier (for nicer output).
 */
int endpoint_send_batch(endpoint_t *ep, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_callback_t on_complete, void *arg, const char *name)
{
	if (!ep)
		return EBADMEM;

	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		usb_log_debug("%s %d:%d %zu/%zuB, setup %#016" PRIx64, name,
		    target.address, target.endpoint, size, ep->max_packet_size,
		    setup_data);
	} else {
		usb_log_debug("%s %d:%d %zu/%zuB", name, target.address,
		    target.endpoint, size, ep->max_packet_size);
	}

	device_t * const device = ep->device;
	if (!device) {
		usb_log_warning("Endpoint detached");
		return EAGAIN;
	}

	const bus_ops_t *ops = BUS_OPS_LOOKUP(device->bus->ops, batch_schedule);
	if (!ops) {
		usb_log_error("HCD does not implement scheduler.");
		return ENOTSUP;
	}

	/* Offline devices don't schedule transfers other than on EP0. */
	if (!device->online && ep->endpoint > 0) {
		return EAGAIN;
	}

	const size_t bw = endpoint_count_bw(ep, size);
	/* Check if we have enough bandwidth reserved */
	if (ep->bandwidth < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    device->address, ep->endpoint, name, bw, ep->bandwidth);
		return ENOSPC;
	}

	usb_transfer_batch_t *batch = usb_transfer_batch_create(ep);
	if (!batch) {
		usb_log_error("Failed to create transfer batch.");
		return ENOMEM;
	}

	batch->target = target;
	batch->buffer = data;
	batch->buffer_size = size;
	batch->setup.packed = setup_data;
	batch->dir = direction;
	batch->on_complete = on_complete;
	batch->on_complete_data = arg;

	const int ret = ops->batch_schedule(batch);
	if (ret != EOK) {
		usb_log_warning("Batch %p failed to schedule: %s", batch, str_error(ret));
		usb_transfer_batch_destroy(batch);
	}

	return ret;
}

/**
 * @}
 */
