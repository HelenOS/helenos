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

#include <usb/host/endpoint.h>
#include <usb/descriptor.h>

#include <errno.h>
#include <macros.h>

#include "hc.h"
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

static bool endpoint_using_streams(xhci_endpoint_t *xhci_ep)
{
	return xhci_ep->primary_stream_ctx_array != NULL;
}

static size_t primary_stream_ctx_array_max_size(xhci_endpoint_t *xhci_ep)
{
	if (!xhci_ep->max_streams)
		return 0;

	/* Section 6.2.3, Table 61 */
	return 1 << (xhci_ep->max_streams + 1);
}

// static bool primary_stream_ctx_has_secondary_array(xhci_stream_ctx_t *primary_ctx) {
// 	/* Section 6.2.4.1, SCT values */
// 	return XHCI_STREAM_SCT(*primary_ctx) >= 2;
// }
//
// static size_t secondary_stream_ctx_array_size(xhci_stream_ctx_t *primary_ctx) {
// 	if (XHCI_STREAM_SCT(*primary_ctx) < 2) return 0;
// 	return 2 << XHCI_STREAM_SCT(*primary_ctx);
// }

static void initialize_primary_streams(xhci_hc_t *hc, xhci_endpoint_t *xhci_ep, unsigned count) {
	for (size_t index = 0; index < count; ++index) {
		xhci_stream_ctx_t *ctx = &xhci_ep->primary_stream_ctx_array[index];
		xhci_trb_ring_t *ring = &xhci_ep->primary_stream_rings[index];

		/* Init and register TRB ring for every primary stream */
		xhci_trb_ring_init(ring);
		XHCI_STREAM_DEQ_PTR_SET(*ctx, ring->dequeue);

		/* Set to linear stream array */
		XHCI_STREAM_SCT_SET(*ctx, 1);
	}
}

static void setup_stream_context(xhci_endpoint_t *xhci_ep, xhci_ep_ctx_t *ctx, unsigned pstreams) {
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(xhci_ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, xhci_ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, xhci_ep->max_burst);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	XHCI_EP_MAX_P_STREAMS_SET(*ctx, pstreams);
	XHCI_EP_TR_DPTR_SET(*ctx, xhci_ep->primary_stream_ctx_dma.phys);
	// TODO: set HID?
	XHCI_EP_LSA_SET(*ctx, 1);
}

int xhci_endpoint_request_streams(xhci_hc_t *hc, xhci_device_t *dev, xhci_endpoint_t *xhci_ep, unsigned count) {
	if (xhci_ep->base.transfer_type != USB_TRANSFER_BULK
		|| xhci_ep->base.speed != USB_SPEED_SUPER) {
		usb_log_error("Streams are only supported by superspeed bulk endpoints.");
		return EINVAL;
	}

	if (!primary_stream_ctx_array_max_size(xhci_ep)) {
		usb_log_error("Streams are not supported by endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));
		return EINVAL;
	}

	uint8_t max_psa_size = 2 << XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PSA_SIZE);
	if (count > max_psa_size) {
		// FIXME: We don't support secondary stream arrays yet, so we just give up for this
		return ENOTSUP;
	}

	if (count > (unsigned) (1 << xhci_ep->max_streams)) {
		usb_log_error("Endpoint " XHCI_EP_FMT " supports only %u streams.",
			XHCI_EP_ARGS(*xhci_ep), (1 << xhci_ep->max_streams));
		return EINVAL;
	}

	if (count <= 1024) {
		usb_log_debug2("Allocating primary stream context array of size %u for endpoint " XHCI_EP_FMT,
			count, XHCI_EP_ARGS(*xhci_ep));
		if ((dma_buffer_alloc(&xhci_ep->primary_stream_ctx_dma, count * sizeof(xhci_stream_ctx_t))))
			return ENOMEM;
		xhci_ep->primary_stream_ctx_array = xhci_ep->primary_stream_ctx_dma.virt;

		xhci_ep->primary_stream_rings = calloc(count, sizeof(xhci_trb_ring_t));
		if (!xhci_ep->primary_stream_rings) {
			dma_buffer_free(&xhci_ep->primary_stream_ctx_dma);
			return ENOMEM;
		}

		// FIXME: count should be rounded to nearest power of 2 for xHC, workaround for now
		count = 1024;
		// FIXME: pstreams are "log2(count) - 1"
		const size_t pstreams = 9;
		xhci_ep->primary_stream_ctx_array_size = count;

		memset(xhci_ep->primary_stream_ctx_array, 0, count * sizeof(xhci_stream_ctx_t));
		initialize_primary_streams(hc, xhci_ep, count);

		xhci_ep_ctx_t ep_ctx;
		setup_stream_context(xhci_ep, &ep_ctx, pstreams);
		return hc_add_endpoint(hc, dev->slot_id, xhci_endpoint_index(xhci_ep), &ep_ctx);
	}
	// FIXME: Complex stuff not supported yet
	return ENOTSUP;
}

int xhci_endpoint_alloc_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	/* Can't use XHCI_EP_FMT because the endpoint may not have device. */
	usb_log_debug2("Allocating main transfer ring for endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

	xhci_ep->primary_stream_ctx_array = NULL;

	int err;
	if ((err = xhci_trb_ring_init(&xhci_ep->ring))) {
		return err;
	}

	return EOK;
}

void xhci_endpoint_free_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	if (endpoint_using_streams(xhci_ep)) {
		usb_log_debug2("Freeing primary stream context array of endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

		// maybe check if LSA, then skip?
		// for (size_t index = 0; index < primary_stream_ctx_array_size(xhci_ep); ++index) {
		// 	xhci_stream_ctx_t *primary_ctx = xhci_ep->primary_stream_ctx_array + index;
		// 	if (primary_stream_ctx_has_secondary_array(primary_ctx)) {
		// 		// uintptr_t phys = XHCI_STREAM_DEQ_PTR(*primary_ctx);
		// 		/* size_t size = */ secondary_stream_ctx_array_size(primary_ctx);
		// 		// TODO: somehow map the address to virtual and free the secondary array
		// 	}
		// }
		for (size_t index = 0; index < xhci_ep->primary_stream_ctx_array_size; ++index) {
			// FIXME: Get the trb ring associated with stream [index] and fini it
		}
		dma_buffer_free(&xhci_ep->primary_stream_ctx_dma);
	} else {
		usb_log_debug2("Freeing main transfer ring of endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

		xhci_trb_ring_fini(&xhci_ep->ring);
	}
}

/** See section 4.5.1 of the xHCI spec.
 */
uint8_t xhci_endpoint_dci(xhci_endpoint_t *ep)
{
	return (2 * ep->base.endpoint) +
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

static void setup_control_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst);
	XHCI_EP_MULT_SET(*ctx, ep->mult);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
}

static void setup_bulk_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	XHCI_EP_MAX_P_STREAMS_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
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

void xhci_setup_endpoint_context(xhci_endpoint_t *ep, xhci_ep_ctx_t *ep_ctx)
{
	assert(ep);
	assert(ep_ctx);

	usb_transfer_type_t tt = ep->base.transfer_type;
	assert(tt < ARRAY_SIZE(setup_ep_ctx_helpers));

	memset(ep_ctx, 0, sizeof(*ep_ctx));
	setup_ep_ctx_helpers[tt](ep, ep_ctx);
}

int xhci_device_add_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(dev);
	assert(ep);

	/* Offline devices don't create new endpoints other than EP0. */
	if (!dev->online && ep->base.endpoint > 0) {
		return EAGAIN;
	}

	const usb_endpoint_t ep_num = ep->base.endpoint;

	if (dev->endpoints[ep_num])
		return EEXIST;

	/* Device reference */
	endpoint_add_ref(&ep->base);
	ep->base.device = &dev->base;
	dev->endpoints[ep_num] = ep;

	return EOK;
}

void xhci_device_remove_endpoint(xhci_endpoint_t *ep)
{
	assert(ep);
	xhci_device_t *dev = xhci_device_get(ep->base.device);

	assert(dev->endpoints[ep->base.endpoint]);
	dev->endpoints[ep->base.endpoint] = NULL;
	ep->base.device = NULL;

	endpoint_del_ref(&ep->base);
}

xhci_endpoint_t *xhci_device_get_endpoint(xhci_device_t *dev, usb_endpoint_t ep)
{
	return dev->endpoints[ep];
}

/**
 * @}
 */
