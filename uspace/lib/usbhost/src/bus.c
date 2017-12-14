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

#include <ddf/driver.h>
#include <errno.h>
#include <mem.h>
#include <stdio.h>
#include <usb/debug.h>

#include "endpoint.h"
#include "bus.h"

/**
 * Initializes the bus structure.
 */
void bus_init(bus_t *bus, hcd_t *hcd, size_t device_size)
{
	assert(bus);
	assert(hcd);
	assert(device_size >= sizeof(device_t));
	memset(bus, 0, sizeof(bus_t));

	fibril_mutex_initialize(&bus->guard);
	bus->hcd = hcd;
	bus->device_size = device_size;
}

int bus_device_init(device_t *dev, bus_t *bus)
{
	assert(bus);
	assert(bus->hcd);

	memset(dev, 0, sizeof(*dev));

	dev->bus = bus;

	link_initialize(&dev->link);
	list_initialize(&dev->devices);
	fibril_mutex_initialize(&dev->guard);

	return EOK;
}

int bus_device_set_default_name(device_t *dev)
{
	assert(dev);
	assert(dev->fun);

	char buf[10] = { 0 }; /* usbxyz-ss */
	snprintf(buf, sizeof(buf) - 1, "usb%u-%cs",
	    dev->address, usb_str_speed(dev->speed)[0]);

	return ddf_fun_set_name(dev->fun, buf);
}

int bus_device_enumerate(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_enumerate);
	if (!ops)
		return ENOTSUP;

	return ops->device_enumerate(dev);
}

int bus_device_remove(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_remove);

	if (!ops)
		return ENOTSUP;

	return ops->device_remove(dev);
}

int bus_device_online(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_online);
	if (!ops)
		return ENOTSUP;

	return ops->device_online(dev);
}

int bus_device_offline(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_offline);
	if (!ops)
		return ENOTSUP;

	return ops->device_offline(dev);
}

int bus_endpoint_add(device_t *device, const usb_endpoint_desc_t *desc, endpoint_t **out_ep)
{
	int err;
	assert(device);

	bus_t *bus = device->bus;

	if (desc->max_packet_size == 0 || desc->packets == 0) {
		usb_log_warning("Invalid endpoint description (mps %zu, %u packets)", desc->max_packet_size, desc->packets);
		return EINVAL;
	}

	const bus_ops_t *create_ops = BUS_OPS_LOOKUP(bus->ops, endpoint_create);
	const bus_ops_t *register_ops = BUS_OPS_LOOKUP(bus->ops, endpoint_register);
	if (!create_ops || !register_ops)
		return ENOTSUP;

	endpoint_t *ep = create_ops->endpoint_create(device, desc);
	if (!ep)
		return ENOMEM;

	/* Temporary reference */
	endpoint_add_ref(ep);

	fibril_mutex_lock(&bus->guard);
	err = register_ops->endpoint_register(ep);
	fibril_mutex_unlock(&bus->guard);

	if (out_ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
		*out_ep = ep;
	}

	/* Temporary reference */
	endpoint_del_ref(ep);
	return err;
}

/** Searches for an endpoint. Returns a reference.
 */
endpoint_t *bus_find_endpoint(device_t *device, usb_target_t endpoint, usb_direction_t dir)
{
	assert(device);

	bus_t *bus = device->bus;

	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, device_find_endpoint);
	if (!ops)
		return NULL;

	fibril_mutex_lock(&bus->guard);
	endpoint_t *ep = ops->device_find_endpoint(device, endpoint, dir);
	if (ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
	}

	fibril_mutex_unlock(&bus->guard);
	return ep;
}

int bus_endpoint_remove(endpoint_t *ep)
{
	assert(ep);

	bus_t *bus = endpoint_get_bus(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(ep->device->bus->ops, endpoint_unregister);
	if (!ops)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = ops->endpoint_unregister(ep);
	fibril_mutex_unlock(&bus->guard);

	if (r)
		return r;

	/* Bus reference */
	endpoint_del_ref(ep);

	return EOK;
}

int bus_reserve_default_address(bus_t *bus, usb_speed_t speed)
{
	assert(bus);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, reserve_default_address);
	if (!ops)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = ops->reserve_default_address(bus, speed);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_release_default_address(bus_t *bus)
{
	assert(bus);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, release_default_address);
	if (!ops)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = ops->release_default_address(bus);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

int bus_reset_toggle(bus_t *bus, usb_target_t target, bool all)
{
	assert(bus);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, reset_toggle);
	if (!ops)
		return ENOTSUP;

	fibril_mutex_lock(&bus->guard);
	const int r = ops->reset_toggle(bus, target, all);
	fibril_mutex_unlock(&bus->guard);
	return r;
}

/**
 * @}
 */
