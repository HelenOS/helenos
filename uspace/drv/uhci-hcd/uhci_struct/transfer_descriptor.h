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

#include "utils/malloc32.h"
#include "callback.h"
#include "link_pointer.h"

/** UHCI Transfer Descriptor */
typedef struct transfer_descriptor {
	link_pointer_t next;

	volatile uint32_t status;

#define TD_STATUS_RESERVED_MASK 0xc000f800
#define TD_STATUS_SPD_FLAG ( 1 << 29 )
#define TD_STATUS_ERROR_COUNT_POS ( 27 )
#define TD_STATUS_ERROR_COUNT_MASK ( 0x3 )
#define TD_STATUS_ERROR_COUNT_DEFAULT 3
#define TD_STATUS_LOW_SPEED_FLAG ( 1 << 26 )
#define TD_STATUS_ISOCHRONOUS_FLAG ( 1 << 25 )
#define TD_STATUS_COMPLETE_INTERRUPT_FLAG ( 1 << 24 )

#define TD_STATUS_ERROR_ACTIVE ( 1 << 23 )
#define TD_STATUS_ERROR_STALLED ( 1 << 22 )
#define TD_STATUS_ERROR_BUFFER ( 1 << 21 )
#define TD_STATUS_ERROR_BABBLE ( 1 << 20 )
#define TD_STATUS_ERROR_NAK ( 1 << 19 )
#define TD_STATUS_ERROR_CRC ( 1 << 18 )
#define TD_STATUS_ERROR_BIT_STUFF ( 1 << 17 )
#define TD_STATUS_ERROR_RESERVED ( 1 << 16 )
#define TD_STATUS_ERROR_POS 16
#define TD_STATUS_ERROR_MASK ( 0xff )

#define TD_STATUS_ACTLEN_POS 0
#define TD_STATUS_ACTLEN_MASK 0x7ff

	volatile uint32_t device;

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

	volatile uint32_t buffer_ptr;

	/* there is 16 bytes of data available here
	 * those are used to store callback pointer
	 * and next pointer. Thus, there is some free space
	 * on 32bits systems.
	 */
	struct transfer_descriptor *next_va;
	callback_t *callback;
} __attribute__((packed)) transfer_descriptor_t;


void transfer_descriptor_init(transfer_descriptor_t *instance,
  int error_count, size_t size, bool isochronous, usb_target_t target,
	int pid, void *buffer);

static inline transfer_descriptor_t * transfer_descriptor_get(
  int error_count, size_t size, bool isochronous, usb_target_t target,
  int pid, void *buffer)
{
	transfer_descriptor_t * instance =
	  malloc32(sizeof(transfer_descriptor_t));

	if (instance)
		transfer_descriptor_init(
		  instance, error_count, size, isochronous, target, pid, buffer);
	return instance;
}

void transfer_descriptor_fini(transfer_descriptor_t *instance);

static inline void transfer_descriptor_dispose(transfer_descriptor_t *instance)
{
	assert(instance);
	transfer_descriptor_fini(instance);
	free32(instance);
}

int transfer_descriptor_status(transfer_descriptor_t *instance);

static inline void transfer_descriptor_append(
  transfer_descriptor_t *instance, transfer_descriptor_t *item)
{
	assert(instance);
	instance->next_va = item;
	instance->next = (uintptr_t)addr_to_phys(item) & LINK_POINTER_ADDRESS_MASK;
}
#endif
/**
 * @}
 */
