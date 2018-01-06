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

#include "usb_transfer_batch.h"
#include "bus.h"

#include "endpoint.h"

/** Initialize provided endpoint structure.
 */
void endpoint_init(endpoint_t *ep, device_t *dev, const usb_endpoint_descriptors_t *desc)
{
	memset(ep, 0, sizeof(endpoint_t));

	assert(dev);
	ep->device = dev;

	atomic_set(&ep->refcnt, 0);
	link_initialize(&ep->link);
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

static inline const bus_ops_t *get_bus_ops(endpoint_t *ep)
{
	return ep->device->bus->ops;
}

void endpoint_add_ref(endpoint_t *ep)
{
	atomic_inc(&ep->refcnt);
}

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

void endpoint_del_ref(endpoint_t *ep)
{
	if (atomic_predec(&ep->refcnt) == 0) {
		endpoint_destroy(ep);
	}
}

/** Mark the endpoint as active and block access for further fibrils.
 * @param ep endpoint_t structure.
 */
void endpoint_activate_locked(endpoint_t *ep, usb_transfer_batch_t *batch)
{
	assert(ep);
	assert(batch);
	assert(batch->ep == ep);
	assert(fibril_mutex_is_locked(&ep->guard));

	while (ep->active_batch != NULL)
		fibril_condvar_wait(&ep->avail, &ep->guard);
	ep->active_batch = batch;
}

/** Mark the endpoint as inactive and allow access for further fibrils.
 * @param ep endpoint_t structure.
 */
void endpoint_deactivate_locked(endpoint_t *ep)
{
	assert(ep);
	assert(fibril_mutex_is_locked(&ep->guard));

	if (ep->active_batch && ep->active_batch->error == EOK)
		usb_transfer_batch_reset_toggle(ep->active_batch);

	ep->active_batch = NULL;
	fibril_condvar_signal(&ep->avail);
}

/** Abort an active batch on endpoint, if any.
 *
 * @param[in] ep endpoint_t structure.
 */
void endpoint_abort(endpoint_t *ep)
{
	assert(ep);

	fibril_mutex_lock(&ep->guard);
	usb_transfer_batch_t *batch = ep->active_batch;
	endpoint_deactivate_locked(ep);
	fibril_mutex_unlock(&ep->guard);

	if (batch)
		usb_transfer_batch_abort(batch);
}

/** Get the value of toggle bit. Either uses the toggle_get op, or just returns
 * the value of the toggle.
 * @param ep endpoint_t structure.
 */
int endpoint_toggle_get(endpoint_t *ep)
{
	assert(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(get_bus_ops(ep), endpoint_get_toggle);
	return ops
	    ? ops->endpoint_get_toggle(ep)
	    : ep->toggle;
}

/** Set the value of toggle bit. Either uses the toggle_set op, or just sets
 * the toggle inside.
 * @param ep endpoint_t structure.
 */
void endpoint_toggle_set(endpoint_t *ep, bool toggle)
{
	assert(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(get_bus_ops(ep), endpoint_set_toggle);
	if (ops) {
		ops->endpoint_set_toggle(ep, toggle);
	}
	else {
		ep->toggle = toggle;
	}
}

ssize_t endpoint_count_bw(endpoint_t *ep, size_t packet_size)
{
	assert(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(get_bus_ops(ep), endpoint_count_bw);
	if (!ops)
		return 0;

	return ops->endpoint_count_bw(ep, packet_size);
}

/** Prepare generic usb_transfer_batch and schedule it.
 * @param ep Endpoint for which the batch shall be created.
 * @param target address and endpoint number.
 * @param setup_data Data to use in setup stage (Control communication type)
 * @param in Callback for device to host communication.
 * @param out Callback for host to device communication.
 * @param arg Callback parameter.
 * @param name Communication identifier (for nicer output).
 * @return Error code.
 */
int endpoint_send_batch(endpoint_t *ep, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_callback_t on_complete, void *arg, const char *name)
{
	usb_log_debug2("%s %d:%d %zu(%zu).\n",
	    name, target.address, target.endpoint, size, ep->max_packet_size);

	bus_t *bus = endpoint_get_bus(ep);
	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, batch_schedule);
	if (!ops) {
		usb_log_error("HCD does not implement scheduler.\n");
		return ENOTSUP;
	}

	/* Offline devices don't schedule transfers other than on EP0. */
	if (!ep->device->online && ep->endpoint > 0) {
		return EAGAIN;
	}

	const size_t bw = endpoint_count_bw(ep, size);
	/* Check if we have enough bandwidth reserved */
	if (ep->bandwidth < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    ep->device->address, ep->endpoint, name, bw, ep->bandwidth);
		return ENOSPC;
	}

	usb_transfer_batch_t *batch = usb_transfer_batch_create(ep);
	if (!batch) {
		usb_log_error("Failed to create transfer batch.\n");
		return ENOMEM;
	}

	batch->target = target;
	batch->buffer = data;
	batch->buffer_size = size;
	batch->setup.packed = setup_data;
	batch->dir = direction;
	batch->on_complete = on_complete;
	batch->on_complete_data = arg;

	/* Check for commands that reset toggle bit */
	if (ep->transfer_type == USB_TRANSFER_CONTROL)
		batch->toggle_reset_mode
			= hcd_get_request_toggle_reset_mode(&batch->setup.packet);

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
