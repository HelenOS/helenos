/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
