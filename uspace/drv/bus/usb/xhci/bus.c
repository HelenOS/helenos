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
#include <usb/host/ddf_helpers.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <macros.h>
#include <stdbool.h>

#include "bus.h"
#include "endpoint.h"
#include "transfers.h"

/** Element of the hash table. */
typedef struct {
	ht_link_t link;

	/** Device */
	xhci_device_t *device;
} hashed_device_t;

/** TODO: Still some copy-pasta left...
 */
int xhci_bus_enumerate_device(xhci_bus_t *bus, xhci_hc_t *hc, device_t *dev)
{
	int err;

	/* TODO: get speed from the default address reservation */
	dev->speed = USB_SPEED_FULL;

	/* Manage TT */
	if (dev->hub->speed == USB_SPEED_HIGH && usb_speed_is_11(dev->speed)) {
		/* For LS devices under HS hub */
		/* TODO: How about SS hubs? */
		dev->tt.address = dev->hub->address;
		dev->tt.port = dev->port;
	}
	else {
		/* Inherit hub's TT */
		dev->tt = dev->hub->tt;
	}

	/* Assign an address to the device */
	if ((err = xhci_rh_address_device(&hc->rh, dev, bus))) {
		usb_log_error("Failed to setup address of the new device: %s", str_error(err));
		return err;
	}

	/* Read the device descriptor, derive the match ids */
	if ((err = hcd_ddf_device_explore(hc->hcd, dev))) {
		usb_log_error("Device(%d): Failed to explore device: %s", dev->address, str_error(err));
		bus_release_address(&bus->base, dev->address);
		return err;
	}

	return EOK;
}

int xhci_bus_remove_device(xhci_bus_t *bus, xhci_hc_t *hc, device_t *dev)
{
	// TODO: Implement me!
	return ENOTSUP;
}

/** Ops receive generic bus_t pointer. */
static inline xhci_bus_t *bus_to_xhci_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (xhci_bus_t *) bus_base;
}

static int enumerate_device(bus_t *bus_base, hcd_t *hcd, device_t *dev)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	return xhci_bus_enumerate_device(bus, hc, dev);
}

static int remove_device(bus_t *bus_base, hcd_t *hcd, device_t *dev)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	return xhci_bus_remove_device(bus, hc, dev);
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

static int hashed_device_create(xhci_bus_t *bus, hashed_device_t **hashed_dev, usb_address_t address)
{
	int res;
	xhci_device_t *dev = (xhci_device_t *) malloc(sizeof(xhci_device_t));
	if (!dev) {
		res = ENOMEM;
		goto err_xhci_dev_alloc;
	}

	res = xhci_device_init(dev, bus, address);
	if (res != EOK) {
		goto err_xhci_dev_init;
	}

	hashed_device_t *ret_dev = (hashed_device_t *) malloc(sizeof(hashed_device_t));
	if (!ret_dev) {
		res = ENOMEM;
		goto err_hashed_dev_alloc;
	}

	ret_dev->device = dev;

	usb_log_info("Device(%d) registered to XHCI bus.", dev->address);

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
	usb_log_info("Device(%d) released from XHCI bus.", hashed_dev->device->address);

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
		res = hashed_device_create(bus, &hashed_dev, ep->target.address);

		if (res != EOK)
			return res;
	}

	usb_log_info("Endpoint(%d:%d) registered to XHCI bus.", ep->target.address, ep->target.endpoint);

	return xhci_device_add_endpoint(hashed_dev->device, xhci_endpoint_get(ep));
}

static int release_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	usb_log_info("Endpoint(%d:%d) released from XHCI bus.", ep->target.address, ep->target.endpoint);

	hashed_device_t *hashed_dev;
	int res = hashed_device_find_by_address(bus, ep->target.address, &hashed_dev);
	if (res != EOK)
		return res;

	res = xhci_device_remove_endpoint(hashed_dev->device, xhci_endpoint_get(ep));
	if (res != EOK)
		return res;

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

static usb_transfer_batch_t *create_batch(bus_t *bus, endpoint_t *ep)
{
	xhci_transfer_t *transfer = xhci_transfer_create(ep);
	return &transfer->batch;
}

static void destroy_batch(usb_transfer_batch_t *batch)
{
	xhci_transfer_destroy(xhci_transfer_from_batch(batch));
}

static const bus_ops_t xhci_bus_ops = {
	.enumerate_device = enumerate_device,
	.remove_device = remove_device,

	.create_endpoint = create_endpoint,
	.destroy_endpoint = destroy_endpoint,

	.register_endpoint = register_endpoint,
	.release_endpoint = release_endpoint,
	.find_endpoint = find_endpoint,

	.request_address = NULL,
	.release_address = NULL,
	.reset_toggle = reset_toggle,

	.count_bw = count_bw,

	.endpoint_get_toggle = endpoint_get_toggle,
	.endpoint_set_toggle = endpoint_set_toggle,

	.create_batch = create_batch,
	.destroy_batch = destroy_batch,
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

	bus_init(&bus->base, sizeof(device_t));

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
