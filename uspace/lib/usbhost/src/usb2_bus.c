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

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb/usb.h>

#include "endpoint.h"
#include "ddf_helpers.h"

#include "usb2_bus.h"

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

/** Get speed assigned to USB address.
 *
 * @param[in] bus Device manager structure to use.
 * @param[in] address Address the caller wants to find.
 * @param[out] speed Assigned speed.
 * @return Error code.
 */
static int get_speed(usb2_bus_t *bus, usb_address_t address, usb_speed_t *speed)
{
	if (!usb_address_is_valid(address))
		return EINVAL;

	if (!bus->devices[address].occupied)
		return ENOENT;

	if (speed)
		*speed = bus->devices[address].speed;

	return EOK;
}

/** Get a free USB address
 *
 * @param[in] bus Device manager structure to use.
 * @return Free address, or error code.
 */
static int get_free_address(usb2_bus_t *bus, usb_address_t *addr)
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

/** Unregister and destroy all endpoints using given address.
 * @param bus usb_bus structure, non-null.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @return Error code.
 */
static int release_address(usb2_bus_t *bus, usb_address_t address)
{
	if (!usb_address_is_valid(address))
		return EINVAL;

	const int ret = bus->devices[address].occupied ? EOK : ENOENT;
	bus->devices[address].occupied = false;

	list_t *list = get_list(bus, address);
	for (link_t *link = list_first(list); link != NULL; ) {
		endpoint_t *ep = list_get_instance(link, endpoint_t, link);
		link = list_next(link, list);

		assert(ep->device->address == address);
		list_remove(&ep->link);

		usb_log_warning("Endpoint %d:%d %s was left behind, removing.\n",
		    address, ep->endpoint, usb_str_direction(ep->direction));

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
static int request_address(usb2_bus_t *bus, usb_address_t *addr, bool strict, usb_speed_t speed)
{
	int err;

	assert(bus);
	assert(addr);

	if (!usb_address_is_valid(*addr))
		return EINVAL;

	/* Only grant default address to strict requests */
	if ((*addr == USB_ADDRESS_DEFAULT) && !strict) {
		if ((err = get_free_address(bus, addr)))
			return err;
	}
	else if (bus->devices[*addr].occupied) {
		if (strict) {
			return ENOENT;
		}
		if ((err = get_free_address(bus, addr)))
			return err;
	}

	assert(usb_address_is_valid(*addr));
	assert(bus->devices[*addr].occupied == false);
	assert(*addr != USB_ADDRESS_DEFAULT || strict);

	bus->devices[*addr].occupied = true;
	bus->devices[*addr].speed = speed;

	return EOK;
}

static const usb_endpoint_desc_t usb2_default_control_ep = {
	.endpoint_no = 0,
	.transfer_type = USB_TRANSFER_CONTROL,
	.direction = USB_DIRECTION_BOTH,
	.max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE,
	.packets = 1,
};


static const usb_target_t usb2_default_target = {{
	.address = USB_ADDRESS_DEFAULT,
	.endpoint = 0,
}};

static int address_device(device_t *dev)
{
	int err;

	usb2_bus_t *bus = (usb2_bus_t *) dev->bus;
	hcd_t *hcd = (hcd_t *) bus->base.hcd;

	/* The default address is currently reserved for this device */
	dev->address = USB_ADDRESS_DEFAULT;

	/** Reserve address early, we want pretty log messages */
	usb_address_t address;
	if ((err = request_address(bus, &address, false, dev->speed))) {
		usb_log_error("Failed to reserve new address: %s.",
		    str_error(err));
		return err;
	}
	usb_log_debug("Device(%d): Reserved new address.", address);

	/* Add default pipe on default address */
	usb_log_debug("Device(%d): Adding default target (0:0)", address);

	endpoint_t *default_ep;
	err = bus_endpoint_add(dev, &usb2_default_control_ep, &default_ep);
	if (err != EOK) {
		usb_log_error("Device(%d): Failed to add default target: %s.",
		    address, str_error(err));
		goto err_address;
	}

	uint16_t max_packet_size;
	if ((err = hcd_get_ep0_max_packet_size(&max_packet_size, hcd, dev)))
		goto err_address;

	/* Set new address */
	const usb_device_request_setup_packet_t set_address = SET_ADDRESS(address);

	usb_log_debug("Device(%d): Setting USB address.", address);
	err = hcd_send_batch_sync(hcd, dev, usb2_default_target, USB_DIRECTION_OUT,
	    NULL, 0, *(uint64_t *)&set_address, "set address");
	if (err != 0) {
		usb_log_error("Device(%d): Failed to set new address: %s.",
		    address, str_error(err));
		goto err_default_control_ep;
	}

	/* We need to remove ep before we change the address */
	if ((err = bus_endpoint_remove(default_ep))) {
		usb_log_error("Device(%d): Failed to unregister default target: %s", address, str_error(err));
		goto err_address;
	}
	endpoint_del_ref(default_ep);

	dev->address = address;

	const usb_endpoint_desc_t control_ep = {
		.endpoint_no = 0,
		.transfer_type = USB_TRANSFER_CONTROL,
		.direction = USB_DIRECTION_BOTH,
		.max_packet_size = max_packet_size,
		.packets = 1,
	};

	/* Register EP on the new address */
	usb_log_debug("Device(%d): Registering control EP.", address);
	err = bus_endpoint_add(dev, &control_ep, NULL);
	if (err != EOK) {
		usb_log_error("Device(%d): Failed to register EP0: %s",
		    address, str_error(err));
		goto err_address;
	}

	return EOK;

err_default_control_ep:
	bus_endpoint_remove(default_ep);
	endpoint_del_ref(default_ep);
err_address:
	release_address(bus, address);
	return err;
}

/** Enumerate a new USB device
 */
static int usb2_bus_device_enumerate(device_t *dev)
{
	int err;
	usb2_bus_t *bus = bus_to_usb2_bus(dev->bus);

	/* The speed of the new device was reported by the hub when reserving
	 * default address.
	 */
	if ((err = get_speed(bus, USB_ADDRESS_DEFAULT, &dev->speed))) {
		usb_log_error("Failed to verify speed: %s.", str_error(err));
		return err;
	}
	usb_log_debug("Found new %s speed USB device.", usb_str_speed(dev->speed));

	if (!dev->hub) {
		/* The device is the roothub */
		dev->tt = (usb_tt_address_t) {
			.address = -1,
			.port = 0,
		};
	} else {
		hcd_setup_device_tt(dev);
	}

	/* Assign an address to the device */
	if ((err = address_device(dev))) {
		usb_log_error("Failed to setup address of the new device: %s", str_error(err));
		return err;
	}

	/* Read the device descriptor, derive the match ids */
	if ((err = hcd_ddf_device_explore(dev))) {
		usb_log_error("Device(%d): Failed to explore device: %s", dev->address, str_error(err));
		release_address(bus, dev->address);
		return err;
	}

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
static endpoint_t *usb2_bus_find_ep(device_t *device, usb_target_t target, usb_direction_t direction)
{
	usb2_bus_t *bus = bus_to_usb2_bus(device->bus);

	assert(device->address == target.address);

	list_foreach(*get_list(bus, target.address), link, endpoint_t, ep) {
		if (((direction == ep->direction)
		       || (ep->direction == USB_DIRECTION_BOTH)
		       || (direction == USB_DIRECTION_BOTH))
		    && (target.endpoint == ep->endpoint))
			return ep;
	}
	return NULL;
}

static endpoint_t *usb2_bus_create_ep(device_t *dev, const usb_endpoint_desc_t *desc)
{
	endpoint_t *ep = malloc(sizeof(endpoint_t));
	if (!ep)
		return NULL;

	endpoint_init(ep, dev, desc);
	return ep;
}

static usb_target_t usb2_ep_to_target(endpoint_t *ep)
{
	assert(ep);
	assert(ep->device);

	return (usb_target_t) {{
		.address = ep->device->address,
		.endpoint = ep->endpoint,
	}};
}

/** Register an endpoint to the bus. Reserves bandwidth.
 * @param bus usb_bus structure, non-null.
 * @param endpoint USB endpoint number.
 */
static int usb2_bus_register_ep(endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(ep->device->bus);
	assert(fibril_mutex_is_locked(&bus->base.guard));
	assert(ep);

	/* Check for existence */
	if (usb2_bus_find_ep(ep->device, usb2_ep_to_target(ep), ep->direction))
		return EEXIST;

	/* Check for available bandwidth */
	if (ep->bandwidth > bus->free_bw)
		return ENOSPC;

	endpoint_add_ref(ep);
	list_append(&ep->link, get_list(bus, ep->device->address));
	bus->free_bw -= ep->bandwidth;

	return EOK;
}

/** Release bandwidth reserved by the given endpoint.
 */
static int usb2_bus_unregister_ep(endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(ep->device->bus);
	assert(ep);

	list_remove(&ep->link);

	bus->free_bw += ep->bandwidth;
	endpoint_del_ref(ep);

	return EOK;
}

static int usb2_bus_reset_toggle(bus_t *bus_base, usb_target_t target, toggle_reset_mode_t mode)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);

	if (!usb_target_is_valid(target))
		return EINVAL;

	if (mode == RESET_NONE)
		return EOK;

	int ret = ENOENT;

	list_foreach(*get_list(bus, target.address), link, endpoint_t, ep) {
		assert(ep->device->address == target.address);

		if (mode == RESET_ALL || ep->endpoint == target.endpoint) {
			endpoint_toggle_set(ep, 0);
			ret = EOK;
		}
	}
	return ret;
}

static int usb2_bus_register_default_address(bus_t *bus_base, usb_speed_t speed)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);
	usb_address_t addr = USB_ADDRESS_DEFAULT;
	return request_address(bus, &addr, true, speed);
}

static int usb2_bus_release_default_address(bus_t *bus_base)
{
	usb2_bus_t *bus = bus_to_usb2_bus(bus_base);
	return release_address(bus, USB_ADDRESS_DEFAULT);
}

const bus_ops_t usb2_bus_ops = {
	.reserve_default_address = usb2_bus_register_default_address,
	.release_default_address = usb2_bus_release_default_address,
	.reset_toggle = usb2_bus_reset_toggle,
	.device_enumerate = usb2_bus_device_enumerate,
	.device_find_endpoint = usb2_bus_find_ep,
	.endpoint_create= usb2_bus_create_ep,
	.endpoint_register= usb2_bus_register_ep,
	.endpoint_unregister= usb2_bus_unregister_ep,
};

/** Initialize to default state.
 *
 * @param bus usb_bus structure, non-null.
 * @param available_bandwidth Size of the bandwidth pool.
 * @param bw_count function to use to calculate endpoint bw requirements.
 * @return Error code.
 */
int usb2_bus_init(usb2_bus_t *bus, hcd_t *hcd, size_t available_bandwidth)
{
	assert(bus);

	bus_init(&bus->base, hcd, sizeof(device_t));
	bus->base.ops = &usb2_bus_ops;

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
