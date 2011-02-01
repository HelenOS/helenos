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

int transfer_list_init(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	instance->first = NULL;
	instance->last = NULL;
	instance->queue_head = malloc32(sizeof(queue_head_t));
	if (!instance->queue_head) {
		usb_log_error("Failed to allocate queue head.\n");
		return ENOMEM;
	}
	instance->queue_head_pa = (uintptr_t)addr_to_phys(instance->queue_head);

	uint32_t next_pa = next ? next->queue_head_pa : 0;
	queue_head_init(instance->queue_head, next_pa);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int transfer_list_append(
  transfer_list_t *instance, transfer_descriptor_t *transfer)
{
	assert(instance);
	assert(transfer);

	uint32_t pa = (uintptr_t)addr_to_phys(transfer);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);

	/* empty list */
	if (instance->first == NULL) {
		assert(instance->last == NULL);
		instance->first = instance->last = transfer;
	} else {
		assert(instance->last);
		instance->last->next_va = transfer;

		assert(instance->last->next & LINK_POINTER_TERMINATE_FLAG);
		instance->last->next = (pa & LINK_POINTER_ADDRESS_MASK);
		instance->last = transfer;
	}

	assert(instance->queue_head);
	if (instance->queue_head->element & LINK_POINTER_TERMINATE_FLAG) {
		instance->queue_head->element = (pa & LINK_POINTER_ADDRESS_MASK);
	}
	usb_log_debug("Successfully added transfer to the hc queue %p.\n",
	  instance);
	return EOK;
}
/**
 * @}
 */
