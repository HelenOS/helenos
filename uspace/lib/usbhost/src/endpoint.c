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

#include <usb/host/endpoint.h>
#include <usb/host/bus.h>

#include <assert.h>
#include <atomic.h>
#include <mem.h>
#include <stdlib.h>

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
			assert(!ep->active);

			/* Assume mostly the eps will be allocated by malloc. */
			free(ep);
		}
	}
}

/** Mark the endpoint as active and block access for further fibrils.
 * @param ep endpoint_t structure.
 */
void endpoint_use(endpoint_t *ep)
{
	assert(ep);
	/* Add reference for active endpoint. */
	endpoint_add_ref(ep);
	fibril_mutex_lock(&ep->guard);
	while (ep->active)
		fibril_condvar_wait(&ep->avail, &ep->guard);
	ep->active = true;
	fibril_mutex_unlock(&ep->guard);
}

/** Mark the endpoint as inactive and allow access for further fibrils.
 * @param ep endpoint_t structure.
 */
void endpoint_release(endpoint_t *ep)
{
	assert(ep);
	fibril_mutex_lock(&ep->guard);
	ep->active = false;
	fibril_mutex_unlock(&ep->guard);
	fibril_condvar_signal(&ep->avail);
	/* Drop reference for active endpoint. */
	endpoint_del_ref(ep);
}

/** Get the value of toggle bit. Either uses the toggle_get op, or just returns
 * the value of the toggle.
 * @param ep endpoint_t structure.
 */
int endpoint_toggle_get(endpoint_t *ep)
{
	assert(ep);
	fibril_mutex_lock(&ep->guard);
	const int ret = ep->bus->ops.endpoint_get_toggle
	    ? ep->bus->ops.endpoint_get_toggle(ep)
	    : ep->toggle;
	fibril_mutex_unlock(&ep->guard);
	return ret;
}

/** Set the value of toggle bit. Either uses the toggle_set op, or just sets
 * the toggle inside.
 * @param ep endpoint_t structure.
 */
void endpoint_toggle_set(endpoint_t *ep, unsigned toggle)
{
	assert(ep);
	assert(toggle == 0 || toggle == 1);
	fibril_mutex_lock(&ep->guard);
	if (ep->bus->ops.endpoint_set_toggle) {
		ep->bus->ops.endpoint_set_toggle(ep, toggle);
	}
	else {
		ep->toggle = toggle;
	}
	fibril_mutex_unlock(&ep->guard);
}


/**
 * @}
 */
