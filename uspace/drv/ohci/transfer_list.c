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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver transfer list implementation
 */
#include <errno.h>
#include <usb/debug.h>

#include "transfer_list.h"

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
	instance->list_head = malloc32(sizeof(ed_t));
	if (!instance->list_head) {
		usb_log_error("Failed to allocate list head.\n");
		return ENOMEM;
	}
	instance->list_head_pa = addr_to_phys(instance->list_head);
	usb_log_debug2("Transfer list %s setup with ED: %p(%p).\n",
	    name, instance->list_head, instance->list_head_pa);

	ed_init(instance->list_head, NULL);
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
	ed_append_ed(instance->list_head, next->list_head);
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
void transfer_list_add_batch(
    transfer_list_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	usb_log_debug2("Queue %s: Adding batch(%p).\n", instance->name, batch);

	fibril_mutex_lock(&instance->guard);

	ed_t *last_ed = NULL;
	/* Add to the hardware queue. */
	if (list_empty(&instance->batch_list)) {
		/* There is nothing scheduled */
		last_ed = instance->list_head;
	} else {
		/* There is something scheduled */
		usb_transfer_batch_t *last = list_get_instance(
		    instance->batch_list.prev, usb_transfer_batch_t, link);
		last_ed = batch_ed(last);
	}
	/* keep link */
	batch_ed(batch)->next = last_ed->next;
	ed_append_ed(last_ed, batch_ed(batch));

	asm volatile ("": : :"memory");

	/* Add to the driver list */
	list_append(&batch->link, &instance->batch_list);

	usb_transfer_batch_t *first = list_get_instance(
	    instance->batch_list.next, usb_transfer_batch_t, link);
	usb_log_debug("Batch(%p) added to list %s, first is %p(%p).\n",
		batch, instance->name, first, batch_ed(first));
	if (last_ed == instance->list_head) {
		usb_log_debug2("%s head ED(%p-%p): %x:%x:%x:%x.\n",
		    instance->name, last_ed, instance->list_head_pa,
		    last_ed->status, last_ed->td_tail, last_ed->td_head,
		    last_ed->next);
	}
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Create list for finished batches.
 *
 * @param[in] instance List to use.
 * @param[in] done list to fill
 */
void transfer_list_remove_finished(transfer_list_t *instance, link_t *done)
{
	assert(instance);
	assert(done);

	fibril_mutex_lock(&instance->guard);
	usb_log_debug2("Checking list %s for completed batches(%d).\n",
	    instance->name, list_count(&instance->batch_list));
	link_t *current = instance->batch_list.next;
	while (current != &instance->batch_list) {
		link_t *next = current->next;
		usb_transfer_batch_t *batch =
		    list_get_instance(current, usb_transfer_batch_t, link);

		if (batch_is_complete(batch)) {
			/* Save for post-processing */
			transfer_list_remove_batch(instance, batch);
			list_append(current, done);
		}
		current = next;
	}
	fibril_mutex_unlock(&instance->guard);
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
		usb_transfer_batch_t *batch =
		    list_get_instance(current, usb_transfer_batch_t, link);
		transfer_list_remove_batch(instance, batch);
		usb_transfer_batch_finish_error(batch, EIO);
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
void transfer_list_remove_batch(
    transfer_list_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(instance->list_head);
	assert(batch);
	assert(batch_ed(batch));
	assert(fibril_mutex_is_locked(&instance->guard));

	usb_log_debug2(
	    "Queue %s: removing batch(%p).\n", instance->name, batch);

	const char *qpos = NULL;
	/* Remove from the hardware queue */
	if (instance->batch_list.next == &batch->link) {
		/* I'm the first one here */
		assert((instance->list_head->next & ED_NEXT_PTR_MASK)
		    == addr_to_phys(batch_ed(batch)));
		instance->list_head->next = batch_ed(batch)->next;
		qpos = "FIRST";
	} else {
		usb_transfer_batch_t *prev =
		    list_get_instance(
		        batch->link.prev, usb_transfer_batch_t, link);
		assert((batch_ed(prev)->next & ED_NEXT_PTR_MASK)
		    == addr_to_phys(batch_ed(batch)));
		batch_ed(prev)->next = batch_ed(batch)->next;
		qpos = "NOT FIRST";
	}
	asm volatile ("": : :"memory");
	usb_log_debug("Batch(%p) removed (%s) from %s, next %x.\n",
	    batch, qpos, instance->name, batch_ed(batch)->next);

	/* Remove from the batch list */
	list_remove(&batch->link);
}
/**
 * @}
 */
