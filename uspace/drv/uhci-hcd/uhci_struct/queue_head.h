
/*
 * Copyright (c) 2010 Jan Vesely
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
#ifndef DRV_UHCI_QH_H
#define DRV_UHCI_QH_H

/* libc */
#include <assert.h>

#include "link_pointer.h"
#include "utils/malloc32.h"

typedef struct queue_head {
	link_pointer_t next_queue;
	link_pointer_t element;
} __attribute__((packed)) queue_head_t;

static inline void queue_head_init(queue_head_t *instance, uint32_t next_queue_pa)
{
	assert(instance);
	assert((next_queue_pa & LINK_POINTER_ADDRESS_MASK) == next_queue_pa);

	instance->element = 0 | LINK_POINTER_TERMINATE_FLAG;
	if (next_queue_pa) {
		instance->next_queue = (next_queue_pa & LINK_POINTER_ADDRESS_MASK)
		  | LINK_POINTER_QUEUE_HEAD_FLAG;
	} else {
		instance->next_queue = 0 | LINK_POINTER_TERMINATE_FLAG;
	}
}

static inline queue_head_t * queue_head_get()
	{ return malloc32(sizeof(queue_head_t)); }

static inline void queue_head_dispose(queue_head_t *head)
	{ free32(head); }


#endif
/**
 * @}
 */
