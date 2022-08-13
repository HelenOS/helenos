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
 * @brief OHCI driver USB transaction structure
 */
#ifndef DRV_OHCI_BATCH_H
#define DRV_OHCI_BATCH_H

#include <adt/list.h>
#include <assert.h>
#include <stdbool.h>
#include <usb/dma_buffer.h>
#include <usb/host/usb_transfer_batch.h>

#include "hw_struct/transfer_descriptor.h"
#include "hw_struct/endpoint_descriptor.h"

/** OHCI specific data required for USB transfer */
typedef struct ohci_transfer_batch {
	usb_transfer_batch_t base;

	/** Number of TDs used by the transfer */
	size_t td_count;

	/**
	 * List of TDs needed for the transfer - together with setup data
	 * backed by the dma buffer. Note that the TD pointers are pointing to
	 * the DMA buffer initially, but as the scheduling must use the first TD
	 * from EP, it is replaced.
	 */
	td_t **tds;
	char *setup_buffer;
	char *data_buffer;

	dma_buffer_t ohci_dma_buffer;
} ohci_transfer_batch_t;

ohci_transfer_batch_t *ohci_transfer_batch_create(endpoint_t *batch);
int ohci_transfer_batch_prepare(ohci_transfer_batch_t *ohci_batch);
void ohci_transfer_batch_commit(const ohci_transfer_batch_t *batch);
bool ohci_transfer_batch_check_completed(ohci_transfer_batch_t *batch);
void ohci_transfer_batch_destroy(ohci_transfer_batch_t *ohci_batch);

static inline ohci_transfer_batch_t *ohci_transfer_batch_get(
    usb_transfer_batch_t *usb_batch)
{
	assert(usb_batch);

	return (ohci_transfer_batch_t *) usb_batch;
}

#endif
/**
 * @}
 */
