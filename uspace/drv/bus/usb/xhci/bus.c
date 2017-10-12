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

	/** Endpoint */
	xhci_endpoint_t *endpoint;
} hashed_endpoint_t;

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

static int endpoint_find_by_target(xhci_bus_t *bus, usb_target_t target, hashed_endpoint_t **ep)
{
	ht_link_t *link = hash_table_find(&bus->endpoints, &target.packed);
	if (link == NULL)
		return ENOENT;

	*ep = hash_table_get_inst(link, hashed_endpoint_t, link);
	return EOK;
}

static int register_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	hashed_endpoint_t *hashed_ep =
	    (hashed_endpoint_t *) malloc(sizeof(hashed_endpoint_t));
	if (!hashed_ep)
		return ENOMEM;

	hashed_ep->endpoint = (xhci_endpoint_t *) ep;
	hash_table_insert(&bus->endpoints, &hashed_ep->link);

	return EOK;
}

static int release_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	hashed_endpoint_t *hashed_ep;
	int res = endpoint_find_by_target(bus, ep->target, &hashed_ep);
	if (res != EOK)
		return res;

	hash_table_remove(&bus->endpoints, &ep->target.packed);
	free(hashed_ep);

	return EOK;
}

static endpoint_t* find_endpoint(bus_t *bus_base, usb_target_t target, usb_direction_t direction)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	hashed_endpoint_t *hashed_ep;
	int res = endpoint_find_by_target(bus, target, &hashed_ep);
	if (res != EOK)
		return NULL;

	return (endpoint_t *) hashed_ep->endpoint;
}

static int request_address(bus_t *bus_base, usb_address_t *addr, bool strict, usb_speed_t speed)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static int get_speed(bus_t *bus_base, usb_address_t address, usb_speed_t *speed)
{
	// TODO: Implement me!
	return ENOTSUP;
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
static int endpoint_get_toggle(endpoint_t *ep)
{
	// TODO: Implement me!
	return ENOTSUP;
}

static void endpoint_set_toggle(endpoint_t *ep, unsigned toggle)
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

static size_t endpoint_ht_hash(const ht_link_t *item)
{
	hashed_endpoint_t *ep = hash_table_get_inst(item, hashed_endpoint_t, link);
	return (size_t) hash_mix32(ep->endpoint->base.target.packed);
}

static size_t endpoint_ht_key_hash(void *key)
{
	return (size_t) hash_mix32(*(uint32_t *)key);
}

static bool endpoint_ht_key_equal(void *key, const ht_link_t *item)
{
	hashed_endpoint_t *ep = hash_table_get_inst(item, hashed_endpoint_t, link);
	return ep->endpoint->base.target.packed == *(uint32_t *) key;
}

/** Operations for the endpoint hash table. */
static hash_table_ops_t endpoint_ht_ops = {
	.hash = endpoint_ht_hash,
	.key_hash = endpoint_ht_key_hash,
	.key_equal = endpoint_ht_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

int xhci_bus_init(xhci_bus_t *bus, hcd_t *hcd)
{
	assert(bus);

	bus_init(&bus->base, hcd);

	if (!hash_table_create(&bus->endpoints, 0, 0, &endpoint_ht_ops)) {
		// FIXME: Dealloc base!
		return ENOMEM;
	}

	bus->base.ops = xhci_bus_ops;
	return EOK;
}

void xhci_bus_fini(xhci_bus_t *bus)
{
	hash_table_destroy(&bus->endpoints);
}
/**
 * @}
 */
