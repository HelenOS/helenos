/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller endpoint management.
 */

#include <usb/host/utils/malloc32.h>
#include <usb/host/endpoint.h>
#include <usb/descriptor.h>

#include <errno.h>

#include "bus.h"
#include "commands.h"
#include "endpoint.h"

int xhci_endpoint_init(xhci_endpoint_t *xhci_ep, xhci_bus_t *xhci_bus)
{
	assert(xhci_ep);
	assert(xhci_bus);

	bus_t *bus = &xhci_bus->base;
	endpoint_t *ep = &xhci_ep->base;

	endpoint_init(ep, bus);

	return EOK;
}

void xhci_endpoint_fini(xhci_endpoint_t *xhci_ep)
{
	assert(xhci_ep);

	// TODO: Something missed?
}

static bool endpoint_uses_streams(xhci_endpoint_t *xhci_ep)
{
	return xhci_ep->base.transfer_type == USB_TRANSFER_BULK
	    && xhci_ep->max_streams;
}

static size_t primary_stream_ctx_array_size(xhci_endpoint_t *xhci_ep)
{
	if (!endpoint_uses_streams(xhci_ep))
		return 0;

	/* Section 6.2.3, Table 61 */
	return 1 << (xhci_ep->max_streams + 1);
}

int xhci_endpoint_alloc_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	if (endpoint_uses_streams(xhci_ep)) {
		/* Set up primary stream context array if needed. */
		const size_t size = primary_stream_ctx_array_size(xhci_ep);
		usb_log_debug2("Allocating primary stream context array of size %lu for endpoint %d:%d.",
		    size, xhci_ep->base.target.address, xhci_ep->base.target.endpoint);

		xhci_ep->primary_stream_ctx_array = malloc32(size * sizeof(xhci_stream_ctx_t));
		if (!xhci_ep->primary_stream_ctx_array) {
			return ENOMEM;
		}

		memset(xhci_ep->primary_stream_ctx_array, 0, size * sizeof(xhci_stream_ctx_t));
	} else {
		usb_log_debug2("Allocating main transfer ring for endpoint %d:%d.",
		    xhci_ep->base.target.address, xhci_ep->base.target.endpoint);

		xhci_ep->primary_stream_ctx_array = NULL;

		int err;
		if ((err = xhci_trb_ring_init(&xhci_ep->ring))) {
			return err;
		}
	}

	return EOK;
}

int xhci_endpoint_free_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	if (endpoint_uses_streams(xhci_ep)) {
		usb_log_debug2("Freeing primary stream context array for endpoint %d:%d.",
		    xhci_ep->base.target.address, xhci_ep->base.target.endpoint);

		// TODO: What about secondaries?
		free32(xhci_ep->primary_stream_ctx_array);
	} else {
		usb_log_debug2("Freeing main transfer ring for endpoint %d:%d.",
		    xhci_ep->base.target.address, xhci_ep->base.target.endpoint);

		int err;
		if ((err = xhci_trb_ring_fini(&xhci_ep->ring))) {
			return err;
		}
	}

	return EOK;
}

/** See section 4.5.1 of the xHCI spec.
 */
uint8_t xhci_endpoint_dci(xhci_endpoint_t *ep)
{
	return (2 * ep->base.target.endpoint) +
		(ep->base.transfer_type == USB_TRANSFER_CONTROL
		 || ep->base.direction == USB_DIRECTION_IN);
}

/** Return an index to the endpoint array. The indices are assigned as follows:
 * 0	EP0 BOTH
 * 1	EP1 OUT
 * 2	EP1 IN
 *
 * For control endpoints >0, the IN endpoint index is used.
 *
 * The index returned must be usually offset by a number of contexts preceding
 * the endpoint contexts themselves.
 */
uint8_t xhci_endpoint_index(xhci_endpoint_t *ep)
{
	return xhci_endpoint_dci(ep) - 1;
}

static int xhci_endpoint_type(xhci_endpoint_t *ep)
{
	const bool in = ep->base.direction == USB_DIRECTION_IN;

	switch (ep->base.transfer_type) {
	case USB_TRANSFER_CONTROL:
		return EP_TYPE_CONTROL;

	case USB_TRANSFER_ISOCHRONOUS:
		return in ? EP_TYPE_ISOCH_IN
			  : EP_TYPE_ISOCH_OUT;

	case USB_TRANSFER_BULK:
		return in ? EP_TYPE_BULK_IN
			  : EP_TYPE_BULK_OUT;

	case USB_TRANSFER_INTERRUPT:
		return in ? EP_TYPE_INTERRUPT_IN
			  : EP_TYPE_INTERRUPT_OUT;
	}

	return EP_TYPE_INVALID;
}

static void setup_control_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	// EP0 is configured elsewhere.
	assert(ep->base.target.endpoint > 0);

	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
}

static void setup_bulk_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx,
	    xhci_device_get(ep->base.device)->usb3 ? ep->max_burst : 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	if (endpoint_uses_streams(ep)) {
		XHCI_EP_MAX_P_STREAMS_SET(*ctx, ep->max_streams);
		XHCI_EP_TR_DPTR_SET(*ctx, addr_to_phys(ep->primary_stream_ctx_array));
		// TODO: set HID
		// TODO: set LSA
	} else {
		XHCI_EP_MAX_P_STREAMS_SET(*ctx, 0);
		XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
		XHCI_EP_DCS_SET(*ctx, 1);
	}
}

static void setup_isoch_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst);
	XHCI_EP_MULT_SET(*ctx, ep->mult);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	// TODO: max ESIT payload
}

static void setup_interrupt_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst);
	XHCI_EP_MULT_SET(*ctx, 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	// TODO: max ESIT payload
}

typedef void (*setup_ep_ctx_helper)(xhci_endpoint_t *, xhci_ep_ctx_t *);

static const setup_ep_ctx_helper setup_ep_ctx_helpers[] = {
	[USB_TRANSFER_CONTROL] = setup_control_ep_ctx,
	[USB_TRANSFER_ISOCHRONOUS] = setup_isoch_ep_ctx,
	[USB_TRANSFER_BULK] = setup_bulk_ep_ctx,
	[USB_TRANSFER_INTERRUPT] = setup_interrupt_ep_ctx,
};

int xhci_device_add_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(dev);
	assert(ep);

	/* Offline devices don't create new endpoints other than EP0. */
	if (!dev->online) {
		return EAGAIN;
	}

	int err = ENOMEM;
	const usb_endpoint_t ep_num = ep->base.target.endpoint;

	assert(&dev->base == ep->base.device);
	assert(dev->base.address == ep->base.target.address);
	assert(!dev->endpoints[ep_num]);

	dev->endpoints[ep_num] = ep;
	++dev->active_endpoint_count;

	if (ep_num == 0) {
		/* EP 0 is initialized while setting up the device,
		 * so we must not issue the command now. */
		return EOK;
	}

	// FIXME: Set these from usb_superspeed_endpoint_companion_descriptor_t:
	ep->max_streams = 0;
	ep->max_burst = 0;
	ep->mult = 0;

	/* Set up TRB ring / PSA. */
	if ((err = xhci_endpoint_alloc_transfer_ds(ep))) {
		goto err;
	}

	/* Add endpoint. */
	xhci_ep_ctx_t ep_ctx;
	memset(&ep_ctx, 0, sizeof(xhci_ep_ctx_t));
	setup_ep_ctx_helpers[ep->base.transfer_type](ep, &ep_ctx);

	if ((err = hc_add_endpoint(dev->hc, dev->slot_id, xhci_endpoint_index(ep), &ep_ctx))) {
		goto err_ds;
	}

	return EOK;

err_ds:
	xhci_endpoint_free_transfer_ds(ep);
err:
	dev->endpoints[ep_num] = NULL;
	dev->active_endpoint_count--;
	return err;
}

int xhci_device_remove_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(&dev->base == ep->base.device);
	assert(dev->base.address == ep->base.target.address);
	assert(dev->endpoints[ep->base.target.endpoint]);

	int err = ENOMEM;
	const usb_endpoint_t ep_num = ep->base.target.endpoint;

	dev->endpoints[ep->base.target.endpoint] = NULL;
	--dev->active_endpoint_count;

	if (ep_num == 0) {
		/* EP 0 is finalized while releasing the device,
		 * so we must not issue the command now. */
		return EOK;
	}

	/* Drop the endpoint. */
	if ((err = hc_drop_endpoint(dev->hc, dev->slot_id, xhci_endpoint_index(ep)))) {
		goto err;
	}

	/* Tear down TRB ring / PSA. */
	/* FIXME: For some reason, this causes crash at xhci_trb_ring_fini.
	if ((err = xhci_endpoint_free_transfer_ds(ep))) {
		goto err_cmd;
	}
	*/

	return EOK;

err:
	dev->endpoints[ep_num] = ep;
	++dev->active_endpoint_count;
	return err;
}

xhci_endpoint_t *xhci_device_get_endpoint(xhci_device_t *dev, usb_endpoint_t ep)
{
	return dev->endpoints[ep];
}

/**
 * @}
 */
