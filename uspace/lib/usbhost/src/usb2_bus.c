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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 * HC Endpoint management.
 */

#include <usb/host/usb2_bus.h>
#include <usb/host/endpoint.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdlib.h>
#include <stdbool.h>

/** Ops receive generic bus_t pointer. */
static inline usb2_bus_t *bus_to_usb2_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (usb2_bus_t *) bus_base;
}

/** Get list that holds endpoints for given address.
 * @param bus usb2_bus structure, non-null.
 * @param addr USB address, must be >= 0.
 * @return Pointer to the appropriate list.
 */
static list_t * get_list(usb2_bus_t *bus, usb_address_t addr)
{
	assert(bus);
	assert(addr >= 0);
	return &bus->devices[addr % ARRAY_SIZE(bus->devices)].endpoint_list;
}

/** Get a free USB address
 *
 * @param[in] bus Device manager structure to use.
 * @return Free address, or error code.
 */
static int usb_bus_get_free_address(usb2_bus_t *bus, usb_address_t *addr)
{
	usb_address_t new_address = bus->last_address;
	do {
		new_address = (new_address + 1) % USB_ADDRESS_COUNT;
		if (new_address == USB_ADDRESS_DEFAULT)
			new_address = 1;
		if (new_address == bus->last_address)
			return ENOSPC;
	} while (bus->devices[new_address].occupied);

	assert(new_address != USB_ADDRESS_DEFAULT);
	bus->last_address = new_address;

	*addr = new_address;
	return EOK;
}

/** Find endpoint.
 * @param bus usb_bus structure, non-null.
 * @param target Endpoint address.
 * @param direction Communication direction.
 * @return Pointer to endpoint_t structure representing given communication
 * target, NULL if there is no such endpoint registered.
 * @note Assumes that the internal mutex is locked.
 */
static endpoint_t *usb2_bus_find_ep(bus_t *bus_base, usb_target_t target, usb_direction_t direction)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);

	if (!usb_address_is_valid(target.address))
		return NULL;

	list_foreach(*get_list(bus, target.address), link, endpoint_t, ep) {
		if (((direction == ep->direction)
		       || (ep->direction == USB_DIRECTION_BOTH)
		       || (direction == USB_DIRECTION_BOTH))
		    && (target.packed == ep->target.packed))
			return ep;
	}
	return NULL;
}

static endpoint_t *usb2_bus_create_ep(bus_t *bus)
{
	endpoint_t *ep = malloc(sizeof(endpoint_t));
	if (!ep)
		return NULL;

	endpoint_init(ep, bus);
	return ep;
}

/** Register an endpoint to the bus. Reserves bandwidth.
 * @param bus usb_bus structure, non-null.
 * @param endpoint USB endpoint number.
 */
static int usb2_bus_register_ep(bus_t *bus_base, endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);
	assert(ep);

	usb_address_t address = ep->target.address;

	if (!usb_address_is_valid(address))
		return EINVAL;

	/* Check for speed and address */
	if (!bus->devices[address].occupied)
		return ENOENT;

	/* Check for existence */
	if (usb2_bus_find_ep(bus_base, ep->target, ep->direction))
		return EEXIST;

	/* Check for available bandwidth */
	if (ep->bandwidth > bus->free_bw)
		return ENOSPC;

	list_append(&ep->link, get_list(bus, ep->target.address));
	bus->free_bw -= ep->bandwidth;

	return EOK;
}


/** Release bandwidth reserved by the given endpoint.
 */
static int usb2_bus_release_ep(bus_t *bus_base, endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);
	assert(ep);

	bus->free_bw += ep->bandwidth;

	return EOK;
}

static int usb2_bus_reset_toggle(bus_t *bus_base, usb_target_t target, bool all)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);

	if (!usb_target_is_valid(target))
		return EINVAL;

	int ret = ENOENT;

	list_foreach(*get_list(bus, target.address), link, endpoint_t, ep) {
		if ((ep->target.address == target.address)
		    && (all || ep->target.endpoint == target.endpoint)) {
			endpoint_toggle_set(ep, 0);
			ret = EOK;
		}
	}
	return ret;
}

/** Unregister and destroy all endpoints using given address.
 * @param bus usb_bus structure, non-null.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @param callback Function to call after unregister, before destruction.
 * @arg Argument to pass to the callback function.
 * @return Error code.
 */
static int usb2_bus_release_address(bus_t *bus_base, usb_address_t address)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);

	if (!usb_address_is_valid(address))
		return EINVAL;

	const int ret = bus->devices[address].occupied ? EOK : ENOENT;
	bus->devices[address].occupied = false;

	list_t *list = get_list(bus, address);
	for (link_t *link = list_first(list); link != NULL; ) {
		endpoint_t *ep = list_get_instance(link, endpoint_t, link);
		link = list_next(link, list);
		assert(ep->target.address == address);
		list_remove(&ep->link);

		/* Drop bus reference */
		endpoint_del_ref(ep);
	}

	return ret;
}

/** Request USB address.
 * @param bus usb_device_manager
 * @param addr Pointer to requested address value, place to store new address
 * @parma strict Fail if the requested address is not available.
 * @return Error code.
 * @note Default address is only available in strict mode.
 */
static int usb2_bus_request_address(bus_t *bus_base, usb_address_t *addr, bool strict, usb_speed_t speed)
{
	int err;

	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);
	assert(addr);

	if (!usb_address_is_valid(*addr))
		return EINVAL;

	/* Only grant default address to strict requests */
	if ((*addr == USB_ADDRESS_DEFAULT) && !strict) {
		if ((err = usb_bus_get_free_address(bus, addr)))
			return err;
	}
	else if (bus->devices[*addr].occupied) {
		if (strict) {
			return ENOENT;
		}
		if ((err = usb_bus_get_free_address(bus, addr)))
			return err;
	}

	assert(usb_address_is_valid(*addr));
	assert(bus->devices[*addr].occupied == false);
	assert(*addr != USB_ADDRESS_DEFAULT || strict);

	bus->devices[*addr].occupied = true;
	bus->devices[*addr].speed = speed;

	return EOK;
}

/** Get speed assigned to USB address.
 *
 * @param[in] bus Device manager structure to use.
 * @param[in] address Address the caller wants to find.
 * @param[out] speed Assigned speed.
 * @return Error code.
 */
static int usb2_bus_get_speed(bus_t *bus_base, usb_address_t address, usb_speed_t *speed)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);

	if (!usb_address_is_valid(address)) {
		return EINVAL;
	}

	const int ret = bus->devices[address].occupied ? EOK : ENOENT;
	if (speed && bus->devices[address].occupied) {
		*speed = bus->devices[address].speed;
	}

	return ret;
}

static const bus_ops_t usb2_bus_ops = {
	.create_endpoint = usb2_bus_create_ep,
	.find_endpoint = usb2_bus_find_ep,
	.release_endpoint = usb2_bus_release_ep,
	.register_endpoint = usb2_bus_register_ep,
	.request_address = usb2_bus_request_address,
	.release_address = usb2_bus_release_address,
	.reset_toggle = usb2_bus_reset_toggle,
	.get_speed = usb2_bus_get_speed,
};

/** Initialize to default state.
 *
 * @param bus usb_bus structure, non-null.
 * @param available_bandwidth Size of the bandwidth pool.
 * @param bw_count function to use to calculate endpoint bw requirements.
 * @return Error code.
 */
int usb2_bus_init(usb2_bus_t *bus, size_t available_bandwidth, count_bw_func_t count_bw)
{
	assert(bus);

	bus_init(&bus->base);

	bus->base.ops = usb2_bus_ops;
	bus->base.ops.count_bw = count_bw;

	bus->free_bw = available_bandwidth;
	bus->last_address = 0;
	for (unsigned i = 0; i < ARRAY_SIZE(bus->devices); ++i) {
		list_initialize(&bus->devices[i].endpoint_list);
		bus->devices[i].speed = USB_SPEED_MAX;
		bus->devices[i].occupied = false;
	}
	return EOK;
}
/**
 * @}
 */
