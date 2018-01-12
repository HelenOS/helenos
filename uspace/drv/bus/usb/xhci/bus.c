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
#include "transfers.h"


/** Initial descriptor used for control endpoint 0 before more configuration is retrieved. */
static const usb_endpoint_descriptors_t ep0_initial_desc = {
	.endpoint.max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE,
};

static endpoint_t *endpoint_create(device_t *, const usb_endpoint_descriptors_t *);

/**
 * Assign address and control endpoint to a new XHCI device. Once this function
 * successfully returns, the device is online.
 *
 * @param[in] bus XHCI bus, in which the address is assigned.
 * @param[in] dev New device to address and configure./e
 * @return Error code.
 */
static int address_device(xhci_bus_t *bus, xhci_device_t *dev)
{
	int err;

	/* Enable new slot. */
	if ((err = hc_enable_slot(bus->hc, &dev->slot_id)) != EOK)
		return err;
	usb_log_debug2("Obtained slot ID: %u.\n", dev->slot_id);

	/* Create and configure control endpoint. */
	endpoint_t *ep0_base = endpoint_create(&dev->base, &ep0_initial_desc);
	if (!ep0_base)
		goto err_slot;

	/* Bus reference */
	endpoint_add_ref(ep0_base);
	dev->base.endpoints[0] = ep0_base;

	xhci_endpoint_t *ep0 = xhci_endpoint_get(ep0_base);

	/* Address device */
	if ((err = hc_address_device(bus->hc, dev, ep0)))
		goto err_added;

	return EOK;

err_added:
	/* Bus reference */
	endpoint_del_ref(ep0_base);
	dev->base.endpoints[0] = NULL;
err_slot:
	hc_disable_slot(bus->hc, dev);
	return err;
}

/**
 * Retrieve and set maximum packet size for endpoint zero of a XHCI device.
 *
 * @param[in] hc Host controller, which manages the device.
 * @param[in] dev Device with operational endpoint zero.
 * @return Error code.
 */
static int setup_ep0_packet_size(xhci_hc_t *hc, xhci_device_t *dev)
{
	int err;

	uint16_t max_packet_size;
	if ((err = hcd_get_ep0_max_packet_size(&max_packet_size, (bus_t *) &hc->bus, &dev->base)))
		return err;

	xhci_endpoint_t *ep0 = xhci_device_get_endpoint(dev, 0);
	assert(ep0);

	if (ep0->base.max_packet_size == max_packet_size)
		return EOK;

	ep0->base.max_packet_size = max_packet_size;
	ep0->base.max_transfer_size = max_packet_size * ep0->base.packets_per_uframe;

	xhci_ep_ctx_t ep_ctx;
	xhci_setup_endpoint_context(ep0, &ep_ctx);

	if ((err = hc_update_endpoint(hc, dev->slot_id, 0, &ep_ctx)))
		return err;

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
static int device_enumerate(device_t *dev)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(dev->bus);
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

	int retries = 3;
	do {
		/* Assign an address to the device */
		err = address_device(bus, xhci_dev);
	} while (err == ESTALL && --retries > 0);

	if (err) {
		usb_log_error("Failed to setup address of the new device: %s", str_error(err));
		return err;
	}

	/* Setup EP0 might already need to issue a transfer. */
	fibril_mutex_lock(&bus->base.guard);
	assert(bus->devices_by_slot[xhci_dev->slot_id] == NULL);
	bus->devices_by_slot[xhci_dev->slot_id] = xhci_dev;
	fibril_mutex_unlock(&bus->base.guard);

	if ((err = setup_ep0_packet_size(bus->hc, xhci_dev))) {
		usb_log_error("Failed to setup control endpoint of the new device: %s", str_error(err));
		goto err_address;
	}

	/* Read the device descriptor, derive the match ids */
	if ((err = hcd_device_explore(dev))) {
		usb_log_error("Device(%d): Failed to explore device: %s", dev->address, str_error(err));
		goto err_address;
	}

	return EOK;

err_address:
	// TODO: deaddress device
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
static void device_gone(device_t *dev)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(dev->bus);
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* Disable the slot, dropping all endpoints. */
	const uint32_t slot_id = xhci_dev->slot_id;
	if ((err = hc_disable_slot(bus->hc, xhci_dev))) {
		usb_log_warning("Failed to disable slot of device " XHCI_DEV_FMT ": %s",
		    XHCI_DEV_ARGS(*xhci_dev), str_error(err));
	}

	bus->devices_by_slot[slot_id] = NULL;
}

/**
 * Reverts things device_offline did, getting the device back up.
 *
 * Bus callback.
 */
static int device_online(device_t *dev_base)
{
	int err;

	xhci_bus_t *bus = bus_to_xhci_bus(dev_base->bus);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Transition the device from the Addressed to the Configured state. */
	if ((err = hc_configure_device(bus->hc, dev->slot_id))) {
		usb_log_warning("Failed to configure device " XHCI_DEV_FMT ".", XHCI_DEV_ARGS(*dev));
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
static void device_offline(device_t *dev_base)
{
	int err;

	xhci_bus_t *bus = bus_to_xhci_bus(dev_base->bus);
	assert(bus);

	xhci_device_t *dev = xhci_device_get(dev_base);
	assert(dev);

	/* Issue one HC command to simultaneously drop all endpoints except zero. */
	if ((err = hc_deconfigure_device(bus->hc, dev->slot_id))) {
		usb_log_warning("Failed to deconfigure device " XHCI_DEV_FMT ".",
		    XHCI_DEV_ARGS(*dev));
	}
}

/**
 * Create a new xHCI endpoint structure.
 *
 * Bus callback.
 */
static endpoint_t *endpoint_create(device_t *dev, const usb_endpoint_descriptors_t *desc)
{
	const usb_transfer_type_t type = USB_ED_GET_TRANSFER_TYPE(desc->endpoint);

	xhci_endpoint_t *ep = calloc(1, sizeof(xhci_endpoint_t)
		+ (type == USB_TRANSFER_ISOCHRONOUS) * sizeof(*ep->isoch));
	if (!ep)
		return NULL;

	if (xhci_endpoint_init(ep, dev, desc)) {
		free(ep);
		return NULL;
	}

	return &ep->base;
}

/**
 * Destroy given xHCI endpoint structure.
 *
 * Bus callback.
 */
static void endpoint_destroy(endpoint_t *ep)
{
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);

	xhci_endpoint_fini(xhci_ep);
	free(xhci_ep);
}

/**
 * Register an andpoint to the xHC.
 *
 * Bus callback.
 */
static int endpoint_register(endpoint_t *ep_base)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(endpoint_get_bus(ep_base));
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);
	xhci_device_t *dev = xhci_device_get(ep_base->device);

	xhci_ep_ctx_t ep_ctx;
	xhci_setup_endpoint_context(ep, &ep_ctx);

	if ((err = hc_add_endpoint(bus->hc, dev->slot_id, xhci_endpoint_index(ep), &ep_ctx)))
		return err;

	return EOK;
}

/**
 * Abort a transfer on an endpoint.
 */
static int endpoint_abort(endpoint_t *ep)
{
	xhci_bus_t *bus = bus_to_xhci_bus(endpoint_get_bus(ep));
	xhci_device_t *dev = xhci_device_get(ep->device);

	usb_transfer_batch_t *batch = NULL;
	fibril_mutex_lock(&ep->guard);
	if (ep->active_batch) {
		if (dev->slot_id) {
			const int err = hc_stop_endpoint(bus->hc, dev->slot_id, xhci_endpoint_dci(xhci_endpoint_get(ep)));
			if (err) {
				usb_log_warning("Failed to stop endpoint %u of device " XHCI_DEV_FMT ": %s",
				    ep->endpoint, XHCI_DEV_ARGS(*dev), str_error(err));
			}

			endpoint_wait_timeout_locked(ep, 2000);
		}

		batch = ep->active_batch;
		if (batch) {
			endpoint_deactivate_locked(ep);
		}
	}
	fibril_mutex_unlock(&ep->guard);

	if (batch) {
		batch->error = EINTR;
		batch->transfered_size = 0;
		usb_transfer_batch_finish(batch);
	}
	return EOK;
}

/**
 * Unregister an endpoint. If the device is still available, inform the xHC
 * about it.
 *
 * Bus callback.
 */
static void endpoint_unregister(endpoint_t *ep_base)
{
	int err;
	xhci_bus_t *bus = bus_to_xhci_bus(endpoint_get_bus(ep_base));
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);
	xhci_device_t *dev = xhci_device_get(ep_base->device);

	endpoint_abort(ep_base);

	/* If device slot is still available, drop the endpoint. */
	if (dev->slot_id) {

		if ((err = hc_drop_endpoint(bus->hc, dev->slot_id, xhci_endpoint_index(ep)))) {
			usb_log_error("Failed to drop endpoint " XHCI_EP_FMT ": %s", XHCI_EP_ARGS(*ep), str_error(err));
		}
	} else {
		usb_log_debug("Not going to drop endpoint " XHCI_EP_FMT " because"
		    " the slot has already been disabled.", XHCI_EP_ARGS(*ep));
	}
}

/**
 * Schedule a batch for xHC.
 *
 * Bus callback.
 */
static int batch_schedule(usb_transfer_batch_t *batch)
{
	assert(batch);
	xhci_hc_t *hc = bus_to_hc(endpoint_get_bus(batch->ep));

	if (!batch->target.address) {
		usb_log_error("Attempted to schedule transfer to address 0.");
		return EINVAL;
	}

	return xhci_transfer_schedule(hc, batch);
}

static const bus_ops_t xhci_bus_ops = {
	.interrupt = hc_interrupt,
	.status = hc_status,

	.device_enumerate = device_enumerate,
	.device_gone = device_gone,
	.device_online = device_online,
	.device_offline = device_offline,

	.endpoint_create = endpoint_create,
	.endpoint_destroy = endpoint_destroy,
	.endpoint_register = endpoint_register,
	.endpoint_unregister = endpoint_unregister,

	.batch_schedule = batch_schedule,
	.batch_create = xhci_transfer_create,
	.batch_destroy = xhci_transfer_destroy,
};

/** Initialize XHCI bus.
 * @param[in] bus Allocated XHCI bus to initialize.
 * @param[in] hc Associated host controller, which manages the bus.
 *
 * @return Error code.
 */
int xhci_bus_init(xhci_bus_t *bus, xhci_hc_t *hc)
{
	assert(bus);

	bus_init(&bus->base, sizeof(xhci_device_t));

	bus->devices_by_slot = calloc(hc->max_slots, sizeof(xhci_device_t *));
	if (!bus->devices_by_slot)
		return ENOMEM;

	bus->hc = hc;
	bus->base.ops = &xhci_bus_ops;
	return EOK;
}

/** Finalize XHCI bus.
 * @param[in] bus XHCI bus to finalize.
 */
void xhci_bus_fini(xhci_bus_t *bus)
{
	// FIXME: Ensure there are no more devices?
	free(bus->devices_by_slot);
	// FIXME: Something else we forgot?
}

/**
 * @}
 */
