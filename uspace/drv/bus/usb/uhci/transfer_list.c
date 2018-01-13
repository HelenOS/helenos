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
 * @brief UHCI driver transfer list implementation
 */

#include <assert.h>
#include <errno.h>
#include <libarch/barrier.h>
#include <stdint.h>
#include <usb/debug.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/host/utils/malloc32.h>

#include "hw_struct/link_pointer.h"
#include "transfer_list.h"

static void transfer_list_remove_batch(
    transfer_list_t *instance, uhci_transfer_batch_t *uhci_batch);

/** Initialize transfer list structures.
 *
 * @param[in] instance Memory place to use.
 * @param[in] name Name of the new list.
 * @return Error code
 *
 * Allocates memory for internal qh_t structure.
 */
errno_t transfer_list_init(transfer_list_t *instance, const char *name)
{
	assert(instance);
	instance->name = name;
	instance->queue_head = malloc32(sizeof(qh_t));
	if (!instance->queue_head) {
		usb_log_error("Failed to allocate queue head.\n");
		return ENOMEM;
	}
	const uint32_t queue_head_pa = addr_to_phys(instance->queue_head);
	usb_log_debug2("Transfer list %s setup with QH: %p (%#" PRIx32" ).\n",
	    name, instance->queue_head, queue_head_pa);

	qh_init(instance->queue_head);
	list_initialize(&instance->batch_list);
	fibril_mutex_initialize(&instance->guard);
	return EOK;
}

/** Dispose transfer list structures.
 *
 * @param[in] instance Memory place to use.
 *
 * Frees memory of the internal qh_t structure.
 */
void transfer_list_fini(transfer_list_t *instance)
{
	assert(instance);
	free32(instance->queue_head);
}
/** Set the next list in transfer list chain.
 *
 * @param[in] instance List to lead.
 * @param[in] next List to append.
 * @return Error code
 *
 * Does not check whether this replaces an existing list .
 */
void transfer_list_set_next(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	assert(instance->queue_head);
	assert(next);
	/* Set queue_head.next to point to the follower */
	qh_set_next_qh(instance->queue_head, next->queue_head);
}

/** Add transfer batch to the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to submit.
 *
 * The batch is added to the end of the list and queue.
 */
void transfer_list_add_batch(
    transfer_list_t *instance, uhci_transfer_batch_t *uhci_batch)
{
	assert(instance);
	assert(uhci_batch);
	usb_log_debug2("Batch %p adding to queue %s.\n",
	    uhci_batch->usb_batch, instance->name);

	fibril_mutex_lock(&instance->guard);

	/* Assume there is nothing scheduled */
	qh_t *last_qh = instance->queue_head;
	/* There is something scheduled */
	if (!list_empty(&instance->batch_list)) {
		last_qh = uhci_transfer_batch_from_link(
		    list_last(&instance->batch_list))->qh;
	}
	/* Add to the hardware queue. */
	const uint32_t pa = addr_to_phys(uhci_batch->qh);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);

	/* Make sure all data in the batch are written */
	write_barrier();

	/* keep link */
	uhci_batch->qh->next = last_qh->next;
	qh_set_next_qh(last_qh, uhci_batch->qh);

	/* Make sure the pointer is updated */
	write_barrier();

	/* Add to the driver's list */
	list_append(&uhci_batch->link, &instance->batch_list);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " scheduled in queue %s.\n", uhci_batch->usb_batch,
	    USB_TRANSFER_BATCH_ARGS(*uhci_batch->usb_batch), instance->name);
	fibril_mutex_unlock(&instance->guard);
}

/** Add completed batches to the provided list.
 *
 * @param[in] instance List to use.
 * @param[in] done list to fill
 */
void transfer_list_remove_finished(transfer_list_t *instance, list_t *done)
{
	assert(instance);
	assert(done);

	fibril_mutex_lock(&instance->guard);
	link_t *current = list_first(&instance->batch_list);
	while (current && current != &instance->batch_list.head) {
		link_t * const next = current->next;
		uhci_transfer_batch_t *batch =
		    uhci_transfer_batch_from_link(current);

		if (uhci_transfer_batch_is_complete(batch)) {
			/* Save for processing */
			transfer_list_remove_batch(instance, batch);
			list_append(current, done);
		}
		current = next;
	}
	fibril_mutex_unlock(&instance->guard);
}

/** Walk the list and finish all batches with EINTR.
 *
 * @param[in] instance List to use.
 */
void transfer_list_abort_all(transfer_list_t *instance)
{
	fibril_mutex_lock(&instance->guard);
	while (!list_empty(&instance->batch_list)) {
		link_t * const current = list_first(&instance->batch_list);
		uhci_transfer_batch_t *batch =
		    uhci_transfer_batch_from_link(current);
		transfer_list_remove_batch(instance, batch);
		uhci_transfer_batch_abort(batch);
	}
	fibril_mutex_unlock(&instance->guard);
}

/** Remove a transfer batch from the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to remove.
 *
 * Does not lock the transfer list, caller is responsible for that.
 */
void transfer_list_remove_batch(
    transfer_list_t *instance, uhci_transfer_batch_t *uhci_batch)
{
	assert(instance);
	assert(instance->queue_head);
	assert(uhci_batch);
	assert(uhci_batch->qh);
	assert(fibril_mutex_is_locked(&instance->guard));

	usb_log_debug2("Batch %p removing from queue %s.\n",
	    uhci_batch->usb_batch, instance->name);

	/* Assume I'm the first */
	const char *qpos = "FIRST";
	qh_t *prev_qh = instance->queue_head;
	/* Remove from the hardware queue */
	if (list_first(&instance->batch_list) != &uhci_batch->link) {
		/* There is a batch in front of me */
		prev_qh =
		    uhci_transfer_batch_from_link(uhci_batch->link.prev)->qh;
		qpos = "NOT FIRST";
	}
	assert((prev_qh->next & LINK_POINTER_ADDRESS_MASK)
	    == addr_to_phys(uhci_batch->qh));
	prev_qh->next = uhci_batch->qh->next;

	/* Make sure the pointer is updated */
	write_barrier();

	/* Remove from the batch list */
	list_remove(&uhci_batch->link);
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " removed (%s) "
	    "from %s, next: %x.\n", uhci_batch->usb_batch,
	    USB_TRANSFER_BATCH_ARGS(*uhci_batch->usb_batch),
	    qpos, instance->name, uhci_batch->qh->next);
}
/**
 * @}
 */
