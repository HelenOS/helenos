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
