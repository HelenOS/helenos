/*
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
 *
 */

#include <usb/host/bus.h>
#include <usb/host/endpoint.h>

#include <mem.h>
#include <errno.h>

/**
 * Initializes the bus structure.
 */
void bus_init(bus_t *bus, hcd_t *hcd)
{
	memset(bus, 0, sizeof(bus_t));

	bus->hcd = hcd;
	fibril_mutex_initialize(&bus->guard);
}

endpoint_t *bus_create_endpoint(bus_t *bus)
{
	assert(bus);

	fibril_mutex_lock(&bus->guard);
	endpoint_t *ep = bus->ops.create_endpoint(bus);
	if (ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
	}
	fibril_mutex_unlock(&bus->guard);

	return ep;
}

int bus_register_endpoint(bus_t *bus, endpoint_t *ep)
{
	assert(bus);
	assert(ep);

	/* Bus reference */
	endpoint_add_ref(ep);

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.register_endpoint(bus, ep);
	fibril_mutex_unlock(&bus->guard);

	return r;
}

int bus_release_endpoint(bus_t *bus, endpoint_t *ep)
{
	int err;

	assert(bus);
	assert(ep);

	fibril_mutex_lock(&bus->guard);
	if ((err = bus->ops.release_endpoint(bus, ep)))
		return err;
	fibril_mutex_unlock(&bus->guard);

	/* Bus reference */
	endpoint_del_ref(ep);

	return EOK;
}

/** Searches for an endpoint. Returns a reference.
 */
endpoint_t *bus_find_endpoint(bus_t *bus, usb_target_t target, usb_direction_t dir)
{
	assert(bus);

	fibril_mutex_lock(&bus->guard);
	endpoint_t *ep = bus->ops.find_endpoint(bus, target, dir);
	if (ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
	}

	fibril_mutex_unlock(&bus->guard);
	return ep;
}

int bus_request_address(bus_t *bus, usb_address_t *hint, bool strict, usb_speed_t speed)
{
	assert(bus);

	if (!bus->ops.request_address)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.request_address(bus, hint, strict, speed);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_release_address(bus_t *bus, usb_address_t address)
{
	assert(bus);

	if (!bus->ops.release_address)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.release_address(bus, address);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_get_speed(bus_t *bus, usb_address_t address, usb_speed_t *speed)
{
	assert(bus);
	assert(speed);

	if (!bus->ops.get_speed)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.get_speed(bus, address, speed);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_reset_toggle(bus_t *bus, usb_target_t target, bool all)
{
	assert(bus);

	if (!bus->ops.reset_toggle)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.reset_toggle(bus, target, all);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

size_t bus_count_bw(endpoint_t *ep, size_t size)
{
	assert(ep);

	fibril_mutex_lock(&ep->guard);
	const size_t bw = ep->bus->ops.count_bw(ep, size);
	fibril_mutex_unlock(&ep->guard);
	return bw;
}

/**
 * @}
 */
