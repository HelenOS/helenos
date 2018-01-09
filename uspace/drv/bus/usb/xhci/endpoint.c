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

static int alloc_transfer_ds(xhci_endpoint_t *);
static void free_transfer_ds(xhci_endpoint_t *);

/**
 * Initialize new XHCI endpoint.
 * @param[in] xhci_ep Allocated XHCI endpoint to initialize.
 * @param[in] dev Device, to which the endpoint belongs.
 * @param[in] desc USB endpoint descriptor carrying configuration data.
 *
 * @return Error code.
 */
int xhci_endpoint_init(xhci_endpoint_t *xhci_ep, device_t *dev, const usb_endpoint_descriptors_t *desc)
{
	int rc;
	assert(xhci_ep);

	endpoint_t *ep = &xhci_ep->base;

	endpoint_init(ep, dev, desc);

	xhci_ep->max_streams = 1 << (USB_SSC_MAX_STREAMS(desc->companion));
	xhci_ep->max_burst = desc->companion.max_burst + 1;
	xhci_ep->mult = USB_SSC_MULT(desc->companion) + 1;

	/* In USB 3, the semantics of wMaxPacketSize changed. Now the number of
	 * packets per service interval is determined from max_burst and mult.
	 */
	if (dev->speed >= USB_SPEED_SUPER) {
		ep->packets_per_uframe = xhci_ep->max_burst * xhci_ep->mult;
		ep->max_transfer_size = ep->max_packet_size * ep->packets_per_uframe;
	}

	xhci_ep->interval = desc->endpoint.poll_interval;

	/*
	 * Only Low/Full speed interrupt endpoints have interval as a linear field,
	 * others have 2-based log of it.
	 */
	if (dev->speed >= USB_SPEED_HIGH || ep->transfer_type != USB_TRANSFER_INTERRUPT) {
		xhci_ep->interval = desc->endpoint.poll_interval;
	}

	/* Full speed devices have interval in frames */
	if (dev->speed <= USB_SPEED_FULL) {
		xhci_ep->interval *= 8;
	}

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		xhci_ep->isoch->max_size = desc->companion.bytes_per_interval
			? desc->companion.bytes_per_interval
			: ep->max_transfer_size;
		/* Technically there could be superspeed plus too. */

		/* Allocate and setup isochronous-specific structures. */
		xhci_ep->isoch->enqueue = 0;
		xhci_ep->isoch->dequeue = 0;
		xhci_ep->isoch->started = false;

		fibril_mutex_initialize(&xhci_ep->isoch->guard);
		fibril_condvar_initialize(&xhci_ep->isoch->avail);
	}

	if ((rc = alloc_transfer_ds(xhci_ep)))
		goto err;

	return EOK;

err:
	return rc;
}

/**
 * Finalize XHCI endpoint.
 * @param[in] xhci_ep XHCI endpoint to finalize.
 */
void xhci_endpoint_fini(xhci_endpoint_t *xhci_ep)
{
	assert(xhci_ep);

	free_transfer_ds(xhci_ep);

	// TODO: Something missed?
}

/**
 * Determine the type of a XHCI endpoint.
 * @param[in] ep XHCI endpoint to query.
 *
 * @return EP_TYPE_[CONTROL|ISOCH|BULK|INTERRUPT]_[IN|OUT]
 */
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

/**
 * Test whether an XHCI endpoint uses streams.
 * @param[in] xhci_ep XHCI endpoint to query.
 *
 * @return True if the endpoint uses streams.
 */
static bool endpoint_using_streams(xhci_endpoint_t *xhci_ep)
{
	return xhci_ep->primary_stream_ctx_array != NULL;
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

/** Initialize primary streams of XHCI bulk endpoint.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] xhci_epi XHCI bulk endpoint to use.
 * @param[in] count Number of primary streams to initialize.
 */
static void initialize_primary_streams(xhci_hc_t *hc, xhci_endpoint_t *xhci_ep, unsigned count) {
	for (size_t index = 0; index < count; ++index) {
		xhci_stream_ctx_t *ctx = &xhci_ep->primary_stream_ctx_array[index];
		xhci_trb_ring_t *ring = &xhci_ep->primary_stream_rings[index];

		/* Init and register TRB ring for every primary stream */
		xhci_trb_ring_init(ring); // FIXME: Not checking error code?
		XHCI_STREAM_DEQ_PTR_SET(*ctx, ring->dequeue);

		/* Set to linear stream array */
		XHCI_STREAM_SCT_SET(*ctx, 1);
	}
}

/** Configure XHCI bulk endpoint's stream context.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 * @param[in] ctx Endpoint context to configure.
 * @param[in] pstreams The value of MaxPStreams.
 */
static void setup_stream_context(xhci_endpoint_t *xhci_ep, xhci_ep_ctx_t *ctx, unsigned pstreams) {
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(xhci_ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, xhci_ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, xhci_ep->max_burst - 1);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	XHCI_EP_MAX_P_STREAMS_SET(*ctx, pstreams);
	XHCI_EP_TR_DPTR_SET(*ctx, xhci_ep->primary_stream_ctx_dma.phys);
	// TODO: set HID?
	XHCI_EP_LSA_SET(*ctx, 1);
}

/** TODO document this
 */
int xhci_endpoint_request_streams(xhci_hc_t *hc, xhci_device_t *dev, xhci_endpoint_t *xhci_ep, unsigned count) {
	if (xhci_ep->base.transfer_type != USB_TRANSFER_BULK
		|| dev->base.speed != USB_SPEED_SUPER) {
		usb_log_error("Streams are only supported by superspeed bulk endpoints.");
		return EINVAL;
	}

	if (xhci_ep->max_streams == 1) {
		usb_log_error("Streams are not supported by endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));
		return EINVAL;
	}

	uint8_t max_psa_size = 2 << XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PSA_SIZE);
	if (count > max_psa_size) {
		// FIXME: We don't support secondary stream arrays yet, so we just give up for this
		return ENOTSUP;
	}

	if (count > xhci_ep->max_streams) {
		usb_log_error("Endpoint " XHCI_EP_FMT " supports only %" PRIu32 " streams.",
			XHCI_EP_ARGS(*xhci_ep), xhci_ep->max_streams);
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

/** TODO document this
 */
static int xhci_isoch_alloc_transfers(xhci_endpoint_t *xhci_ep) {
	int i = 0;
	int err = EOK;
	while (i < XHCI_ISOCH_BUFFER_COUNT) {
		xhci_isoch_transfer_t *transfer = &xhci_ep->isoch->transfers[i];
		if (dma_buffer_alloc(&transfer->data, xhci_ep->isoch->max_size)) {
			err = ENOMEM;
			break;
		}
		transfer->size = 0;
		++i;
	}

	if (err) {
		--i;
		while(i >= 0) {
			dma_buffer_free(&xhci_ep->isoch->transfers[i].data);
			--i;
		}
	}

	return err;
}

/** Allocate transfer data structures for XHCI endpoint.
 * @param[in] xhci_ep XHCI endpoint to allocate data structures for.
 *
 * @return Error code.
 */
static int alloc_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	/* Can't use XHCI_EP_FMT because the endpoint may not have device. */
	usb_log_debug2("Allocating main transfer ring for endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

	xhci_ep->primary_stream_ctx_array = NULL;

	int err;
	if ((err = xhci_trb_ring_init(&xhci_ep->ring))) {
		return err;
	}

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		if ((err = xhci_isoch_alloc_transfers(xhci_ep))) {
			xhci_trb_ring_fini(&xhci_ep->ring);
			return err;
		}
	}

	return EOK;
}

/** Free transfer data structures for XHCI endpoint.
 * @param[in] xhci_ep XHCI endpoint to free data structures for.
 */
static void free_transfer_ds(xhci_endpoint_t *xhci_ep)
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

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		for (size_t i = 0; i < XHCI_ISOCH_BUFFER_COUNT; ++i) {
			dma_buffer_free(&xhci_ep->isoch->transfers[i].data);
		}
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

/** Configure endpoint context of a control endpoint.
 * @param[in] ep XHCI control endpoint.
 * @param[in] ctx Endpoint context to configure.
 */
static void setup_control_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst - 1);
	XHCI_EP_MULT_SET(*ctx, ep->mult - 1);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
}

/** Configure endpoint context of a bulk endpoint.
 * @param[in] ep XHCI bulk endpoint.
 * @param[in] ctx Endpoint context to configure.
 */
static void setup_bulk_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst - 1);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	XHCI_EP_MAX_P_STREAMS_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
}

/** Configure endpoint context of a isochronous endpoint.
 * @param[in] ep XHCI isochronous endpoint.
 * @param[in] ctx Endpoint context to configure.
 */
static void setup_isoch_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst - 1);
	XHCI_EP_MULT_SET(*ctx, ep->mult - 1);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	XHCI_EP_INTERVAL_SET(*ctx, fnzb32(ep->interval) % 32 - 1);

	XHCI_EP_MAX_ESIT_PAYLOAD_LO_SET(*ctx, ep->isoch->max_size & 0xFFFF);
	XHCI_EP_MAX_ESIT_PAYLOAD_HI_SET(*ctx, (ep->isoch->max_size >> 16) & 0xFF);
}

/** Configure endpoint context of a interrupt endpoint.
 * @param[in] ep XHCI interrupt endpoint.
 * @param[in] ctx Endpoint context to configure.
 */
static void setup_interrupt_ep_ctx(xhci_endpoint_t *ep, xhci_ep_ctx_t *ctx)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, ep->base.max_packet_size & 0x07FF);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, ep->max_burst - 1);
	XHCI_EP_MULT_SET(*ctx, 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
	XHCI_EP_TR_DPTR_SET(*ctx, ep->ring.dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	XHCI_EP_INTERVAL_SET(*ctx, fnzb32(ep->interval) % 32 - 1);
	// TODO: max ESIT payload
}

/** Type of endpoint context configuration function. */
typedef void (*setup_ep_ctx_helper)(xhci_endpoint_t *, xhci_ep_ctx_t *);

/** Static array, which maps USB endpoint types to their respective endpoint context configuration functions. */
static const setup_ep_ctx_helper setup_ep_ctx_helpers[] = {
	[USB_TRANSFER_CONTROL] = setup_control_ep_ctx,
	[USB_TRANSFER_ISOCHRONOUS] = setup_isoch_ep_ctx,
	[USB_TRANSFER_BULK] = setup_bulk_ep_ctx,
	[USB_TRANSFER_INTERRUPT] = setup_interrupt_ep_ctx,
};

/** Configure endpoint context of XHCI endpoint.
 * @param[in] ep Associated XHCI endpoint.
 * @param[in] ep_ctx Endpoint context to configure.
 */
void xhci_setup_endpoint_context(xhci_endpoint_t *ep, xhci_ep_ctx_t *ep_ctx)
{
	assert(ep);
	assert(ep_ctx);

	usb_transfer_type_t tt = ep->base.transfer_type;
	assert(tt < ARRAY_SIZE(setup_ep_ctx_helpers));

	memset(ep_ctx, 0, sizeof(*ep_ctx));
	setup_ep_ctx_helpers[tt](ep, ep_ctx);
}

/** Retrieve XHCI endpoint from a device by the endpoint number.
 * @param[in] dev XHCI device to query.
 * @param[in] ep Endpoint number identifying the endpoint to retrieve.
 *
 * @return XHCI endpoint with the specified number or NULL if no such endpoint exists.
 */
xhci_endpoint_t *xhci_device_get_endpoint(xhci_device_t *dev, usb_endpoint_t ep)
{
	endpoint_t *ep_base = dev->base.endpoints[ep];
	if (!ep_base)
		return NULL;

	return xhci_endpoint_get(ep_base);
}

/**
 * @}
 */
