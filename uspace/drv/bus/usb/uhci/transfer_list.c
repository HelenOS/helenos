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
#include <errno.h>
#include <usb/debug.h>
#include <arch/barrier.h>


#include "transfer_list.h"
#include "batch.h"

static void transfer_list_remove_batch(
    transfer_list_t *instance, usb_transfer_batch_t *batch);
/*----------------------------------------------------------------------------*/
/** Initialize transfer list structures.
 *
 * @param[in] instance Memory place to use.
 * @param[in] name Name of the new list.
 * @return Error code
 *
 * Allocates memory for internal qh_t structure.
 */
int transfer_list_init(transfer_list_t *instance, const char *name)
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
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
/** Add transfer batch to the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to submit.
 *
 * The batch is added to the end of the list and queue.
 */
void transfer_list_add_batch(
    transfer_list_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	usb_log_debug2("Queue %s: Adding batch(%p).\n", instance->name, batch);

	fibril_mutex_lock(&instance->guard);

	qh_t *last_qh = NULL;
	/* Add to the hardware queue. */
	if (list_empty(&instance->batch_list)) {
		/* There is nothing scheduled */
		last_qh = instance->queue_head;
	} else {
		/* There is something scheduled */
		usb_transfer_batch_t *last = usb_transfer_batch_from_link(
		    list_last(&instance->batch_list));
		last_qh = batch_qh(last);
	}
	const uint32_t pa = addr_to_phys(batch_qh(batch));
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);

	/* Make sure all data in the batch are written */
	write_barrier();

	/* keep link */
	batch_qh(batch)->next = last_qh->next;
	qh_set_next_qh(last_qh, batch_qh(batch));

	/* Make sure the pointer is updated */
	write_barrier();

	/* Add to the driver's list */
	list_append(&batch->link, &instance->batch_list);

	usb_log_debug("Batch %p " USB_TRANSFER_BATCH_FMT " scheduled in queue %s.\n",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch), instance->name);
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
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
	link_t *current = instance->batch_list.head.next;
	while (current != &instance->batch_list.head) {
		link_t * const next = current->next;
		usb_transfer_batch_t *batch =
		    usb_transfer_batch_from_link(current);

		if (batch_is_complete(batch)) {
			/* Save for processing */
			transfer_list_remove_batch(instance, batch);
			list_append(current, done);
		}
		current = next;
	}
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Walk the list and finish all batches with EINTR.
 *
 * @param[in] instance List to use.
 */
void transfer_list_abort_all(transfer_list_t *instance)
{
	fibril_mutex_lock(&instance->guard);
	while (!list_empty(&instance->batch_list)) {
		link_t * const current = list_first(&instance->batch_list);
		usb_transfer_batch_t *batch =
		    usb_transfer_batch_from_link(current);
		transfer_list_remove_batch(instance, batch);
		usb_transfer_batch_finish_error(batch, EINTR);
	}
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Remove a transfer batch from the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to remove.
 *
 * Does not lock the transfer list, caller is responsible for that.
 */
void transfer_list_remove_batch(
    transfer_list_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(instance->queue_head);
	assert(batch);
	assert(batch_qh(batch));
	assert(fibril_mutex_is_locked(&instance->guard));

	usb_log_debug2(
	    "Queue %s: removing batch(%p).\n", instance->name, batch);

	const char *qpos = NULL;
	qh_t *prev_qh = NULL;
	/* Remove from the hardware queue */
	if (list_first(&instance->batch_list) == &batch->link) {
		/* I'm the first one here */
		prev_qh = instance->queue_head;
		qpos = "FIRST";
	} else {
		/* The thing before me is a batch too */
		usb_transfer_batch_t *prev =
		    usb_transfer_batch_from_link(batch->link.prev);
		prev_qh = batch_qh(prev);
		qpos = "NOT FIRST";
	}
	assert((prev_qh->next & LINK_POINTER_ADDRESS_MASK)
	    == addr_to_phys(batch_qh(batch)));
	prev_qh->next = batch_qh(batch)->next;

	/* Make sure the pointer is updated */
	write_barrier();

	/* Remove from the batch list */
	list_remove(&batch->link);
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " removed (%s) "
	    "from %s, next: %x.\n",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch),
	    qpos, instance->name, batch_qh(batch)->next);
}
/**
 * @}
 */
