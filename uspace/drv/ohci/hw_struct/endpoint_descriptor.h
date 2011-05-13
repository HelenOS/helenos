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
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_HW_STRUCT_ENDPOINT_DESCRIPTOR_H
#define DRV_OHCI_HW_STRUCT_ENDPOINT_DESCRIPTOR_H

#include <assert.h>
#include <stdint.h>

#include <usb/host/endpoint.h>

#include "../utils/malloc32.h"
#include "transfer_descriptor.h"

#include "completion_codes.h"

typedef struct ed {
	volatile uint32_t status;
#define ED_STATUS_FA_MASK (0x7f)   /* USB device address   */
#define ED_STATUS_FA_SHIFT (0)
#define ED_STATUS_EN_MASK (0xf)    /* USB endpoint address */
#define ED_STATUS_EN_SHIFT (7)
#define ED_STATUS_D_MASK (0x3)     /* direction */
#define ED_STATUS_D_SHIFT (11)
#define ED_STATUS_D_OUT (0x1)
#define ED_STATUS_D_IN (0x2)
#define ED_STATUS_D_TRANSFER (0x3)

#define ED_STATUS_S_FLAG (1 << 13) /* speed flag: 1 = low */
#define ED_STATUS_K_FLAG (1 << 14) /* skip flag (no not execute this ED) */
#define ED_STATUS_F_FLAG (1 << 15) /* format: 1 = isochronous*/
#define ED_STATUS_MPS_MASK (0x3ff) /* max_packet_size*/
#define ED_STATUS_MPS_SHIFT (16)

	volatile uint32_t td_tail;
#define ED_TDTAIL_PTR_MASK (0xfffffff0)
#define ED_TDTAIL_PTR_SHIFT (0)

	volatile uint32_t td_head;
#define ED_TDHEAD_PTR_MASK (0xfffffff0)
#define ED_TDHEAD_PTR_SHIFT (0)
#define ED_TDHEAD_ZERO_MASK (0x3)
#define ED_TDHEAD_ZERO_SHIFT (2)
#define ED_TDHEAD_TOGGLE_CARRY (0x2)
#define ED_TDHEAD_HALTED_FLAG (0x1)

	volatile uint32_t next;
#define ED_NEXT_PTR_MASK (0xfffffff0)
#define ED_NEXT_PTR_SHIFT (0)
} __attribute__((packed)) ed_t;

void ed_init(ed_t *instance, endpoint_t *ep);

static inline void ed_set_td(ed_t *instance, td_t *td)
{
	assert(instance);
	uintptr_t pa = addr_to_phys(td);
	instance->td_head =
	    ((pa & ED_TDHEAD_PTR_MASK)
	    | (instance->td_head & ~ED_TDHEAD_PTR_MASK));
	instance->td_tail = pa & ED_TDTAIL_PTR_MASK;
}

static inline void ed_set_end_td(ed_t *instance, td_t *td)
{
	assert(instance);
	uintptr_t pa = addr_to_phys(td);
	instance->td_tail = pa & ED_TDTAIL_PTR_MASK;
}

static inline void ed_append_ed(ed_t *instance, ed_t *next)
{
	assert(instance);
	assert(next);
	uint32_t pa = addr_to_phys(next);
	assert((pa & ED_NEXT_PTR_MASK) << ED_NEXT_PTR_SHIFT == pa);
	instance->next = pa;
}

static inline int ed_toggle_get(ed_t *instance)
{
	assert(instance);
	return (instance->td_head & ED_TDHEAD_TOGGLE_CARRY) ? 1 : 0;
}

static inline void ed_toggle_set(ed_t *instance, int toggle)
{
	assert(instance);
	assert(toggle == 0 || toggle == 1);
	if (toggle == 1) {
		instance->td_head |= ED_TDHEAD_TOGGLE_CARRY;
	} else {
		/* clear halted flag when reseting toggle */
		instance->td_head &= ~ED_TDHEAD_TOGGLE_CARRY;
		instance->td_head &= ~ED_TDHEAD_HALTED_FLAG;
	}
}
#endif
/**
 * @}
 */
