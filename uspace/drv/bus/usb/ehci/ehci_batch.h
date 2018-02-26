/*
 * Copyright (c) 2014 Jan Vesely
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

