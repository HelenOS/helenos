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

	/** Link in endpoint_list*/
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
