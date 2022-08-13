/*
 * SPDX-FileCopyrightText: 2018 Petr Manek, Ondrej Hlavaty, Michal Staruch, Jan Hrach
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller endpoint management.
 */

#ifndef XHCI_ENDPOINT_H
#define XHCI_ENDPOINT_H

#include <assert.h>

#include <usb/debug.h>
#include <usb/dma_buffer.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>
#include <ddf/driver.h>

#include "device.h"
#include "isoch.h"
#include "transfers.h"
#include "trb_ring.h"

typedef struct xhci_device xhci_device_t;
typedef struct xhci_endpoint xhci_endpoint_t;
typedef struct xhci_stream_data xhci_stream_data_t;
typedef struct xhci_bus xhci_bus_t;

enum {
	EP_TYPE_INVALID = 0,
	EP_TYPE_ISOCH_OUT = 1,
	EP_TYPE_BULK_OUT = 2,
	EP_TYPE_INTERRUPT_OUT = 3,
	EP_TYPE_CONTROL = 4,
	EP_TYPE_ISOCH_IN = 5,
	EP_TYPE_BULK_IN = 6,
	EP_TYPE_INTERRUPT_IN = 7
};

/** Connector structure linking endpoint context to the endpoint. */
typedef struct xhci_endpoint {
	endpoint_t base;	/**< Inheritance. Keep this first. */

	/** Guarding scheduling of this endpoint. */
	fibril_mutex_t guard;

	/** Main transfer ring (unused if streams are enabled) */
	xhci_trb_ring_t ring;

	/**
	 * Primary stream context data array
	 * (or NULL if endpoint doesn't use streams).
	 */
	xhci_stream_data_t *primary_stream_data_array;

	/** Primary stream context array - allocated for xHC hardware. */
	xhci_stream_ctx_t *primary_stream_ctx_array;
	dma_buffer_t primary_stream_ctx_dma;

	/** Size of the allocated primary stream data and context array. */
	uint16_t primary_stream_data_size;

	/* Maximum number of primary streams (0 - 2^16). */
	uint32_t max_streams;

	/**
	 * Maximum number of consecutive USB transactions (0-15) that
	 * should be executed per scheduling opportunity
	 */
	uint8_t max_burst;

	/**
	 * Maximum number of bursts within an interval that
	 * this endpoint supports
	 */
	uint8_t mult;

	/**
	 * Scheduling interval for periodic endpoints,
	 * as a number of 125us units. (0 - 2^16)
	 */
	uint32_t interval;

	/**
	 * This field is a valid pointer for (and only for) isochronous
	 * endpoints.
	 */
	xhci_isoch_t isoch [0];
} xhci_endpoint_t;

#define XHCI_EP_FMT  "(%d:%d %s)"
/* FIXME: "Device -1" messes up log messages, figure out a better way. */
#define XHCI_EP_ARGS(ep)		\
	((ep).base.device ? (ep).base.device->address : -1),	\
	((ep).base.endpoint),		\
	(usb_str_transfer_type((ep).base.transfer_type))

extern int xhci_endpoint_type(xhci_endpoint_t *ep);

extern endpoint_t *xhci_endpoint_create(device_t *,
    const usb_endpoint_descriptors_t *);
extern errno_t xhci_endpoint_register(endpoint_t *);
extern void xhci_endpoint_unregister(endpoint_t *);
extern void xhci_endpoint_destroy(endpoint_t *);

extern void xhci_endpoint_free_transfer_ds(xhci_endpoint_t *);
extern xhci_trb_ring_t *xhci_endpoint_get_ring(xhci_endpoint_t *, uint32_t);

extern void xhci_setup_endpoint_context(xhci_endpoint_t *, xhci_ep_ctx_t *);
extern errno_t xhci_endpoint_clear_halt(xhci_endpoint_t *, unsigned);

static inline xhci_endpoint_t *xhci_endpoint_get(endpoint_t *ep)
{
	assert(ep);
	return (xhci_endpoint_t *) ep;
}

static inline xhci_device_t *xhci_ep_to_dev(xhci_endpoint_t *ep)
{
	assert(ep);
	return xhci_device_get(ep->base.device);
}

#endif

/**
 * @}
 */
