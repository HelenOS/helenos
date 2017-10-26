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

#include <usb/host/utils/malloc32.h>
#include <usb/host/ddf_helpers.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <macros.h>
#include <stdbool.h>

#include "hc.h"
#include "bus.h"
#include "endpoint.h"
#include "transfers.h"


/* FIXME Are these really static? Older HCs fetch it from descriptor. */
/* FIXME Add USB3 options, if applicable. */
static const usb_endpoint_desc_t ep0_desc = {
	.endpoint_no = 0,
	.direction = USB_DIRECTION_BOTH,
	.transfer_type = USB_TRANSFER_CONTROL,
	.max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE,
	.packets = 1,
};

static int prepare_endpoint(xhci_endpoint_t *ep, const usb_endpoint_desc_t *desc)
{
	/* Extract information from endpoint_desc */
	ep->base.endpoint = desc->endpoint_no;
	ep->base.direction = desc->direction;
	ep->base.transfer_type = desc->transfer_type;
	ep->base.max_packet_size = desc->max_packet_size;
	ep->base.packets = desc->packets;
	ep->max_streams = desc->usb3.max_streams;
	ep->max_burst = desc->usb3.max_burst;
	// TODO add this property to usb_endpoint_desc_t and fetch it from ss companion desc
	ep->mult = 0;

	return xhci_endpoint_alloc_transfer_ds(ep);
}

static endpoint_t *create_endpoint(bus_t *base);

static int address_device(xhci_hc_t *hc, xhci_device_t *dev)
{
	int err;

	/* Enable new slot. */
	if ((err = hc_enable_slot(hc, &dev->slot_id)) != EOK)
		return err;
	usb_log_debug2("Obtained slot ID: %u.\n", dev->slot_id);

	/* Create and configure control endpoint. */
	endpoint_t *ep0_base = create_endpoint(&hc->bus.base);
	if (!ep0_base)
		goto err_slot;

	/* Temporary reference */
	endpoint_add_ref(ep0_base);

	ep0_base->device = &dev->base;
	xhci_endpoint_t *ep0 = xhci_endpoint_get(ep0_base);

	if ((err = prepare_endpoint(ep0, &ep0_desc)))
		goto err_ep;

	/* Address device */
	if ((err = hc_address_device(hc, dev, ep0)))
		goto err_prepared_ep;

	/* Register EP0, passing Temporary reference */
	dev->endpoints[0] = ep0;

	return EOK;

err_prepared_ep:
	xhci_endpoint_free_transfer_ds(ep0);
err_ep:
	endpoint_del_ref(ep0_base);
err_slot:
	hc_disable_slot(hc, dev->slot_id);
	return err;
}

int xhci_bus_enumerate_device(xhci_bus_t *bus, xhci_hc_t *hc, device_t *dev)
{
	int err;
	xhci_device_t *xhci_dev = xhci_device_get(dev);

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

	/* Calculate route string */
	xhci_device_t *xhci_hub = xhci_device_get(dev->hub);
	xhci_dev->tier = xhci_hub->tier + 1;
	xhci_dev->route_str = xhci_hub->route_str;

	/* Roothub port is not part of the route string */
	if (xhci_dev->tier >= 2) {
		const unsigned offset = 4 * (xhci_dev->tier - 2);
		xhci_dev->route_str |= (dev->port & 0xf) << offset;
		xhci_dev->rh_port = xhci_hub->rh_port;
	}

	fibril_mutex_lock(&bus->base.guard);
	/* Assign an address to the device */
	if ((err = address_device(hc, xhci_dev))) {
		usb_log_error("Failed to setup address of the new device: %s", str_error(err));
		return err;
	}

	// TODO: Fetch descriptor of EP0 and reconfigure it accordingly
	assert(xhci_dev->endpoints[0]);

	assert(bus->devices_by_slot[xhci_dev->slot_id] == NULL);
	bus->devices_by_slot[xhci_dev->slot_id] = xhci_dev;
	fibril_mutex_unlock(&bus->base.guard);

	/* Read the device descriptor, derive the match ids */
	if ((err = hcd_ddf_device_explore(hc->hcd, dev))) {
		usb_log_error("Device(%d): Failed to explore device: %s", dev->address, str_error(err));
		goto err_address;
	}

	return EOK;

err_address:
	bus_release_address(&bus->base, dev->address);
	return err;
}

static int unregister_endpoint(bus_t *, endpoint_t *);

int xhci_bus_remove_device(xhci_bus_t *bus, xhci_hc_t *hc, device_t *dev)
{
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* Unregister remaining endpoints. */
	for (unsigned i = 0; i < ARRAY_SIZE(xhci_dev->endpoints); ++i) {
		if (!xhci_dev->endpoints[i])
			continue;

		const int err = unregister_endpoint(&bus->base, &xhci_dev->endpoints[i]->base);
		if (err)
			usb_log_warning("Failed to unregister EP (%u:%u): %s", dev->address, i, str_error(err));
	}

	// XXX: Ugly here. Move to device_destroy at endpoint.c?
	free32(xhci_dev->dev_ctx);
	hc->dcbaa[xhci_dev->slot_id] = 0;
	return EOK;
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

	xhci_endpoint_t *ep = calloc(1, sizeof(xhci_endpoint_t));
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

static int register_endpoint(bus_t *bus_base, endpoint_t *ep, const usb_endpoint_desc_t *desc)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	assert(ep->device);

	xhci_device_t *xhci_dev = xhci_device_get(ep->device);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);

	if ((err = prepare_endpoint(xhci_ep, desc)))
		return err;

	usb_log_info("Endpoint(%d:%d) registered to XHCI bus.", ep->device->address, ep->endpoint);
	return xhci_device_add_endpoint(bus->hc, xhci_dev, xhci_ep);
}

static int unregister_endpoint(bus_t *bus_base, endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	usb_log_info("Endpoint(%d:%d) unregistered from XHCI bus.", ep->device->address, ep->endpoint);

	xhci_device_t *xhci_dev = xhci_device_get(ep->device);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);
	const int res = xhci_device_remove_endpoint(bus->hc, xhci_dev, xhci_ep);
	if (res != EOK)
		return res;

	return EOK;
}

static endpoint_t* find_endpoint(bus_t *bus_base, device_t *dev_base, usb_target_t target, usb_direction_t direction)
{
	xhci_device_t *dev = xhci_device_get(dev_base);

	xhci_endpoint_t *ep = xhci_device_get_endpoint(dev, target.endpoint);
	if (!ep)
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

static int request_address(bus_t *bus_base, usb_address_t *addr, bool strict, usb_speed_t speed)
{
	assert(addr);

	if (*addr != USB_ADDRESS_DEFAULT)
		/* xHCI does not allow software to assign addresses. */
		return ENOTSUP;

	assert(strict);

	xhci_bus_t *xhci_bus = bus_to_xhci_bus(bus_base);

	if (xhci_bus->default_address_speed != USB_SPEED_MAX)
		/* Already allocated */
		return ENOENT;

	xhci_bus->default_address_speed = speed;
	return EOK;
}

static int release_address(bus_t *bus_base, usb_address_t addr)
{
	if (addr != USB_ADDRESS_DEFAULT)
		return ENOTSUP;

	xhci_bus_t *xhci_bus = bus_to_xhci_bus(bus_base);

	xhci_bus->default_address_speed = USB_SPEED_MAX;
	return EOK;
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
	.unregister_endpoint = unregister_endpoint,
	.find_endpoint = find_endpoint,

	.request_address = request_address,
	.release_address = release_address,
	.reset_toggle = reset_toggle,

	.count_bw = count_bw,

	.endpoint_get_toggle = endpoint_get_toggle,
	.endpoint_set_toggle = endpoint_set_toggle,

	.create_batch = create_batch,
	.destroy_batch = destroy_batch,
};

int xhci_bus_init(xhci_bus_t *bus, xhci_hc_t *hc)
{
	assert(bus);

	bus_init(&bus->base, sizeof(xhci_device_t));

	bus->devices_by_slot = calloc(hc->max_slots, sizeof(xhci_device_t *));
	if (!bus->devices_by_slot)
		return ENOMEM;

	bus->hc = hc;
	bus->base.ops = xhci_bus_ops;
	bus->default_address_speed = USB_SPEED_MAX;
	return EOK;
}

void xhci_bus_fini(xhci_bus_t *bus)
{

}

/**
 * @}
 */
