/*
 * SPDX-FileCopyrightText: 2017 Michal Staruch
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief Structures and functions for Superspeed bulk streams.
 */

#ifndef XHCI_STREAMS_H
#define XHCI_STREAMS_H

#include "endpoint.h"
#include "hw_struct/context.h"
#include "trb_ring.h"

typedef struct xhci_endpoint xhci_endpoint_t;
typedef struct xhci_stream_data xhci_stream_data_t;

typedef struct xhci_stream_data {
	/** The TRB ring for the context, if valid */
	xhci_trb_ring_t ring;

	/**
	 * Pointer to the array of secondary stream context data for primary
	 * data.
	 */
	xhci_stream_data_t *secondary_data;

	/** The size of secondary stream context data array */
	uint32_t secondary_size;

	/** Secondary stream context array - allocated for xHC hardware.
	 * Required for later dealocation of secondary structure.
	 */
	xhci_stream_ctx_t *secondary_stream_ctx_array;
	dma_buffer_t secondary_stream_ctx_dma;
} xhci_stream_data_t;

extern xhci_stream_data_t *xhci_get_stream_ctx_data(xhci_endpoint_t *, uint32_t);
extern void xhci_stream_free_ds(xhci_endpoint_t *);

extern errno_t xhci_endpoint_remove_streams(xhci_hc_t *, xhci_device_t *,
    xhci_endpoint_t *);
extern errno_t xhci_endpoint_request_primary_streams(xhci_hc_t *,
    xhci_device_t *, xhci_endpoint_t *, unsigned);
extern errno_t xhci_endpoint_request_secondary_streams(xhci_hc_t *,
    xhci_device_t *, xhci_endpoint_t *, unsigned *, unsigned);

#endif
