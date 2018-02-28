/*
 * Copyright (c) 2018 Ondrej Hlavaty, Jan Hrach
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

#include <usb/host/utility.h>
#include <usb/host/ddf_helpers.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>
#include <usb/host/utility.h>
#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usb/descriptor.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <macros.h>
#include <stdbool.h>

#include "hc.h"
#include "bus.h"
#include "endpoint.h"
#include "hw_struct/context.h"

#include "device.h"

/**
 * Initial descriptor used for control endpoint 0,
 * before more configuration is retrieved.
 */
static const usb_endpoint_descriptors_t ep0_initial_desc = {
	.endpoint.max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE,
};

/**
 * Assign address and control endpoint to a new XHCI device. Once this function
 * successfully returns, the device is online.
 *
 * @param[in] bus XHCI bus, in which the address is assigned.
 * @param[in] dev New device to address and configure./e
 * @return Error code.
 */
static errno_t address_device(xhci_device_t *dev)
{
	errno_t err;

	/* Enable new slot. */
	if ((err = hc_enable_slot(dev)) != EOK)
		return err;
	usb_log_debug("Obtained slot ID: %u.", dev->slot_id);

	/* Temporary reference */
	endpoint_t *ep0_base;
	if ((err = bus_endpoint_add(&dev->base, &ep0_initial_desc, &ep0_base)))
		goto err_slot;

	usb_log_debug("Looking up new device initial MPS: %s",
	    usb_str_speed(dev->base.speed));
	ep0_base->max_packet_size = hc_get_ep0_initial_mps(dev->base.speed);

	/* Address device */
	if ((err = hc_address_device(dev)))
		goto err_added;

	/* Temporary reference */
	endpoint_del_ref(ep0_base);

	return EOK;

err_added:
	bus_endpoint_remove(ep0_base);
	/* Temporary reference */
	endpoint_del_ref(ep0_base);
err_slot:
	hc_disable_slot(dev);
	return err;
}

/**
 * Retrieve and set maximum packet size for endpoint zero of a XHCI device.
 *
 * @param[in] hc Host controller, which manages the device.
 * @param[in] dev Device with operational endpoint zero.
 * @return Error code.
 */
static errno_t setup_ep0_packet_size(xhci_hc_t *hc, xhci_device_t *dev)
{
	errno_t err;

	uint16_t max_packet_size;
	if ((err = hc_get_ep0_max_packet_size(&max_packet_size, &dev->base)))
		return err;

	xhci_endpoint_t *ep0 = xhci_endpoint_get(dev->base.endpoints[0]);
	assert(ep0);

	if (ep0->base.max_packet_size == max_packet_size)
		return EOK;

	ep0->base.max_packet_size = max_packet_size;
	ep0->base.max_transfer_size = max_packet_size * ep0->base.packets_per_uframe;

	if ((err = hc_update_endpoint(ep0)))
		return err;

	return EOK;
}

/**
 * Check whether the device is a hub and if so, fill its characterstics.
 *
 * If this fails, it does not necessarily mean the device is unusable.
 * Just the TT will not work correctly.
 */
static errno_t setup_hub(xhci_device_t *dev, usb_standard_device_descriptor_t *desc)
{
	if (desc->device_class != USB_CLASS_HUB)
		return EOK;

	usb_hub_descriptor_header_t hub_desc = { 0 };
	const errno_t err = hc_get_hub_desc(&dev->base, &hub_desc);
	if (err)
		return err;

	dev->is_hub = 1;
	dev->num_ports = hub_desc.port_count;

	if (dev->base.speed == USB_SPEED_HIGH) {
		dev->tt_think_time = 8 +
			8  * !!(hub_desc.characteristics & HUB_CHAR_TT_THINK_8) +
			16 * !!(hub_desc.characteristics & HUB_CHAR_TT_THINK_16);
	}

	usb_log_debug("Device(%u): recognised USB hub with %u ports",
	    dev->base.address, dev->num_ports);
	return EOK;
}

/**
 * Respond to a new device on the XHCI bus. Address it, negotiate packet size
 * and retrieve USB descriptors.
 *
 * @param[in] bus XHCI bus, where the new device emerged.
 * @param[in] dev XHCI device, which has appeared on the bus.
 *
 * @return Error code.
 */
errno_t xhci_device_enumerate(device_t *dev)
{
	errno_t err;
	xhci_bus_t *bus = bus_to_xhci_bus(dev->bus);
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* Calculate route string */
	xhci_device_t *xhci_hub = xhci_device_get(dev->hub);
	xhci_dev->route_str = xhci_hub->route_str;

	/* Roothub port is not part of the route string */
	if (dev->tier >= 2) {
		const unsigned offset = 4 * (dev->tier - 2);
		xhci_dev->route_str |= (dev->port & 0xf) << offset;
		xhci_dev->rh_port = xhci_hub->rh_port;
	}

	int retries = 3;
	do {
		/* Assign an address to the device */
		err = address_device(xhci_dev);
	} while (err == ESTALL && --retries > 0);

	if (err) {
		usb_log_error("Failed to setup address of the new device: %s",
		    str_error(err));
		return err;
	}

	/* Setup EP0 might already need to issue a transfer. */
	fibril_mutex_lock(&bus->base.guard);
	assert(bus->devices_by_slot[xhci_dev->slot_id] == NULL);
	bus->devices_by_slot[xhci_dev->slot_id] = xhci_dev;
	fibril_mutex_unlock(&bus->base.guard);

	if ((err = setup_ep0_packet_size(bus->hc, xhci_dev))) {
		usb_log_error("Failed to setup control endpoint "
		    "of the new device: %s", str_error(err));
		goto err_address;
	}

	usb_standard_device_descriptor_t desc = { 0 };

	if ((err = hc_get_device_desc(dev, &desc))) {
		usb_log_error("Device(%d): failed to get device "
		   "descriptor: %s", dev->address, str_error(err));
		goto err_address;
	}

	if ((err = setup_hub(xhci_dev, &desc)))
		usb_log_warning("Device(%d): failed to setup hub "
		    "characteristics: %s.  Continuing anyway.",
		    dev->address, str_error(err));

	if ((err = hcd_ddf_setup_match_ids(dev, &desc))) {
		usb_log_error("Device(%d): failed to setup match IDs: %s",
		    dev->address, str_error(err));
		goto err_address;
	}

	return EOK;

err_address:
	bus_endpoint_remove(xhci_dev->base.endpoints[0]);
	bus->devices_by_slot[xhci_dev->slot_id] = NULL;
	hc_disable_slot(xhci_dev);
	return err;
}

/**
 * Remove device from XHCI bus. Transition it to the offline state, abort all
 * ongoing transfers and unregister all of its endpoints.
 *
 * Bus callback.
 *
 * @param[in] bus XHCI bus, from which the device is removed.
 * @param[in] dev XHCI device, which is removed from the bus.
 * @return Error code.
 */
void xhci_device_gone(device_t *dev)
{
	errno_t err;
	xhci_bus_t *bus = bus_to_xhci_bus(dev->bus);
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* Disable the slot, dropping all endpoints. */
	const uint32_t slot_id = xhci_dev->slot_id;
	if ((err = hc_disable_slot(xhci_dev))) {
		usb_log_warning("Failed to disable slot of device " XHCI_DEV_FMT
		    ": %s", XHCI_DEV_ARGS(*xhci_dev), str_error(err));
	}

	bus->devices_by_slot[slot_id] = NULL;
}

/**
 * Reverts things device_offline did, getting the device back up.
 *
 * Bus callback.
 */
errno_t xhci_device_online(device_t *dev_base)
{
	errno_t err;

	xhci_bus_t *bus = bus_to_xhci_bus(dev_base->bus);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Transition the device from the Addressed to the Configured state. */
	if ((err = hc_configure_device(dev))) {
		usb_log_warning("Failed to configure device " XHCI_DEV_FMT ".",
		    XHCI_DEV_ARGS(*dev));
		return err;
	}

	return EOK;
}

/**
 * Make given device offline. Offline the DDF function, tear down all
 * endpoints, issue Deconfigure Device command to xHC.
 *
 * Bus callback.
 */
void xhci_device_offline(device_t *dev_base)
{
	errno_t err;

	xhci_bus_t *bus = bus_to_xhci_bus(dev_base->bus);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Issue one HC command to simultaneously drop all endpoints except zero. */
	if ((err = hc_deconfigure_device(dev))) {
		usb_log_warning("Failed to deconfigure device "
		    XHCI_DEV_FMT ".", XHCI_DEV_ARGS(*dev));
	}
}

/**
 * Fill a slot context that is part of an Input Context with appropriate
 * values.
 *
 * @param ctx Slot context, zeroed out.
 */
void xhci_setup_slot_context(xhci_device_t *dev, xhci_slot_ctx_t *ctx)
{
	/* Initialize slot_ctx according to section 4.3.3 point 3. */
	XHCI_SLOT_ROOT_HUB_PORT_SET(*ctx, dev->rh_port);
	XHCI_SLOT_ROUTE_STRING_SET(*ctx, dev->route_str);
	XHCI_SLOT_SPEED_SET(*ctx, hc_speed_to_psiv(dev->base.speed));

	/*
	 * Note: This function is used even before this flag can be set, to
	 *       issue the address device command. It is OK, because these
	 *       flags are not required to be valid for that command.
	 */
	if (dev->is_hub) {
		XHCI_SLOT_HUB_SET(*ctx, 1);
		XHCI_SLOT_NUM_PORTS_SET(*ctx, dev->num_ports);
		XHCI_SLOT_TT_THINK_TIME_SET(*ctx, dev->tt_think_time);
		XHCI_SLOT_MTT_SET(*ctx, 0); // MTT not supported yet
	}

	/* Setup Transaction Translation. TODO: Test this with HS hub. */
	if (dev->base.tt.dev != NULL) {
		xhci_device_t *hub = xhci_device_get(dev->base.tt.dev);
		XHCI_SLOT_TT_HUB_SLOT_ID_SET(*ctx, hub->slot_id);
		XHCI_SLOT_TT_HUB_PORT_SET(*ctx, dev->base.tt.port);
	}

	/*
	 * As we always allocate space for whole input context, we can set this
	 * to maximum. The only exception being Address Device command, which
	 * explicitly requires this to be se to 1.
	 */
	XHCI_SLOT_CTX_ENTRIES_SET(*ctx, 31);
}


/**
 * @}
 */
