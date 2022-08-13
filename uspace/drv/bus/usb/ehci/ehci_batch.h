/*
 * SPDX-FileCopyrightText: 2014 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver USB transaction structure
 */
#ifndef DRV_EHCI_BATCH_H
#define DRV_EHCI_BATCH_H

#include <adt/list.h>
#include <assert.h>
#include <stdbool.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/dma_buffer.h>

#include "hw_struct/queue_head.h"
#include "hw_struct/transfer_descriptor.h"

/** EHCI specific data required for USB transfer */
typedef struct ehci_transfer_batch {
	usb_transfer_batch_t base;
	/** Number of TDs used by the transfer */
	size_t td_count;
	/** Endpoint descriptor of the target endpoint. */
	qh_t *qh;
	/** Backend for TDs and setup data. */
	dma_buffer_t ehci_dma_buffer;
	/** List of TDs needed for the transfer - backed by dma_buffer */
	td_t *tds;
	/** Data buffers - backed by dma_buffer */
	void *setup_buffer;
	void *data_buffer;
	/** Generic USB transfer structure */
	usb_transfer_batch_t *usb_batch;
} ehci_transfer_batch_t;

ehci_transfer_batch_t *ehci_transfer_batch_create(endpoint_t *ep);
int ehci_transfer_batch_prepare(ehci_transfer_batch_t *batch);
void ehci_transfer_batch_commit(const ehci_transfer_batch_t *batch);
bool ehci_transfer_batch_check_completed(ehci_transfer_batch_t *batch);
void ehci_transfer_batch_destroy(ehci_transfer_batch_t *batch);

static inline ehci_transfer_batch_t *ehci_transfer_batch_get(
    usb_transfer_batch_t *usb_batch)
{
	assert(usb_batch);

	return (ehci_transfer_batch_t *) usb_batch;
}

#endif
/**
 * @}
 */
