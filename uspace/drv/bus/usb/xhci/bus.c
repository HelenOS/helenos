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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 * HC Endpoint management.
 */

#include <adt/hash_table.h>
#include <adt/hash.h>
#include <usb/host/endpoint.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdbool.h>

#include "bus.h"
#include "endpoint.h"

/** Element of the hash table. */
typedef struct {
	ht_link_t link;

	/** Device */
	xhci_device_t *device;
} hashed_device_t;

/** Ops receive generic bus_t pointer. */
static inline xhci_bus_t *bus_to_xhci_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (xhci_bus_t *) bus_base;
}

static endpoint_t *create_endpoint(bus_t *base)
{
	xhci_bus_t *bus = bus_to_xhci_bus(base);

	xhci_endpoint_t *ep = malloc(sizeof(xhci_endpoint_t));
	if (!ep)
		return NULL;

	if (xhci_endpoint_init(ep, bus)) {
		free(ep);
		return NULL;
	}

	return &ep->base;
}

static void destroy_endpoint(endpoint_t *ep)
{
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);

	xhci_endpoint_fini(xhci_ep);
	free(xhci_ep);
}

static int hashed_device_find_by_address(xhci_bus_t *bus, usb_address_t address, hashed_device_t **dev)
{
	ht_link_t *link = hash_table_find(&bus->devices, &address);
	if (link == NULL)
		return ENOENT;

	*dev = hash_table_get_inst(link, hashed_device_t, link);
	return EOK;
}

static int xhci_endpoint_find_by_target(xhci_bus_t *bus, usb_target_t target, xhci_endpoint_t **ep)
{
	hashed_device_t *dev;
	int res = hashed_device_find_by_address(bus, target.address, &dev);
	if (res != EOK)
		return res;

	xhci_endpoint_t *ret_ep = xhci_device_get_endpoint(dev->device, target.endpoint);
	if (!ret_ep)
		return ENOENT;

	*ep = ret_ep;
	return EOK;
}

static int hashed_device_create(xhci_bus_t *bus, hashed_device_t **hashed_dev)
{
	int res;
	xhci_device_t *dev = (xhci_device_t *) malloc(sizeof(xhci_device_t));
	if (!dev) {
		res = ENOMEM;
		goto err_xhci_dev_alloc;
	}

	res = xhci_device_init(dev, bus);
	if (res != EOK) {
		goto err_xhci_dev_init;
	}

	// TODO: Set device data.

	hashed_device_t *ret_dev = (hashed_device_t *) malloc(sizeof(hashed_device_t));
	if (!ret_dev) {
		res = ENOMEM;
		goto err_hashed_dev_alloc;
	}

	ret_dev->device = dev;

	hash_table_insert(&bus->devices, &ret_dev->link);
	*hashed_dev = ret_dev;
	return EOK;

err_hashed_dev_alloc:
err_xhci_dev_init:
	free(dev);
err_xhci_dev_alloc:
	return res;
}

static int hashed_device_remove(xhci_bus_t *bus, hashed_device_t *hashed_dev)
{
	hash_table_remove(&bus->devices, &hashed_dev->device->address);
	xhci_device_fini(hashed_dev->device);
	free(hashed_dev->device);
	free(hashed_dev);

	return EOK;
}

static int register_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	hashed_device_t *hashed_dev;
	int res = hashed_device_find_by_address(bus, ep->target.address, &hashed_dev);
	if (res != EOK && res != ENOENT)
		return res;

	if (res == ENOENT) {
		res = hashed_device_create(bus, &hashed_dev);

		if (res != EOK)
			return res;
	}

	return xhci_device_add_endpoint(hashed_dev->device, xhci_endpoint_get(ep));
}

static int release_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	hashed_device_t *hashed_dev;
	int res = hashed_device_find_by_address(bus, ep->target.address, &hashed_dev);
	if (res != EOK)
		return res;

	xhci_device_remove_endpoint(hashed_dev->device, xhci_endpoint_get(ep));

	if (hashed_dev->device->active_endpoint_count == 0) {
		res = hashed_device_remove(bus, hashed_dev);

		if (res != EOK)
			return res;
	}

	return EOK;
}

static endpoint_t* find_endpoint(bus_t *bus_base, usb_target_t target, usb_direction_t direction)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	xhci_endpoint_t *ep;
	int res = xhci_endpoint_find_by_target(bus, target, &ep);
	if (res != EOK)
		return NULL;

	return &ep->base;
}

static int request_address(bus_t *bus_base, usb_address_t *addr, bool strict, usb_speed_t speed)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static int get_speed(bus_t *bus_base, usb_address_t address, usb_speed_t *speed)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	// TODO: Use `xhci_get_port_speed` once we find the port corresponding to `address`.
	*speed = USB_SPEED_SUPER;
	return EOK;
}

static int release_address(bus_t *bus_base, usb_address_t address)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static int reset_toggle(bus_t *bus_base, usb_target_t target, bool all)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static size_t count_bw(endpoint_t *ep, size_t size)
{
	// TODO: Implement me!
	return 0;
}

/* Endpoint ops, optional (have generic fallback) */
static bool endpoint_get_toggle(endpoint_t *ep)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static void endpoint_set_toggle(endpoint_t *ep, bool toggle)
{
	// TODO: Implement me!
}

static const bus_ops_t xhci_bus_ops = {
	.create_endpoint = create_endpoint,
	.destroy_endpoint = destroy_endpoint,

	.register_endpoint = register_endpoint,
	.release_endpoint = release_endpoint,
	.find_endpoint = find_endpoint,

	.request_address = request_address,
	.get_speed = get_speed,
	.release_address = release_address,
	.reset_toggle = reset_toggle,

	.count_bw = count_bw,

	.endpoint_get_toggle = endpoint_get_toggle,
	.endpoint_set_toggle = endpoint_set_toggle,
};

static size_t device_ht_hash(const ht_link_t *item)
{
	hashed_device_t *dev = hash_table_get_inst(item, hashed_device_t, link);
	return (size_t) hash_mix(dev->device->address);
}

static size_t device_ht_key_hash(void *key)
{
	return (size_t) hash_mix(*(usb_address_t *)key);
}

static bool device_ht_key_equal(void *key, const ht_link_t *item)
{
	hashed_device_t *dev = hash_table_get_inst(item, hashed_device_t, link);
	return dev->device->address == *(usb_address_t *) key;
}

/** Operations for the device hash table. */
static hash_table_ops_t device_ht_ops = {
	.hash = device_ht_hash,
	.key_hash = device_ht_key_hash,
	.key_equal = device_ht_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

int xhci_bus_init(xhci_bus_t *bus)
{
	assert(bus);

	bus_init(&bus->base);

	if (!hash_table_create(&bus->devices, 0, 0, &device_ht_ops)) {
		// FIXME: Dealloc base!
		return ENOMEM;
	}

	bus->base.ops = xhci_bus_ops;
	return EOK;
}

void xhci_bus_fini(xhci_bus_t *bus)
{
	// FIXME: Make sure no devices are in the hash table.

	hash_table_destroy(&bus->devices);
}
/**
 * @}
 */
