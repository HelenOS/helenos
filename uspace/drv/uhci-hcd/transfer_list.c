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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>

#include <usb/debug.h>

#include "transfer_list.h"

int transfer_list_init(transfer_list_t *instance, const char *name)
{
	assert(instance);
	instance->next = NULL;
	instance->name = name;
	instance->queue_head = queue_head_get();
	if (!instance->queue_head) {
		usb_log_error("Failed to allocate queue head.\n");
		return ENOMEM;
	}
	instance->queue_head_pa = (uintptr_t)addr_to_phys(instance->queue_head);

	queue_head_init(instance->queue_head);
	list_initialize(&instance->batch_list);
	fibril_mutex_initialize(&instance->guard);
	return EOK;
}
/*----------------------------------------------------------------------------*/
void transfer_list_set_next(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	assert(next);
	if (!instance->queue_head)
		return;
	queue_head_append_qh(instance->queue_head, next->queue_head_pa);
	instance->queue_head->element = instance->queue_head->next_queue;
}
/*----------------------------------------------------------------------------*/
void transfer_list_add_batch(transfer_list_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);

	uint32_t pa = (uintptr_t)addr_to_phys(batch->qh);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);
	pa |= LINK_POINTER_QUEUE_HEAD_FLAG;

	batch->qh->next_queue = instance->queue_head->next_queue;

	fibril_mutex_lock(&instance->guard);

	if (instance->queue_head->element == instance->queue_head->next_queue) {
		/* there is nothing scheduled */
		list_append(&batch->link, &instance->batch_list);
		instance->queue_head->element = pa;
		usb_log_debug2("Added batch(%p) to queue %s first.\n",
			batch, instance->name);
		fibril_mutex_unlock(&instance->guard);
		return;
	}
	/* now we can be sure that there is someting scheduled */
	assert(!list_empty(&instance->batch_list));
	batch_t *first = list_get_instance(
	          instance->batch_list.next, batch_t, link);
	batch_t *last = list_get_instance(
	    instance->batch_list.prev, batch_t, link);
	queue_head_append_qh(last->qh, pa);
	list_append(&batch->link, &instance->batch_list);
	usb_log_debug2("Added batch(%p) to queue %s last, first is %p.\n",
		batch, instance->name, first );
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
static void transfer_list_remove_batch(
    transfer_list_t *instance, batch_t *batch)
{
	assert(instance);
	assert(batch);
	assert(instance->queue_head);
	assert(batch->qh);

	/* I'm the first one here */
	if (batch->link.prev == &instance->batch_list) {
		usb_log_debug("Removing tracer %p was first, next element %x.\n",
			batch, batch->qh->next_queue);
		instance->queue_head->element = batch->qh->next_queue;
	} else {
		usb_log_debug("Removing tracer %p was NOT first, next element %x.\n",
			batch, batch->qh->next_queue);
		batch_t *prev = list_get_instance(batch->link.prev, batch_t, link);
		prev->qh->next_queue = batch->qh->next_queue;
	}
	list_remove(&batch->link);
}
/*----------------------------------------------------------------------------*/
void transfer_list_check(transfer_list_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	link_t *current = instance->batch_list.next;
	while (current != &instance->batch_list) {
		link_t *next = current->next;
		batch_t *batch = list_get_instance(current, batch_t, link);

		if (batch_is_complete(batch)) {
			transfer_list_remove_batch(instance, batch);
			batch->next_step(batch);
		}
		current = next;
	}
	fibril_mutex_unlock(&instance->guard);
}
/**
 * @}
 */
