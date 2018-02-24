/*
 * Copyright (c) 2011 Jan Vesely
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
	ep->transfer_buffer_policy = DMA_POLICY_STRICT;
	ep->required_transfer_buffer_policy = DMA_POLICY_STRICT;
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
	const bus_ops_t *ops = get_bus_ops(ep);
	if (ops->endpoint_destroy) {
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
 * Mark the endpoint as online. Supply a guard to be used for this endpoint
 * synchronization.
 */
void endpoint_set_online(endpoint_t *ep, fibril_mutex_t *guard)
{
	ep->guard = guard;
	ep->online = true;
}

/**
 * Mark the endpoint as offline. All other fibrils waiting to activate this
 * endpoint will be interrupted.
 */
void endpoint_set_offline_locked(endpoint_t *ep)
{
	assert(ep);
	assert(fibril_mutex_is_locked(ep->guard));

	ep->online = false;
	fibril_condvar_broadcast(&ep->avail);
}

/**
 * Wait until a transfer finishes. Can be used even when the endpoint is
 * offline (and is interrupted by the endpoint going offline).
 */
void endpoint_wait_timeout_locked(endpoint_t *ep, suseconds_t timeout)
{
	assert(ep);
	assert(fibril_mutex_is_locked(ep->guard));

	if (ep->active_batch == NULL)
		return;

	fibril_condvar_wait_timeout(&ep->avail, ep->guard, timeout);
}

/**
 * Mark the endpoint as active and block access for further fibrils. If the
 * endpoint is already active, it will block on ep->avail condvar.
 *
 * Call only under endpoint guard. After you activate the endpoint and release
 * the guard, you must assume that particular transfer is already
 * finished/aborted.
 *
 * Activation and deactivation is not done by the library to maximize
 * performance. The HC might want to prepare some memory buffers prior to
 * interfering with other world.
 *
 * @param batch Transfer batch this endpoint is blocked by.
 */
int endpoint_activate_locked(endpoint_t *ep, usb_transfer_batch_t *batch)
{
	assert(ep);
	assert(batch);
	assert(batch->ep == ep);
	assert(ep->guard);
	assert(fibril_mutex_is_locked(ep->guard));

	while (ep->online && ep->active_batch != NULL)
		fibril_condvar_wait(&ep->avail, ep->guard);

	if (!ep->online)
		return EINTR;

	assert(ep->active_batch == NULL);
	ep->active_batch = batch;
	return EOK;
}

/**
 * Mark the endpoint as inactive and allow access for further fibrils.
 */
void endpoint_deactivate_locked(endpoint_t *ep)
{
	assert(ep);
	assert(fibril_mutex_is_locked(ep->guard));

	ep->active_batch = NULL;
	fibril_condvar_signal(&ep->avail);
}

/**
 * Initiate a transfer on an endpoint. Creates a transfer batch, checks the
 * bandwidth requirements and schedules the batch.
 *
 * @param endpoint Endpoint for which to send the batch
 */
errno_t endpoint_send_batch(endpoint_t *ep, const transfer_request_t *req)
{
	assert(ep);
	assert(req);

	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		usb_log_debug("%s %d:%d %zu/%zuB, setup %#016" PRIx64, req->name,
		    req->target.address, req->target.endpoint,
		    req->size, ep->max_packet_size,
		    req->setup);
	} else {
		usb_log_debug("%s %d:%d %zu/%zuB", req->name,
		    req->target.address, req->target.endpoint,
		    req->size, ep->max_packet_size);
	}

	device_t * const device = ep->device;
	if (!device) {
		usb_log_warning("Endpoint detached");
		return EAGAIN;
	}

	const bus_ops_t *ops = device->bus->ops;
	if (!ops->batch_schedule) {
		usb_log_error("HCD does not implement scheduler.");
		return ENOTSUP;
	}

	size_t size = req->size;
	/*
	 * Limit transfers with reserved bandwidth to the amount reserved.
	 * OUT transfers are rejected, IN can be just trimmed in advance.
	 */
	if (size > ep->max_transfer_size &&
	    (ep->transfer_type == USB_TRANSFER_INTERRUPT
	     || ep->transfer_type == USB_TRANSFER_ISOCHRONOUS)) {
		if (req->dir == USB_DIRECTION_OUT)
			return ENOSPC;
		else
			size = ep->max_transfer_size;
	}

	/* Offline devices don't schedule transfers other than on EP0. */
	if (!device->online && ep->endpoint > 0)
		return EAGAIN;

	usb_transfer_batch_t *batch = usb_transfer_batch_create(ep);
	if (!batch) {
		usb_log_error("Failed to create transfer batch.");
		return ENOMEM;
	}

	batch->target = req->target;
	batch->setup.packed = req->setup;
	batch->dir = req->dir;
	batch->size = size;
	batch->offset = req->offset;
	batch->dma_buffer = req->buffer;

	dma_buffer_acquire(&batch->dma_buffer);

	if (batch->offset != 0) {
		usb_log_debug("A transfer with nonzero offset requested.");
		usb_transfer_batch_bounce(batch);
	}

	if (usb_transfer_batch_bounce_required(batch))
		usb_transfer_batch_bounce(batch);

	batch->on_complete = req->on_complete;
	batch->on_complete_data = req->arg;

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
