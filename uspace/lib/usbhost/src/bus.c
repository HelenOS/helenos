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
#include <ddf/driver.h>

#include <mem.h>
#include <errno.h>
#include <stdio.h>

/**
 * Initializes the bus structure.
 */
void bus_init(bus_t *bus, size_t device_size)
{
	assert(bus);
	assert(device_size >= sizeof(device_t));
	memset(bus, 0, sizeof(bus_t));

	fibril_mutex_initialize(&bus->guard);
	bus->device_size = device_size;
}

int device_init(device_t *dev)
{
	memset(dev, 0, sizeof(*dev));

	link_initialize(&dev->link);
	list_initialize(&dev->devices);
	fibril_mutex_initialize(&dev->guard);

	return EOK;
}

int bus_add_ep(bus_t *bus, device_t *device, usb_endpoint_t endpoint,
    usb_direction_t dir, usb_transfer_type_t type, size_t max_packet_size,
    unsigned packets, size_t size)
{
	assert(bus);
	assert(device);

	/* Temporary reference */
	endpoint_t *ep = bus_create_endpoint(bus);
	if (!ep)
		return ENOMEM;

	ep->target = (usb_target_t) {
		.address = device->address,
		.endpoint = endpoint,
	};

	ep->device = device;
	ep->direction = dir;
	ep->transfer_type = type;
	ep->max_packet_size = max_packet_size;
	ep->packets = packets;

	ep->bandwidth = bus_count_bw(ep, size);

	const int err = bus_register_endpoint(bus, ep);

	/* drop Temporary reference */
	endpoint_del_ref(ep);

	return err;
}

int bus_remove_ep(bus_t *bus, usb_target_t target, usb_direction_t dir)
{
	assert(bus);
	endpoint_t *ep = bus_find_endpoint(bus, target, dir);
	if (!ep)
		return ENOENT;

	return bus_release_endpoint(bus, ep);
}

int device_set_default_name(device_t *dev)
{
	assert(dev);
	assert(dev->fun);

	char buf[10] = { 0 }; /* usbxyz-ss */
	snprintf(buf, sizeof(buf) - 1, "usb%u-%cs",
	    dev->address, usb_str_speed(dev->speed)[0]);

	return ddf_fun_set_name(dev->fun, buf);
}

int bus_enumerate_device(bus_t *bus, hcd_t *hcd, device_t *dev)
{
	assert(bus);
	assert(hcd);
	assert(dev);

	if (!bus->ops.enumerate_device)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.enumerate_device(bus, hcd, dev);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_remove_device(bus_t *bus, hcd_t *hcd, device_t *dev)
{
	assert(bus);
	assert(dev);

	if (!bus->ops.remove_device)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.remove_device(bus, hcd, dev);
	fibril_mutex_unlock(&bus->guard);
	return r;
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
	assert(bus);
	assert(ep);

	fibril_mutex_lock(&bus->guard);
	const int r = bus->ops.release_endpoint(bus, ep);
	fibril_mutex_unlock(&bus->guard);

	if (r)
		return r;

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
