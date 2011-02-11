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
	return EOK;
}
/*----------------------------------------------------------------------------*/
void transfer_list_set_next(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	assert(next);
	instance->next = next;
	if (!instance->queue_head)
		return;
	queue_head_add_next(instance->queue_head, next->queue_head_pa);
}
/*----------------------------------------------------------------------------*/
void transfer_list_add_tracker(transfer_list_t *instance, tracker_t *tracker)
{
	assert(instance);
	assert(tracker);

	uint32_t pa = (uintptr_t)addr_to_phys(tracker->td);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);


	if (instance->queue_head->element & LINK_POINTER_TERMINATE_FLAG) {
		usb_log_debug2("Adding td(%X:%X) to queue %s first.\n",
			tracker->td->status, tracker->td->device, instance->name);
		/* there is nothing scheduled */
		instance->last_tracker = tracker;
		instance->queue_head->element = pa;
		usb_log_debug2("Added td(%X:%X) to queue %s first.\n",
			tracker->td->status, tracker->td->device, instance->name);
		return;
	}
	usb_log_debug2("Adding td(%X:%X) to queue %s last.%p\n",
	    tracker->td->status, tracker->td->device, instance->name,
	    instance->last_tracker);
	/* now we can be sure that last_tracker is a valid pointer */
	instance->last_tracker->td->next = pa;
	instance->last_tracker = tracker;

	usb_log_debug2("Added td(%X:%X) to queue %s last.\n",
		tracker->td->status, tracker->td->device, instance->name);

	/* check again, may be use atomic compare and swap */
	if (instance->queue_head->element & LINK_POINTER_TERMINATE_FLAG) {
		instance->queue_head->element = pa;
		usb_log_debug2("Added td(%X:%X) to queue first2 %s.\n",
			tracker->td->status, tracker->td->device, instance->name);
	}
}
/**
 * @}
 */
