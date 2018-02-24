/*
 * Copyright (c) 2018 Michal Staruch, Ondrej Hlavaty, Jan Hrach
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
 * @brief Structures and functions for Superspeed bulk streams.
 */

#include "endpoint.h"
#include "hc.h"
#include "hw_struct/regs.h"
#include "streams.h"

/**
 * Finds stream data with given stream ID if it exists.
 * Note that streams with ID 0, 65534 and 65535 are reserved.
 * Splits the ID into primary and secondary context ID and searches the structures.
 * @param[in] ep Affected endpoint.
 * @param[in] stream_id Id of the stream.
 */
xhci_stream_data_t *xhci_get_stream_ctx_data(xhci_endpoint_t *ep, uint32_t stream_id)
{
	if (stream_id == 0 || stream_id >= 65534) {
		return NULL;
	}

	/* See 4.12.2.1 for the calculation of the IDs and dividing the stream_id */
	uint32_t primary_stream_id =
	    (uint32_t) (stream_id & (ep->primary_stream_data_size - 1));
	uint32_t secondary_stream_id =
	    (uint32_t) ((stream_id / ep->primary_stream_data_size) & 0xFF);

	if (primary_stream_id >= ep->primary_stream_data_size) {
		return NULL;
	}

	xhci_stream_data_t *primary_data =
	    &ep->primary_stream_data_array[primary_stream_id];
	if (secondary_stream_id != 0 && !primary_data->secondary_size) {
		return NULL;
	}

	if (!primary_data->secondary_size) {
		return primary_data;
	}

	xhci_stream_data_t *secondary_data = primary_data->secondary_data;
	if (secondary_stream_id >= primary_data->secondary_size) {
		return NULL;
	}

	return &secondary_data[secondary_stream_id];
}

/**
 * Initializes primary stream data structures in endpoint.
 * @param[in] xhci_ep Used XHCI bulk endpoint.
 * @param[in] count Amount of primary streams.
 */
static errno_t initialize_primary_structures(xhci_endpoint_t *xhci_ep, unsigned count)
{
	usb_log_debug("Allocating primary stream context array of size %u "
		"for endpoint " XHCI_EP_FMT, count, XHCI_EP_ARGS(*xhci_ep));

	if ((dma_buffer_alloc(&xhci_ep->primary_stream_ctx_dma,
		count * sizeof(xhci_stream_ctx_t)))) {
		return ENOMEM;
	}

	xhci_ep->primary_stream_ctx_array = xhci_ep->primary_stream_ctx_dma.virt;
	xhci_ep->primary_stream_data_array = calloc(count, sizeof(xhci_stream_data_t));
	if (!xhci_ep->primary_stream_data_array) {
		dma_buffer_free(&xhci_ep->primary_stream_ctx_dma);
		return ENOMEM;
	}

	xhci_ep->primary_stream_data_size = count;

	return EOK;
}

static void clear_primary_structures(xhci_endpoint_t *xhci_ep)
{
	usb_log_debug("Deallocating primary stream structures for "
		"endpoint " XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

	dma_buffer_free(&xhci_ep->primary_stream_ctx_dma);
	free(xhci_ep->primary_stream_data_array);

	xhci_ep->primary_stream_data_array = NULL;
	xhci_ep->primary_stream_data_size = 0;
}

static void clear_secondary_streams(xhci_endpoint_t *xhci_ep, unsigned index)
{
	xhci_stream_data_t *data = &xhci_ep->primary_stream_data_array[index];
	if (!data->secondary_size) {
		xhci_trb_ring_fini(&data->ring);
		return;
	}

	for (size_t i = 0; i < data->secondary_size; ++i) {
		xhci_trb_ring_fini(&data->secondary_data[i].ring);
	}

	dma_buffer_free(&data->secondary_stream_ctx_dma);
	free(data->secondary_data);
}

void xhci_stream_free_ds(xhci_endpoint_t *xhci_ep)
{
	usb_log_debug("Freeing stream rings and context arrays of endpoint "
		XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));

	for (size_t index = 0; index < xhci_ep->primary_stream_data_size; ++index) {
		clear_secondary_streams(xhci_ep, index);
	}
	clear_primary_structures(xhci_ep);
}

/**
 * Initialize a single primary stream structure with given index.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] xhci_ep XHCI bulk endpoint to use.
 * @param[in] index index of the initialized stream structure.
 */
static errno_t initialize_primary_stream(xhci_hc_t *hc, xhci_endpoint_t *xhci_ep,
	unsigned index)
{
	xhci_stream_ctx_t *ctx = &xhci_ep->primary_stream_ctx_array[index];
	xhci_stream_data_t *data = &xhci_ep->primary_stream_data_array[index];
	memset(data, 0, sizeof(xhci_stream_data_t));

	errno_t err = EOK;

	/* Init and register TRB ring for the primary stream */
	if ((err = xhci_trb_ring_init(&data->ring, 0))) {
		return err;
	}
	XHCI_STREAM_DEQ_PTR_SET(*ctx, data->ring.dequeue);

	/* Set to linear stream array */
	XHCI_STREAM_SCT_SET(*ctx, 1);

	return EOK;
}

/**
 * Initialize primary streams of XHCI bulk endpoint.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] xhci_ep XHCI bulk endpoint to use.
 */
static errno_t initialize_primary_streams(xhci_hc_t *hc, xhci_endpoint_t *xhci_ep)
{
	errno_t err = EOK;
	size_t index;
	for (index = 0; index < xhci_ep->primary_stream_data_size; ++index) {
		err = initialize_primary_stream(hc, xhci_ep, index);
		if (err) {
			goto err_clean;
		}
	}

	return EOK;

err_clean:
	for (size_t i = 0; i < index; ++i) {
		xhci_trb_ring_fini(&xhci_ep->primary_stream_data_array[i].ring);
	}
	return err;
}

/**
 * Initialize secondary streams of XHCI bulk endpoint.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] xhci_epi XHCI bulk endpoint to use.
 * @param[in] idx Index to primary stream array
 * @param[in] count Number of secondary streams to initialize.
 */
static errno_t initialize_secondary_streams(xhci_hc_t *hc, xhci_endpoint_t *xhci_ep,
	unsigned idx, unsigned count)
{
	if (count == 0) {
		/*
		 * The primary stream context can still point to a single ring, not
		 * a secondary.
		 */
		return initialize_primary_stream(hc, xhci_ep, idx);
	}

	if ((count & (count - 1)) != 0 || count < 8 || count > 256) {
		usb_log_error("The secondary stream array size must be a power of 2 "
			"between 8 and 256.");
		return EINVAL;
	}

	xhci_stream_ctx_t *ctx = &xhci_ep->primary_stream_ctx_array[idx];
	xhci_stream_data_t *data = &xhci_ep->primary_stream_data_array[idx];
	memset(data, 0, sizeof(xhci_stream_data_t));

	data->secondary_size = count;
	data->secondary_data = calloc(count, sizeof(xhci_stream_data_t));
	if (!data->secondary_size) {
		return ENOMEM;
	}

	if ((dma_buffer_alloc(&data->secondary_stream_ctx_dma,
		count * sizeof(xhci_stream_ctx_t)))) {
		free(data->secondary_data);
		return ENOMEM;
	}
	data->secondary_stream_ctx_array = data->secondary_stream_ctx_dma.virt;

	XHCI_STREAM_DEQ_PTR_SET(*ctx, dma_buffer_phys_base(&data->secondary_stream_ctx_dma));
	XHCI_STREAM_SCT_SET(*ctx, fnzb32(count) + 1);

	/* Initialize all the rings. */
	errno_t err = EOK;
	size_t index;
	for (index = 0; index < count; ++index) {
		xhci_stream_ctx_t *secondary_ctx = &data->secondary_stream_ctx_array[index];
		xhci_stream_data_t *secondary_data = &data->secondary_data[index];
		/* Init and register TRB ring for every secondary stream */
		if ((err = xhci_trb_ring_init(&secondary_data->ring, 0))) {
			goto err_init;
		}

		XHCI_STREAM_DEQ_PTR_SET(*secondary_ctx, secondary_data->ring.dequeue);
		/* Set to secondary stream array */
		XHCI_STREAM_SCT_SET(*secondary_ctx, 0);
	}

	return EOK;

err_init:
	for (size_t i = 0; i < index; ++i) {
		xhci_trb_ring_fini(&data->secondary_data[i].ring);
	}
	return err;
}

/**
 * Configure XHCI bulk endpoint's stream context.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 * @param[in] ctx Endpoint context to configure.
 * @param[in] pstreams The value of MaxPStreams.
 * @param[in] lsa Specifies if the stream IDs point to primary stream array.
 */
static void setup_stream_context(xhci_endpoint_t *xhci_ep, xhci_ep_ctx_t *ctx,
	unsigned pstreams, unsigned lsa)
{
	XHCI_EP_TYPE_SET(*ctx, xhci_endpoint_type(xhci_ep));
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, xhci_ep->base.max_packet_size);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, xhci_ep->max_burst - 1);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);

	XHCI_EP_MAX_P_STREAMS_SET(*ctx, pstreams);
	XHCI_EP_TR_DPTR_SET(*ctx, dma_buffer_phys_base(&xhci_ep->primary_stream_ctx_dma));
	XHCI_EP_LSA_SET(*ctx, lsa);
}

/**
 * Verifies if all the common preconditions are satisfied.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] dev Used device.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 * @param[in] count Amount of primary streams requested.
 */
static errno_t verify_stream_conditions(xhci_hc_t *hc, xhci_device_t *dev,
	xhci_endpoint_t *xhci_ep, unsigned count)
{
	if (xhci_ep->base.transfer_type != USB_TRANSFER_BULK
		|| dev->base.speed != USB_SPEED_SUPER) {
		usb_log_error("Streams are only supported by superspeed bulk endpoints.");
		return EINVAL;
	}

	if (xhci_ep->max_streams <= 1) {
		usb_log_error("Streams are not supported by endpoint "
		    XHCI_EP_FMT, XHCI_EP_ARGS(*xhci_ep));
		return EINVAL;
	}

	if (count < 2) {
		usb_log_error("The minumum amount of primary streams is 2.");
		return EINVAL;
	}

	/*
	 * The maximum amount of primary streams is 2 ^ (MaxPSA + 1)
	 * See table 26 of XHCI specification.
	 */
	uint8_t max_psa_size = 1 << (XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PSA_SIZE) + 1);
	if (count > max_psa_size) {
		usb_log_error("Host controller only supports "
			"%u primary streams.", max_psa_size);
		return EINVAL;
	}

	if (count > xhci_ep->max_streams) {
		usb_log_error("Endpoint " XHCI_EP_FMT " supports only %" PRIu32 " streams.",
			XHCI_EP_ARGS(*xhci_ep), xhci_ep->max_streams);
		return EINVAL;
	}

	if ((count & (count - 1)) != 0) {
		usb_log_error("The amount of primary streams must be a power of 2.");
		return EINVAL;
	}

	return EOK;
}

/**
 * Cancels streams and reconfigures endpoint back to single ring no stream endpoint.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] dev Used device.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 */
errno_t xhci_endpoint_remove_streams(xhci_hc_t *hc, xhci_device_t *dev,
	xhci_endpoint_t *xhci_ep)
{
	if (!xhci_ep->primary_stream_data_size) {
		usb_log_warning("There are no streams enabled on the endpoint, doing nothing.");
		return EOK;
	}

	hc_stop_endpoint(xhci_ep);
	xhci_endpoint_free_transfer_ds(xhci_ep);

	/* Streams are now removed, proceed with reconfiguring endpoint. */
	errno_t err;
	if ((err = xhci_trb_ring_init(&xhci_ep->ring, 0))) {
		usb_log_error("Failed to initialize a transfer ring.");
		return err;
	}

	return hc_update_endpoint(xhci_ep);
}

/**
 * Initialize, setup and register primary streams.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] dev Used device.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 * @param[in] count Amount of primary streams requested.
 */
errno_t xhci_endpoint_request_primary_streams(xhci_hc_t *hc, xhci_device_t *dev,
	xhci_endpoint_t *xhci_ep, unsigned count)
{
	errno_t err = verify_stream_conditions(hc, dev, xhci_ep, count);
	if (err) {
		return err;
	}

	/*
	 * We have passed the checks.
	 * Stop the endpoint, destroy the ring, and transition to streams.
	 */
	hc_stop_endpoint(xhci_ep);
	xhci_endpoint_free_transfer_ds(xhci_ep);

	err = initialize_primary_structures(xhci_ep, count);
	if (err) {
		return err;
	}

	memset(xhci_ep->primary_stream_ctx_array, 0, count * sizeof(xhci_stream_ctx_t));
	err = initialize_primary_streams(hc, xhci_ep);
	if (err) {
		clear_primary_structures(xhci_ep);
		return err;
	}

	xhci_ep_ctx_t ep_ctx;
	/*
	 * Allowed values are 1-15, where 2 ^ pstreams is the actual amount of
	 * streams.
	 */
	const size_t pstreams = fnzb32(count) - 1;
	setup_stream_context(xhci_ep, &ep_ctx, pstreams, 1);

	return hc_update_endpoint(xhci_ep);
}

/**
 * Initialize, setup and register secondary streams.
 * @param[in] hc Host controller of the endpoint.
 * @param[in] dev Used device.
 * @param[in] xhci_ep Associated XHCI bulk endpoint.
 * @param[in] sizes Amount of secondary streams in each of the primary streams.
 *                  This array should have exactly count elements. If the size
 *                  is 0, then a primary ring is created with that index.
 * @param[in] count Amount of primary streams requested.
 */
errno_t xhci_endpoint_request_secondary_streams(xhci_hc_t *hc, xhci_device_t *dev,
	xhci_endpoint_t *xhci_ep, unsigned *sizes, unsigned count)
{
	/* Check if HC supports secondary indexing */
	if (XHCI_REG_RD(hc->cap_regs, XHCI_CAP_NSS)) {
		usb_log_error("The host controller doesn't support secondary streams.");
		return ENOTSUP;
	}

	errno_t err = verify_stream_conditions(hc, dev, xhci_ep, count);
	if (err) {
		return err;
	}

	if (count > 256) {
		usb_log_error("The amount of primary streams cannot be higher than 256.");
		return EINVAL;
	}

	/*
	 * Find the largest requested secondary stream size,
	 * that one is the maximum ID that device can receive.
	 * We need to make sure the device can handle that ID.
	 */
	unsigned max = 0;
	for (size_t index = 0; index < count; ++index) {
		if (sizes[count] > max) {
			max = sizes[count];
		}
	}

	if (max * count > xhci_ep->max_streams) {
		usb_log_error("Endpoint " XHCI_EP_FMT " supports only %" PRIu32 " streams.",
			XHCI_EP_ARGS(*xhci_ep), xhci_ep->max_streams);
		return EINVAL;
	}

	/*
	 * We have passed all checks.
	 * Stop the endpoint, destroy the ring, and transition to streams.
	 */
	hc_stop_endpoint(xhci_ep);
	xhci_endpoint_free_transfer_ds(xhci_ep);

	err = initialize_primary_structures(xhci_ep, count);
	if (err) {
		return err;
	}

	memset(xhci_ep->primary_stream_ctx_array, 0, count * sizeof(xhci_stream_ctx_t));
	size_t index;
	for (index = 0; index < count; ++index) {
		err = initialize_secondary_streams(hc, xhci_ep, index, *(sizes + index));
		if (err) {
			goto err_init;
		}
	}

	xhci_ep_ctx_t ep_ctx;
	const size_t pstreams = fnzb32(count) - 1;
	setup_stream_context(xhci_ep, &ep_ctx, pstreams, 0);

	return hc_update_endpoint(xhci_ep);

err_init:
	for (size_t i = 0; i < index; ++i) {
		clear_secondary_streams(xhci_ep, i);
	}
	clear_primary_structures(xhci_ep);
	return err;
}
