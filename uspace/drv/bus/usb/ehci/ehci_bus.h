/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_HCD_BUS_H
#define DRV_EHCI_HCD_BUS_H

#include <assert.h>
#include <adt/list.h>
#include <usb/host/usb2_bus.h>
#include <usb/host/endpoint.h>
#include <usb/dma_buffer.h>

#include "hw_struct/queue_head.h"

/** Connector structure linking ED to to prepared TD. */
typedef struct ehci_endpoint {
	/* Inheritance */
	endpoint_t base;

	/** EHCI endpoint descriptor, backed by dma_buffer */
	qh_t *qh;

	dma_buffer_t dma_buffer;

	/** Link in endpoint_list */
	link_t eplist_link;

	/** Link in pending_endpoints */
	link_t pending_link;
} ehci_endpoint_t;

typedef struct hc hc_t;

typedef struct {
	bus_t base;
	usb2_bus_helper_t helper;
	hc_t *hc;
} ehci_bus_t;

void ehci_ep_toggle_reset(endpoint_t *);
void ehci_bus_prepare_ops(void);

errno_t ehci_bus_init(ehci_bus_t *, hc_t *);

/** Get and convert assigned ehci_endpoint_t structure
 * @param[in] ep USBD endpoint structure.
 * @return Pointer to assigned ehci endpoint structure
 */
static inline ehci_endpoint_t *ehci_endpoint_get(const endpoint_t *ep)
{
	assert(ep);
	return (ehci_endpoint_t *) ep;
}

static inline ehci_endpoint_t *ehci_endpoint_list_instance(link_t *l)
{
	return list_get_instance(l, ehci_endpoint_t, eplist_link);
}

#endif
/**
 * @}
 */
