/*
 * Copyright (c) 2011 Jan Vesely
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

#include <usb/host/usb_transfer_batch.h>
#include <adt/list.h>

#include "hw_struct/queue_head.h"
#include "hw_struct/transfer_descriptor.h"

/** UHCI specific data required for USB transfer */
typedef struct uhci_transfer_batch {
	/** Queue head
	 * This QH is used to maintain UHCI schedule structure and the element
	 * pointer points to the first TD of this batch.
	 */
	qh_t *qh;
	/** List of TDs needed for the transfer */
	td_t *tds;
	/** Number of TDs used by the transfer */
	size_t td_count;
	/** Data buffer, must be accessible by the UHCI hw */
	void *device_buffer;
	/** Generic transfer data */
	usb_transfer_batch_t *usb_batch;
	/** List element */
	link_t link;
} uhci_transfer_batch_t;

uhci_transfer_batch_t * uhci_transfer_batch_get(usb_transfer_batch_t *batch);
void uhci_transfer_batch_finish_dispose(uhci_transfer_batch_t *uhci_batch);
bool uhci_transfer_batch_is_complete(const uhci_transfer_batch_t *uhci_batch);

/** Get offset to setup buffer accessible to the HC hw.
 * @param uhci_batch UHCI batch structure.
 * @return Pointer to the setup buffer.
 */
static inline void * uhci_transfer_batch_setup_buffer(
    const uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	assert(uhci_batch->device_buffer);
	return uhci_batch->device_buffer + sizeof(qh_t) +
	    uhci_batch->td_count * sizeof(td_t);
}

/** Get offset to data buffer accessible to the HC hw.
 * @param uhci_batch UHCI batch structure.
 * @return Pointer to the data buffer.
 */
static inline void * uhci_transfer_batch_data_buffer(
    const uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);
	return uhci_transfer_batch_setup_buffer(uhci_batch) +
	    uhci_batch->usb_batch->setup_size;
}

/** Aborts the batch.
 * Sets error to EINTR and size off transferd data to 0, before finishing the
 * batch.
 * @param uhci_batch Batch to abort.
 */
static inline void uhci_transfer_batch_abort(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);
	uhci_batch->usb_batch->error = EINTR;
	uhci_batch->usb_batch->transfered_size = 0;
	uhci_transfer_batch_finish_dispose(uhci_batch);
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

#endif
/**
 * @}
 */
