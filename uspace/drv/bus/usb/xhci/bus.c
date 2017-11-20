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


static const usb_endpoint_desc_t ep0_initial_desc = {
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
	ep->mult = desc->usb3.mult;

	if (ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		if (ep->base.device->speed <= USB_SPEED_HIGH) {
			ep->isoch_max_size = desc->max_packet_size * (desc->packets + 1);
		}
		else if (ep->base.device->speed == USB_SPEED_SUPER) {
			ep->isoch_max_size = desc->usb3.bytes_per_interval;
		}
		/* Technically there could be superspeed plus too. */

		/* Allocate and setup isochronous-specific structures. */
		ep->isoch_enqueue = 0;
		ep->isoch_dequeue = XHCI_ISOCH_BUFFER_COUNT - 1;
		ep->isoch_started = false;
	}

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

	xhci_endpoint_t *ep0 = xhci_endpoint_get(ep0_base);

	if ((err = prepare_endpoint(ep0, &ep0_initial_desc)))
		goto err_ep;

	/* Register EP0 */
	if ((err = xhci_device_add_endpoint(dev, ep0)))
		goto err_prepared;

	/* Address device */
	if ((err = hc_address_device(hc, dev, ep0)))
		goto err_added;

	/* Temporary reference */
	endpoint_del_ref(ep0_base);
	return EOK;

err_added:
	xhci_device_remove_endpoint(ep0);
err_prepared:
	xhci_endpoint_free_transfer_ds(ep0);
err_ep:
	/* Temporary reference */
	endpoint_del_ref(ep0_base);
err_slot:
	hc_disable_slot(hc, dev);
	return err;
}

static int setup_ep0_packet_size(xhci_hc_t *hc, xhci_device_t *dev)
{
	int err;

	uint16_t max_packet_size;
	if ((err = hcd_get_ep0_max_packet_size(&max_packet_size, hc->hcd, &dev->base)))
		return err;

	xhci_endpoint_t *ep0 = dev->endpoints[0];
	assert(ep0);

	if (ep0->base.max_packet_size == max_packet_size)
		return EOK;

	ep0->base.max_packet_size = max_packet_size;

	xhci_ep_ctx_t ep_ctx;
	xhci_setup_endpoint_context(ep0, &ep_ctx);

	if ((err = hc_update_endpoint(hc, dev->slot_id, 0, &ep_ctx)))
		return err;

	return EOK;
}

int xhci_bus_enumerate_device(xhci_bus_t *bus, xhci_hc_t *hc, device_t *dev)
{
	int err;
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	hcd_setup_device_tt(dev);

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

	if ((err = setup_ep0_packet_size(hc, xhci_dev))) {
		usb_log_error("Failed to setup control endpoint of the new device: %s", str_error(err));
		goto err_address;
	}

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
	int err;
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* Block creation of new endpoints and transfers. */
	usb_log_debug2("Device " XHCI_DEV_FMT " going offline.", XHCI_DEV_ARGS(*xhci_dev));
	fibril_mutex_lock(&dev->guard);
	xhci_dev->online = false;
	fibril_mutex_unlock(&dev->guard);

	/* Abort running transfers. */
	usb_log_debug2("Aborting all active transfers to device " XHCI_DEV_FMT ".", XHCI_DEV_ARGS(*xhci_dev));
	for (size_t i = 0; i < ARRAY_SIZE(xhci_dev->endpoints); ++i) {
		xhci_endpoint_t *ep = xhci_dev->endpoints[i];
		if (!ep)
			continue;

		endpoint_abort(&ep->base);
	}

	/* TODO: Figure out how to handle errors here. So far, they are reported and skipped. */

	/* Make DDF (and all drivers) forget about the device. */
	if ((err = ddf_fun_unbind(dev->fun))) {
		usb_log_warning("Failed to unbind DDF function of device " XHCI_DEV_FMT ": %s",
		    XHCI_DEV_ARGS(*xhci_dev), str_error(err));
	}

	/* Disable the slot, dropping all endpoints. */
	const uint32_t slot_id = xhci_dev->slot_id;
	if ((err = hc_disable_slot(hc, xhci_dev))) {
		usb_log_warning("Failed to disable slot of device " XHCI_DEV_FMT ": %s",
		    XHCI_DEV_ARGS(*xhci_dev), str_error(err));
	}

	bus->devices_by_slot[slot_id] = NULL;

	/* Unregister remaining endpoints, freeing memory. */
	for (unsigned i = 0; i < ARRAY_SIZE(xhci_dev->endpoints); ++i) {
		if (!xhci_dev->endpoints[i])
			continue;

		if ((err = unregister_endpoint(&bus->base, &xhci_dev->endpoints[i]->base))) {
			usb_log_warning("Failed to unregister endpoint " XHCI_EP_FMT ": %s",
			    XHCI_EP_ARGS(*xhci_dev->endpoints[i]), str_error(err));
		}
	}

	/* Destroy DDF device. */
	/* XXX: Not a good idea, this method should not destroy devices. */
	hcd_ddf_device_destroy(dev);

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

static int online_device(bus_t *bus_base, hcd_t *hcd, device_t *dev_base)
{
	int err;

	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Transition the device from the Addressed to the Configured state. */
	if ((err = hc_configure_device(hc, dev->slot_id))) {
		usb_log_warning("Failed to configure device " XHCI_DEV_FMT ".", XHCI_DEV_ARGS(*dev));
	}

	/* Block creation of new endpoints and transfers. */
	usb_log_debug2("Device " XHCI_DEV_FMT " going online.", XHCI_DEV_ARGS(*dev));
	fibril_mutex_lock(&dev_base->guard);
	dev->online = true;
	fibril_mutex_unlock(&dev_base->guard);

	if ((err = ddf_fun_online(dev_base->fun))) {
		return err;
	}

	return EOK;
}

static int offline_device(bus_t *bus_base, hcd_t *hcd, device_t *dev_base)
{
	int err;

	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Tear down all drivers working with the device. */
	if ((err = ddf_fun_offline(dev_base->fun))) {
		return err;
	}

	/* Block creation of new endpoints and transfers. */
	usb_log_debug2("Device " XHCI_DEV_FMT " going offline.", XHCI_DEV_ARGS(*dev));
	fibril_mutex_lock(&dev_base->guard);
	dev->online = false;
	fibril_mutex_unlock(&dev_base->guard);

	/* We will need the endpoint array later for DS deallocation. */
	xhci_endpoint_t *endpoints[ARRAY_SIZE(dev->endpoints)];
	memcpy(endpoints, dev->endpoints, sizeof(dev->endpoints));

	/* Remove all endpoints except zero. */
	for (unsigned i = 1; i < ARRAY_SIZE(endpoints); ++i) {
		if (!endpoints[i])
			continue;

		/* FIXME: Asserting here that the endpoint is not active. If not, EBUSY? */

		xhci_device_remove_endpoint(endpoints[i]);
	}

	/* Issue one HC command to simultaneously drop all endpoints except zero. */
	if ((err = hc_deconfigure_device(hc, dev->slot_id))) {
		usb_log_warning("Failed to deconfigure device " XHCI_DEV_FMT ".",
		    XHCI_DEV_ARGS(*dev));
	}

	/* Tear down TRB ring / PSA. */
	for (unsigned i = 1; i < ARRAY_SIZE(endpoints); ++i) {
		if (!endpoints[i])
			continue;

		xhci_endpoint_free_transfer_ds(endpoints[i]);
	}

	/* FIXME: What happens to unregistered endpoints now? Destroy them? */

	return EOK;
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

static int register_endpoint(bus_t *bus_base, device_t *device, endpoint_t *ep_base, const usb_endpoint_desc_t *desc)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);

	xhci_device_t *dev = xhci_device_get(device);

	if ((err = prepare_endpoint(ep, desc)))
		return err;

	if ((err = xhci_device_add_endpoint(dev, ep)))
		goto err_prepared;

	usb_log_info("Endpoint " XHCI_EP_FMT " registered to XHCI bus.", XHCI_EP_ARGS(*ep));

	xhci_ep_ctx_t ep_ctx;
	xhci_setup_endpoint_context(ep, &ep_ctx);

	if ((err = hc_add_endpoint(bus->hc, dev->slot_id, xhci_endpoint_index(ep), &ep_ctx)))
		goto err_added;

	return EOK;

err_added:
	xhci_device_remove_endpoint(ep);
err_prepared:
	xhci_endpoint_free_transfer_ds(ep);
	return err;
}

static int unregister_endpoint(bus_t *bus_base, endpoint_t *ep_base)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(bus_base);
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);
	xhci_device_t *dev = xhci_device_get(ep_base->device);

	usb_log_info("Endpoint " XHCI_EP_FMT " unregistered from XHCI bus.", XHCI_EP_ARGS(*ep));

	xhci_device_remove_endpoint(ep);

	/* If device slot is still available, drop the endpoint. */
	if (dev->slot_id) {
		if ((err = hc_drop_endpoint(bus->hc, dev->slot_id, xhci_endpoint_index(ep)))) {
			usb_log_error("Failed to drop endpoint " XHCI_EP_FMT ": %s", XHCI_EP_ARGS(*ep), str_error(err));
		}
	} else {
		usb_log_debug("Not going to drop endpoint " XHCI_EP_FMT " because"
		    " the slot has already been disabled.", XHCI_EP_ARGS(*ep));
	}

	/* Tear down TRB ring / PSA. */
	xhci_endpoint_free_transfer_ds(ep);

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

static int reset_toggle(bus_t *bus_base, usb_target_t target, toggle_reset_mode_t mode)
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
#define BIND_OP(op) .op = op,
	BIND_OP(enumerate_device)
	BIND_OP(remove_device)

	BIND_OP(online_device)
	BIND_OP(offline_device)

	BIND_OP(create_endpoint)
	BIND_OP(destroy_endpoint)

	BIND_OP(register_endpoint)
	BIND_OP(unregister_endpoint)
	BIND_OP(find_endpoint)

	BIND_OP(request_address)
	BIND_OP(release_address)
	BIND_OP(reset_toggle)

	BIND_OP(count_bw)

	BIND_OP(endpoint_get_toggle)
	BIND_OP(endpoint_set_toggle)

	BIND_OP(create_batch)
	BIND_OP(destroy_batch)
#undef BIND_OP
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
