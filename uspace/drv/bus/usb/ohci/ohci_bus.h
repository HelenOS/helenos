/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_HCD_BUS_H
#define DRV_OHCI_HCD_BUS_H

#include <assert.h>
#include <adt/list.h>
#include <usb/dma_buffer.h>
#include <usb/host/usb2_bus.h>

#include "hw_struct/endpoint_descriptor.h"
#include "hw_struct/transfer_descriptor.h"

/**
 * Connector structure linking ED to to prepared TD.
 *
 * OHCI requires new transfers to be appended at the end of a queue. But it has
 * a weird semantics of a leftover TD, which serves as a placeholder. This left
 * TD is overwritten with first TD of a new transfer, and the spare one is used
 * as the next placeholder. Then the two are swapped for the next transaction.
 */
typedef struct ohci_endpoint {
	endpoint_t base;

	/** OHCI endpoint descriptor */
	ed_t *ed;
	/** TDs to be used at the beginning and end of transaction */
	td_t *tds [2];

	/** Buffer to back ED + 2 TD */
	dma_buffer_t dma_buffer;

	/** Link in endpoint_list */
	link_t eplist_link;
	/** Link in pending_endpoints */
	link_t pending_link;
} ohci_endpoint_t;

typedef struct hc hc_t;

typedef struct {
	bus_t base;
	usb2_bus_helper_t helper;
	hc_t *hc;
} ohci_bus_t;

errno_t ohci_bus_init(ohci_bus_t *, hc_t *);
void ohci_ep_toggle_reset(endpoint_t *);

/** Get and convert assigned ohci_endpoint_t structure
 * @param[in] ep USBD endpoint structure.
 * @return Pointer to assigned hcd endpoint structure
 */
static inline ohci_endpoint_t *ohci_endpoint_get(const endpoint_t *ep)
{
	assert(ep);
	return (ohci_endpoint_t *) ep;
}

#endif
/**
 * @}
 */
