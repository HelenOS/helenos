/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
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
