/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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

#include "hw_struct/queue_head.h"

/** Connector structure linking ED to to prepared TD. */
typedef struct ehci_endpoint {
	/* Inheritance */
	endpoint_t base;

	/** EHCI endpoint descriptor */
	qh_t *qh;
	/** Linked list used by driver software */
	link_t link;
} ehci_endpoint_t;

typedef struct hc hc_t;

typedef struct {
	usb2_bus_t base;
	hc_t *hc;
} ehci_bus_t;

void ehci_bus_prepare_ops(void);

int ehci_bus_init(ehci_bus_t *, hcd_t *, hc_t *);

/** Get and convert assigned ehci_endpoint_t structure
 * @param[in] ep USBD endpoint structure.
 * @return Pointer to assigned hcd endpoint structure
 */
static inline ehci_endpoint_t * ehci_endpoint_get(const endpoint_t *ep)
{
	assert(ep);
	return (ehci_endpoint_t *) ep;
}

static inline ehci_endpoint_t * ehci_endpoint_list_instance(link_t *l)
{
	return list_get_instance(l, ehci_endpoint_t, link);
}

#endif
/**
 * @}
 */
