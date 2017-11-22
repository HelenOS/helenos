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

#include "usb_transfer_batch.h"
#include "bus.h"

#include "endpoint.h"

/** Initialize provided endpoint structure.
 */
void endpoint_init(endpoint_t *ep, bus_t *bus)
{
	memset(ep, 0, sizeof(endpoint_t));

	ep->bus = bus;
	atomic_set(&ep->refcnt, 0);
	link_initialize(&ep->link);
	fibril_mutex_initialize(&ep->guard);
	fibril_condvar_initialize(&ep->avail);
}

void endpoint_add_ref(endpoint_t *ep)
{
	atomic_inc(&ep->refcnt);
}

void endpoint_del_ref(endpoint_t *ep)
{
	if (atomic_predec(&ep->refcnt) == 0) {
		if (ep->bus->ops.destroy_endpoint) {
			ep->bus->ops.destroy_endpoint(ep);
		}
		else {
			assert(ep->active_batch == NULL);

			/* Assume mostly the eps will be allocated by malloc. */
			free(ep);
		}
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

	return ep->bus->ops.endpoint_get_toggle
	    ? ep->bus->ops.endpoint_get_toggle(ep)
	    : ep->toggle;
}

/** Set the value of toggle bit. Either uses the toggle_set op, or just sets
 * the toggle inside.
 * @param ep endpoint_t structure.
 */
void endpoint_toggle_set(endpoint_t *ep, bool toggle)
{
	assert(ep);

	if (ep->bus->ops.endpoint_set_toggle) {
		ep->bus->ops.endpoint_set_toggle(ep, toggle);
	}
	else {
		ep->toggle = toggle;
	}
}

/**
 * @}
 */
