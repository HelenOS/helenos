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

/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver USB tranfer helper functions
 */

#ifndef DRV_UHCI_BATCH_H
#define DRV_UHCI_BATCH_H

#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/host/endpoint.h>

#include "hw_struct/queue_head.h"
#include "hw_struct/transfer_descriptor.h"

/** UHCI specific data required for USB transfer */
typedef struct uhci_transfer_batch {
	usb_transfer_batch_t base;

	/** Queue head
	 * This QH is used to maintain UHCI schedule structure and the element
	 * pointer points to the first TD of this batch.
	 */
	qh_t *qh;
	/** List of TDs needed for the transfer */
	td_t *tds;
	/** Number of TDs used by the transfer */
	size_t td_count;
	/* Setup data */
	char *setup_buffer;
	/** Backing TDs + setup_buffer */
	dma_buffer_t uhci_dma_buffer;
	/** List element */
	link_t link;
} uhci_transfer_batch_t;

uhci_transfer_batch_t *uhci_transfer_batch_create(endpoint_t *);
int uhci_transfer_batch_prepare(uhci_transfer_batch_t *);
bool uhci_transfer_batch_check_completed(uhci_transfer_batch_t *);
void uhci_transfer_batch_destroy(uhci_transfer_batch_t *);

/** Get offset to setup buffer accessible to the HC hw.
 * @param uhci_batch UHCI batch structure.
 * @return Pointer to the setup buffer.
 */
static inline void *uhci_transfer_batch_setup_buffer(
    const uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	return uhci_batch->uhci_dma_buffer.virt + sizeof(qh_t) +
	    uhci_batch->td_count * sizeof(td_t);
}

/** Get offset to data buffer accessible to the HC hw.
 * @param uhci_batch UHCI batch structure.
 * @return Pointer to the data buffer.
 */
static inline void *uhci_transfer_batch_data_buffer(
    const uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	return uhci_batch->base.dma_buffer.virt;
}

/** Linked list conversion wrapper.
 * @param l Linked list link.
 * @return Pointer to the uhci batch structure.
 */
static inline uhci_transfer_batch_t *uhci_transfer_batch_from_link(link_t *l)
{
	assert(l);
	return list_get_instance(l, uhci_transfer_batch_t, link);
}

static inline uhci_transfer_batch_t *uhci_transfer_batch_get(
    usb_transfer_batch_t *b)
{
	assert(b);
	return (uhci_transfer_batch_t *) b;
}

#endif

/**
 * @}
 */
