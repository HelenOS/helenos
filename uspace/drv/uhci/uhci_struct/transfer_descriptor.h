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
#ifndef DRV_UHCI_TRANSFER_DESCRIPTOR_H
#define DRV_UHCI_TRANSFER_DESCRIPTOR_H

#include <mem.h>
#include <usb/usb.h>

#include "translating_malloc.h"
#include "callback.h"
#include "link_pointer.h"

/** UHCI Transfer Descriptor */
typedef struct transfer_descriptor {
	link_pointer_t next;

	uint32_t status;

#define TD_STATUS_RESERVED_MASK 0xc000f800
#define TD_STATUS_SPD_FLAG ( 1 << 29 )
#define TD_STATUS_ERROR_COUNT_POS ( 27 )
#define TD_STATUS_ERROR_COUNT_MASK ( 0x11 )
#define TD_STATUS_ERROR_COUNT_DEFAULT 3
#define TD_STATUS_LOW_SPEED_FLAG ( 1 << 26 )
#define TD_STATUS_ISOCHRONOUS_FLAG ( 1 << 25 )
#define TD_STATUS_COMPLETE_INTERRUPT_FLAG ( 1 << 24 )

#define TD_STATUS_ACTIVE ( 1 << 23 )
#define TD_STATUS_ERROR_STALLED ( 1 << 22 )
#define TD_STATUS_ERROR_BUFFER ( 1 << 21 )
#define TD_STATUS_ERROR_BABBLE ( 1 << 20 )
#define TD_STATUS_ERROR_NAK ( 1 << 19 )
#define TD_STATUS_ERROR_CRC ( 1 << 18 )
#define TD_STATUS_ERROR_BIT_STUFF ( 1 << 17 )
#define TD_STATUS_ERROR_RESERVED ( 1 << 16 )
#define TD_STATUS_POS 16
#define TD_STATUS_MASK ( 0xff )

#define TD_STATUS_ACTLEN_POS 0
#define TD_STATUS_ACTLEN_MASK 0x7ff

	uint32_t device;

#define TD_DEVICE_MAXLEN_POS 21
#define TD_DEVICE_MAXLEN_MASK ( 0x7ff )
#define TD_DEVICE_RESERVED_FLAG ( 1 << 20 )
#define TD_DEVICE_DATA_TOGGLE_ONE_FLAG ( 1 << 19 )
#define TD_DEVICE_ENDPOINT_POS 15
#define TD_DEVICE_ENDPOINT_MASK ( 0xf )
#define TD_DEVICE_ADDRESS_POS 8
#define TD_DEVICE_ADDRESS_MASK ( 0x7f )
#define TD_DEVICE_PID_POS 0
#define TD_DEVICE_PID_MASK ( 0xff )

	uint32_t buffer_ptr;

	/* there is 16 byte of data available here
	 * those are used to store callback pointer
	 * and next pointer. Thus there is some free space
	 * on 32bits systems.
	 */
	struct transfer_descriptor *next_va;
	callback_t *callback;
} __attribute__((packed)) transfer_descriptor_t;

static inline int transfer_descriptor_init(transfer_descriptor_t *instance,
  int error_count, size_t size, bool isochronous, usb_target_t target,
	int pid)
{
	assert(instance);

	instance->next =
	  0 | LINK_POINTER_VERTICAL_FLAG | LINK_POINTER_TERMINATE_FLAG;

	assert(size < 1024);
	instance->status = 0
	  | ((error_count & TD_STATUS_ERROR_COUNT_MASK) << TD_STATUS_ERROR_COUNT_POS)
	  | TD_STATUS_ACTIVE;

	instance->device = 0
		| ((size & TD_DEVICE_MAXLEN_MASK) << TD_DEVICE_MAXLEN_POS)
		| ((target.address & TD_DEVICE_ADDRESS_MASK) << TD_DEVICE_ADDRESS_POS)
		| ((target.endpoint & TD_DEVICE_ENDPOINT_MASK) << TD_DEVICE_ENDPOINT_POS)
		| ((pid & TD_DEVICE_PID_MASK) << TD_DEVICE_PID_POS);

	instance->next_va = NULL;
	instance->callback = NULL;

	return EOK;
}

static inline void transfer_descriptor_append(
  transfer_descriptor_t *instance, transfer_descriptor_t *item)
{
	assert(instance);
	instance->next_va = item;
	instance->next = (uintptr_t)addr_to_phys( item ) & LINK_POINTER_ADDRESS_MASK;
}

#endif
/**
 * @}
 */
