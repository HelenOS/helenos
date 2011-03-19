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

#include "transfer_list.h"

static void transfer_list_remove_batch(
    transfer_list_t *instance, batch_t *batch);
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
	instance->queue_head_pa = addr_to_phys(instance->queue_head);

	qh_init(instance->queue_head);
	list_initialize(&instance->batch_list);
	fibril_mutex_initialize(&instance->guard);
	return EOK;
}
/*----------------------------------------------------------------------------*/
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
	assert(next);
	if (!instance->queue_head)
		return;
	/* Set both queue_head.next to point to the follower */
	qh_set_next_qh(instance->queue_head, next->queue_head_pa);
}
/*----------------------------------------------------------------------------*/
/** Submit transfer batch to the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to submit.
 * @return Error code
 *
 * The batch is added to the end of the list and queue.
 */
void transfer_list_add_batch(transfer_list_t *instance, batch_t *batch)
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
		batch_t *last = list_get_instance(
		    instance->batch_list.prev, batch_t, link);
		last_qh = last->qh;
	}
	const uint32_t pa = addr_to_phys(batch->qh);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);

	batch->qh->next = last_qh->next;
	qh_set_next_qh(last_qh, pa);

	/* Add to the driver list */
	list_append(&batch->link, &instance->batch_list);

	batch_t *first = list_get_instance(
	    instance->batch_list.next, batch_t, link);
	usb_log_debug("Batch(%p) added to queue %s, first is %p.\n",
		batch, instance->name, first);
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Check list for finished batches.
 *
 * @param[in] instance List to use.
 * @return Error code
 *
 * Creates a local list of finished batches and calls next_step on each and
 * every one. This is safer because next_step may theoretically access
 * this transfer list leading to the deadlock if its done inline.
 */
void transfer_list_remove_finished(transfer_list_t *instance)
{
	assert(instance);

	LIST_INITIALIZE(done);

	fibril_mutex_lock(&instance->guard);
	link_t *current = instance->batch_list.next;
	while (current != &instance->batch_list) {
		link_t *next = current->next;
		batch_t *batch = list_get_instance(current, batch_t, link);

		if (batch_is_complete(batch)) {
			/* Save for post-processing */
			transfer_list_remove_batch(instance, batch);
			list_append(current, &done);
		}
		current = next;
	}
	fibril_mutex_unlock(&instance->guard);

	while (!list_empty(&done)) {
		link_t *item = done.next;
		list_remove(item);
		batch_t *batch = list_get_instance(item, batch_t, link);
		batch->next_step(batch);
	}
}
/*----------------------------------------------------------------------------*/
/** Walk the list and abort all batches.
 *
 * @param[in] instance List to use.
 */
void transfer_list_abort_all(transfer_list_t *instance)
{
	fibril_mutex_lock(&instance->guard);
	while (!list_empty(&instance->batch_list)) {
		link_t *current = instance->batch_list.next;
		batch_t *batch = list_get_instance(current, batch_t, link);
		transfer_list_remove_batch(instance, batch);
		batch_abort(batch);
	}
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Remove a transfer batch from the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] batch Transfer batch to remove.
 * @return Error code
 *
 * Does not lock the transfer list, caller is responsible for that.
 */
void transfer_list_remove_batch(transfer_list_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);
	assert(instance->queue_head);
	assert(batch->qh);
	usb_log_debug2(
	    "Queue %s: removing batch(%p).\n", instance->name, batch);

	const char * pos = NULL;
	/* Remove from the hardware queue */
	if (batch->link.prev == &instance->batch_list) {
		/* I'm the first one here */
		instance->queue_head->next = batch->qh->next;
		pos = "FIRST";
	} else {
		batch_t *prev =
		    list_get_instance(batch->link.prev, batch_t, link);
		prev->qh->next = batch->qh->next;
		pos = "NOT FIRST";
	}
	/* Remove from the driver list */
	list_remove(&batch->link);
	usb_log_debug("Batch(%p) removed (%s) from %s, next element %x.\n",
	    batch, pos, instance->name, batch->qh->next);
}
/**
 * @}
 */
