/*
 * Copyright (c) 2018 Petr Manek, Ondrej Hlavaty, Michal Staruch, Jan Hrach
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
#include <str_error.h>

#include "hc.h"
#include "bus.h"
#include "commands.h"
#include "device.h"
#include "endpoint.h"
#include "streams.h"

static errno_t alloc_transfer_ds(xhci_endpoint_t *);

/**
 * Initialize new XHCI endpoint.
 * @param[in] xhci_ep Allocated XHCI endpoint to initialize.
 * @param[in] dev Device, to which the endpoint belongs.
 * @param[in] desc USB endpoint descriptor carrying configuration data.
 *
 * @return Error code.
 */
static errno_t xhci_endpoint_init(xhci_endpoint_t *xhci_ep, device_t *dev,
    const usb_endpoint_descriptors_t *desc)
{
	errno_t rc;
	assert(xhci_ep);

	endpoint_t *ep = &xhci_ep->base;

	endpoint_init(ep, dev, desc);

	fibril_mutex_initialize(&xhci_ep->guard);

	xhci_ep->max_burst = desc->companion.max_burst + 1;

	if (ep->transfer_type == USB_TRANSFER_BULK)
		xhci_ep->max_streams = 1 << (USB_SSC_MAX_STREAMS(desc->companion));
	else
		xhci_ep->max_streams = 1;

	if (ep->transfer_type == USB_TRANSFER_ISOCHRONOUS)
		xhci_ep->mult = USB_SSC_MULT(desc->companion) + 1;
	else
		xhci_ep->mult = 1;

	/*
	 * In USB 3, the semantics of wMaxPacketSize changed. Now the number of
	 * packets per service interval is determined from max_burst and mult.
	 */
	if (dev->speed >= USB_SPEED_SUPER) {
		ep->packets_per_uframe = xhci_ep->max_burst * xhci_ep->mult;
		if (ep->transfer_type == USB_TRANSFER_ISOCHRONOUS ||
		    ep->transfer_type == USB_TRANSFER_INTERRUPT) {
			ep->max_transfer_size = ep->max_packet_size * ep->packets_per_uframe;
		}
	}

	xhci_ep->interval = desc->endpoint.poll_interval;

	/*
	 * Only Low/Full speed interrupt endpoints have interval as a linear field,
	 * others have 2-based log of it.
	 */
	if (dev->speed >= USB_SPEED_HIGH ||
	    ep->transfer_type != USB_TRANSFER_INTERRUPT) {
		xhci_ep->interval = 1 << (xhci_ep->interval - 1);
	}

	/* Full speed devices have interval in frames */
	if (dev->speed <= USB_SPEED_FULL) {
		xhci_ep->interval *= 8;
	}

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS)
		isoch_init(xhci_ep, desc);

	if ((rc = alloc_transfer_ds(xhci_ep)))
		goto err;

	unsigned flags = -1U;

	/* Some xHCs can handle 64-bit addresses */
	xhci_bus_t *bus = bus_to_xhci_bus(ep->device->bus);
	if (bus->hc->ac64)
		flags &= ~DMA_POLICY_4GiB;

	/* xHCI works best if it can fit 65k transfers in one TRB */
	ep->transfer_buffer_policy = dma_policy_create(flags, 1 << 16);

	/* But actualy can do full scatter-gather. */
	ep->required_transfer_buffer_policy = dma_policy_create(flags, PAGE_SIZE);

	return EOK;

err:
	return rc;
}

/**
 * Create a new xHCI endpoint structure.
 *
 * Bus callback.
 */
endpoint_t *xhci_endpoint_create(device_t *dev,
    const usb_endpoint_descriptors_t *desc)
{
	const usb_transfer_type_t type = USB_ED_GET_TRANSFER_TYPE(desc->endpoint);

	xhci_endpoint_t *ep = calloc(1, sizeof(xhci_endpoint_t) +
	    (type == USB_TRANSFER_ISOCHRONOUS) * sizeof(*ep->isoch));
	if (!ep)
		return NULL;

	if (xhci_endpoint_init(ep, dev, desc)) {
		free(ep);
		return NULL;
	}

	return &ep->base;
}

/**
 * Finalize XHCI endpoint.
 * @param[in] xhci_ep XHCI endpoint to finalize.
 */
static void xhci_endpoint_fini(xhci_endpoint_t *xhci_ep)
{
	assert(xhci_ep);

	xhci_endpoint_free_transfer_ds(xhci_ep);

	// TODO: Something missed?
}

/**
 * Destroy given xHCI endpoint structure.
 *
 * Bus callback.
 */
void xhci_endpoint_destroy(endpoint_t *ep)
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
errno_t xhci_endpoint_register(endpoint_t *ep_base)
{
	errno_t err;
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);

	if (ep_base->endpoint != 0 && (err = hc_add_endpoint(ep)))
		return err;

	endpoint_set_online(ep_base, &ep->guard);
	return EOK;
}

/**
 * Abort a transfer on an endpoint.
 */
static void endpoint_abort(endpoint_t *ep)
{
	xhci_device_t *dev = xhci_device_get(ep->device);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);

	/* This function can only abort endpoints without streams. */
	assert(xhci_ep->primary_stream_data_array == NULL);

	fibril_mutex_lock(&xhci_ep->guard);

	endpoint_set_offline_locked(ep);

	if (!ep->active_batch) {
		fibril_mutex_unlock(&xhci_ep->guard);
		return;
	}

	/* First, offer the batch a short chance to be finished. */
	endpoint_wait_timeout_locked(ep, 10000);

	if (!ep->active_batch) {
		fibril_mutex_unlock(&xhci_ep->guard);
		return;
	}

	usb_transfer_batch_t *const batch = ep->active_batch;

	const errno_t err = hc_stop_endpoint(xhci_ep);
	if (err) {
		usb_log_error("Failed to stop endpoint %u of device "
		    XHCI_DEV_FMT ": %s", ep->endpoint, XHCI_DEV_ARGS(*dev),
		    str_error(err));
	}

	fibril_mutex_unlock(&xhci_ep->guard);

	batch->error = EINTR;
	batch->transferred_size = 0;
	usb_transfer_batch_finish(batch);
	return;
}

/**
 * Unregister an endpoint. If the device is still available, inform the xHC
 * about it.
 *
 * Bus callback.
 */
void xhci_endpoint_unregister(endpoint_t *ep_base)
{
	errno_t err;
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);
	xhci_device_t *dev = xhci_device_get(ep_base->device);

	endpoint_abort(ep_base);

	/* If device slot is still available, drop the endpoint. */
	if (ep_base->endpoint != 0 && dev->slot_id) {

		if ((err = hc_drop_endpoint(ep))) {
			usb_log_error("Failed to drop endpoint " XHCI_EP_FMT ": %s",
			    XHCI_EP_ARGS(*ep), str_error(err));
		}
	} else {
		usb_log_debug("Not going to drop endpoint " XHCI_EP_FMT " because"
		    " the slot has already been disabled.", XHCI_EP_ARGS(*ep));
	}
}

/**
 * Determine the type of a XHCI endpoint.
 * @param[in] ep XHCI endpoint to query.
 *
 * @return EP_TYPE_[CONTROL|ISOCH|BULK|INTERRUPT]_[IN|OUT]
 */
int xhci_endpoint_type(xhci_endpoint_t *ep)
{
	const bool in = ep->base.direction == USB_DIRECTION_IN;

	switch (ep->base.transfer_type) {
	case USB_TRANSFER_CONTROL:
		return EP_TYPE_CONTROL;

	case USB_TRANSFER_ISOCHRONOUS:
		return in ? EP_TYPE_ISOCH_IN :
		    EP_TYPE_ISOCH_OUT;

	case USB_TRANSFER_BULK:
		return in ? EP_TYPE_BULK_IN :
		    EP_TYPE_BULK_OUT;

	case USB_TRANSFER_INTERRUPT:
		return in ? EP_TYPE_INTERRUPT_IN :
		    EP_TYPE_INTERRUPT_OUT;
	}

	return EP_TYPE_INVALID;
}

/**
 * Allocate transfer data structures for XHCI endpoint not using streams.
 * @param[in] xhci_ep XHCI endpoint to allocate data structures for.
 *
 * @return Error code.
 */
static errno_t alloc_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	/* Can't use XHCI_EP_FMT because the endpoint may not have device. */
	usb_log_debug("Allocating main transfer ring for endpoint " XHCI_EP_FMT,
	    XHCI_EP_ARGS(*xhci_ep));

	xhci_ep->primary_stream_data_array = NULL;
	xhci_ep->primary_stream_data_size = 0;

	errno_t err;
	if ((err = xhci_trb_ring_init(&xhci_ep->ring, 0))) {
		return err;
	}

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		if ((err = isoch_alloc_transfers(xhci_ep))) {
			xhci_trb_ring_fini(&xhci_ep->ring);
			return err;
		}
	}

	return EOK;
}

/**
 * Free transfer data structures for XHCI endpoint.
 * @param[in] xhci_ep XHCI endpoint to free data structures for.
 */
void xhci_endpoint_free_transfer_ds(xhci_endpoint_t *xhci_ep)
{
	if (xhci_ep->primary_stream_data_size) {
		xhci_stream_free_ds(xhci_ep);
	} else {
		usb_log_debug("Freeing main transfer ring of endpoint " XHCI_EP_FMT,
		    XHCI_EP_ARGS(*xhci_ep));
		xhci_trb_ring_fini(&xhci_ep->ring);
	}

	if (xhci_ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS)
		isoch_fini(xhci_ep);
}

xhci_trb_ring_t *xhci_endpoint_get_ring(xhci_endpoint_t *ep, uint32_t stream_id)
{
	if (ep->primary_stream_data_size == 0)
		return stream_id == 0 ? &ep->ring : NULL;

	xhci_stream_data_t *stream_data = xhci_get_stream_ctx_data(ep, stream_id);
	if (stream_data == NULL) {
		usb_log_warning("No transfer ring was found for stream %u.", stream_id);
		return NULL;
	}

	return &stream_data->ring;
}

/**
 * Configure endpoint context of a control endpoint.
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

/**
 * Configure endpoint context of a bulk endpoint.
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

/**
 * Configure endpoint context of a isochronous endpoint.
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
	XHCI_EP_INTERVAL_SET(*ctx, fnzb32(ep->interval) % 32);

	XHCI_EP_MAX_ESIT_PAYLOAD_LO_SET(*ctx, ep->isoch->max_size & 0xFFFF);
	XHCI_EP_MAX_ESIT_PAYLOAD_HI_SET(*ctx, (ep->isoch->max_size >> 16) & 0xFF);
}

/**
 * Configure endpoint context of a interrupt endpoint.
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
	XHCI_EP_INTERVAL_SET(*ctx, fnzb32(ep->interval) % 32);
	// TODO: max ESIT payload
}

/** Type of endpoint context configuration function. */
typedef void (*setup_ep_ctx_helper)(xhci_endpoint_t *, xhci_ep_ctx_t *);

/**
 * Static array, which maps USB endpoint types to their respective endpoint
 * context configuration functions.
 */
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

	memset(ep_ctx, 0, sizeof(*ep_ctx));
	setup_ep_ctx_helpers[tt](ep, ep_ctx);
}

/**
 * Clear endpoint halt condition by resetting the endpoint and skipping the
 * offending transfer.
 */
errno_t xhci_endpoint_clear_halt(xhci_endpoint_t *ep, uint32_t stream_id)
{
	errno_t err;

	if ((err = hc_reset_endpoint(ep)))
		return err;

	if ((err = hc_reset_ring(ep, stream_id)))
		return err;

	return EOK;
}

/**
 * @}
 */
