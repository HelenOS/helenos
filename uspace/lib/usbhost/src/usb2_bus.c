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
 *
 * A bus_t implementation for USB 2 and lower. Implements USB 2 enumeration and
 * configurable bandwidth counting.
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
#include <usb/host/utility.h>
#include <usb/usb.h>

#include "endpoint.h"
#include "ddf_helpers.h"

#include "usb2_bus.h"

/**
 * Ops receive generic bus_t pointer.
 */
static inline usb2_bus_t *bus_to_usb2_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (usb2_bus_t *) bus_base;
}

/**
 * Request a new address. A free address is found and marked as occupied.
 *
 * There's no need to synchronize this method, because it is called only with
 * default address reserved.
 *
 * @param bus usb_device_manager
 * @param addr Pointer to requested address value, place to store new address
 */
static int request_address(usb2_bus_t *bus, usb_address_t *addr)
{
	// Find a free address
	usb_address_t new_address = bus->last_address;
	do {
		new_address = (new_address + 1) % USB_ADDRESS_COUNT;
		if (new_address == USB_ADDRESS_DEFAULT)
			new_address = 1;
		if (new_address == bus->last_address)
			return ENOSPC;
	} while (bus->address_occupied[new_address]);
	bus->last_address = new_address;

	*addr = new_address;
	bus->address_occupied[*addr] = true;

	return EOK;
}

/**
 * Mark address as free.
 */
static void release_address(usb2_bus_t *bus, usb_address_t address)
{
	bus->address_occupied[address] = false;
}

static const usb_target_t usb2_default_target = {{
	.address = USB_ADDRESS_DEFAULT,
	.endpoint = 0,
}};

/**
 * Transition the device to the addressed state.
 *
 * Reserve address, configure the control EP, issue a SET_ADDRESS command.
 * Configure the device with the new address,
 */
static int address_device(device_t *dev)
{
	int err;

	usb2_bus_t *bus = (usb2_bus_t *) dev->bus;

	/* The default address is currently reserved for this device */
	dev->address = USB_ADDRESS_DEFAULT;

	/** Reserve address early, we want pretty log messages */
	usb_address_t address = USB_ADDRESS_DEFAULT;
	if ((err = request_address(bus, &address))) {
		usb_log_error("Failed to reserve new address: %s.",
		    str_error(err));
		return err;
	}
	usb_log_debug("Device(%d): Reserved new address.", address);

	/* Add default pipe on default address */
	usb_log_debug("Device(%d): Adding default target (0:0)", address);

	usb_endpoint_descriptors_t ep0_desc = {
	    .endpoint.max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE,
	};
	endpoint_t *default_ep;
	err = bus_endpoint_add(dev, &ep0_desc, &default_ep);
	if (err != EOK) {
		usb_log_error("Device(%d): Failed to add default target: %s.",
		    address, str_error(err));
		goto err_address;
	}

	if ((err = hc_get_ep0_max_packet_size(&ep0_desc.endpoint.max_packet_size, &bus->base, dev)))
		goto err_address;

	/* Set new address */
	const usb_device_request_setup_packet_t set_address = SET_ADDRESS(address);

	usb_log_debug("Device(%d): Setting USB address.", address);
	err = bus_device_send_batch_sync(dev, usb2_default_target, USB_DIRECTION_OUT,
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

	dev->address = address;

	/* Register EP on the new address */
	usb_log_debug("Device(%d): Registering control EP.", address);
	err = bus_endpoint_add(dev, &ep0_desc, NULL);
	if (err != EOK) {
		usb_log_error("Device(%d): Failed to register EP0: %s",
		    address, str_error(err));
		goto err_address;
	}

	return EOK;

err_default_control_ep:
	bus_endpoint_remove(default_ep);
err_address:
	release_address(bus, address);
	return err;
}

/**
 * Enumerate a USB device. Move it to the addressed state, then explore it
 * to create a DDF function node with proper characteristics.
 */
static int usb2_bus_device_enumerate(device_t *dev)
{
	int err;
	usb2_bus_t *bus = bus_to_usb2_bus(dev->bus);

	usb_log_debug("Found new %s speed USB device.", usb_str_speed(dev->speed));

	/* Assign an address to the device */
	if ((err = address_device(dev))) {
		usb_log_error("Failed to setup address of the new device: %s", str_error(err));
		return err;
	}

	/* Read the device descriptor, derive the match ids */
	if ((err = hc_device_explore(dev))) {
		usb_log_error("Device(%d): Failed to explore device: %s", dev->address, str_error(err));
		release_address(bus, dev->address);
		return err;
	}

	return EOK;
}

/**
 * Register an endpoint to the bus. Reserves bandwidth.
 */
static int usb2_bus_register_ep(endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(ep->device->bus);
	assert(fibril_mutex_is_locked(&ep->device->guard));
	assert(ep);

	/* Check for available bandwidth */
	if (ep->bandwidth > bus->free_bw)
		return ENOSPC;

	bus->free_bw -= ep->bandwidth;

	return EOK;
}

/**
 * Release bandwidth reserved by the given endpoint.
 */
static void usb2_bus_unregister_ep(endpoint_t *ep)
{
	usb2_bus_t *bus = bus_to_usb2_bus(ep->device->bus);
	assert(ep);

	bus->free_bw += ep->bandwidth;
}

const bus_ops_t usb2_bus_ops = {
	.device_enumerate = usb2_bus_device_enumerate,
	.endpoint_register = usb2_bus_register_ep,
	.endpoint_unregister = usb2_bus_unregister_ep,
};

/** Initialize to default state.
 *
 * @param bus usb_bus structure, non-null.
 * @param available_bandwidth Size of the bandwidth pool.
 */
void usb2_bus_init(usb2_bus_t *bus, size_t available_bandwidth)
{
	assert(bus);

	bus_init(&bus->base, sizeof(device_t));
	bus->base.ops = &usb2_bus_ops;

	bus->free_bw = available_bandwidth;

	/*
	 * The first address allocated is for the roothub. This way, its
	 * address will be 127, and the first connected USB device will have
	 * address 1.
	 */
	bus->last_address = USB_ADDRESS_COUNT - 2;
}

/**
 * @}
 */
