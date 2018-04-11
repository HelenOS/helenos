/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
#include <stdbool.h>
#include <stdint.h>

#include <usb/host/endpoint.h>
#include <usb/host/utils/malloc32.h>

#include "transfer_descriptor.h"

#include "completion_codes.h"
#include "mem_access.h"

/**
 * OHCI Endpoint Descriptor representation.
 *
 * See OHCI spec. Chapter 4.2, page 16 (pdf page 30) for details */
typedef struct ed {
	/**
	 * Status field.
	 *
	 * See table 4-1, p. 17 OHCI spec (pdf page 31).
	 */
	volatile uint32_t status;
#define ED_STATUS_FA_MASK (0x7f)   /* USB device address   */
#define ED_STATUS_FA_SHIFT (0)
#define ED_STATUS_EN_MASK (0xf)    /* USB endpoint address */
#define ED_STATUS_EN_SHIFT (7)
#define ED_STATUS_D_MASK (0x3)     /* Direction */
#define ED_STATUS_D_SHIFT (11)
#define ED_STATUS_D_OUT (0x1)
#define ED_STATUS_D_IN (0x2)
#define ED_STATUS_D_TD (0x3) /* Direction is specified by TD */

#define ED_STATUS_S_FLAG (1 << 13) /* Speed flag: 1 = low */
#define ED_STATUS_K_FLAG (1 << 14) /* Skip flag (no not execute this ED) */
#define ED_STATUS_F_FLAG (1 << 15) /* Format: 1 = isochronous */
#define ED_STATUS_MPS_MASK (0x3ff) /* Maximum packet size */
#define ED_STATUS_MPS_SHIFT (16)

	/**
	 * Pointer to the last TD.
	 *
	 * OHCI hw never changes this field and uses it only for a reference.
	 */
	volatile uint32_t td_tail;
#define ED_TDTAIL_PTR_MASK (0xfffffff0)
#define ED_TDTAIL_PTR_SHIFT (0)

	/**
	 * Pointer to the first TD.
	 *
	 * Driver should not change this field if the ED is active.
	 * This field is updated by OHCI hw and points to the next TD
	 * to be executed.
	 */
	volatile uint32_t td_head;
#define ED_TDHEAD_PTR_MASK (0xfffffff0)
#define ED_TDHEAD_PTR_SHIFT (0)
#define ED_TDHEAD_ZERO_MASK (0x3)
#define ED_TDHEAD_ZERO_SHIFT (2)
#define ED_TDHEAD_TOGGLE_CARRY (0x2)
#define ED_TDHEAD_HALTED_FLAG (0x1)

	/**
	 * Pointer to the next ED.
	 *
	 * Driver should not change this field on active EDs.
	 */
	volatile uint32_t next;
#define ED_NEXT_PTR_MASK (0xfffffff0)
#define ED_NEXT_PTR_SHIFT (0)
} __attribute__((packed, aligned(32))) ed_t;

void ed_init(ed_t *instance, const endpoint_t *ep, const td_t *td);

/**
 * Check for SKIP or HALTED flag being set.
 * @param instance ED
 * @return true if either SKIP or HALTED flag is set, false otherwise.
 */
static inline bool ed_inactive(const ed_t *instance)
{
	assert(instance);
	return (OHCI_MEM32_RD(instance->td_head) & ED_TDHEAD_HALTED_FLAG) ||
	    (OHCI_MEM32_RD(instance->status) & ED_STATUS_K_FLAG);
}

static inline void ed_clear_halt(ed_t *instance)
{
	assert(instance);
	OHCI_MEM32_CLR(instance->td_head, ED_TDHEAD_HALTED_FLAG);
}

/**
 * Check whether this ED contains TD to be executed.
 * @param instance ED
 * @return true if there are pending TDs, false otherwise.
 */
static inline bool ed_transfer_pending(const ed_t *instance)
{
	assert(instance);
	return (OHCI_MEM32_RD(instance->td_head) & ED_TDHEAD_PTR_MASK) !=
	    (OHCI_MEM32_RD(instance->td_tail) & ED_TDTAIL_PTR_MASK);
}

/**
 * Set the last element of TD list
 * @param instance ED
 * @param instance TD to set as the last item.
 */
static inline void ed_set_tail_td(ed_t *instance, const td_t *td)
{
	assert(instance);
	const uintptr_t pa = addr_to_phys(td);
	OHCI_MEM32_WR(instance->td_tail, pa & ED_TDTAIL_PTR_MASK);
}

static inline uint32_t ed_tail_td(const ed_t *instance)
{
	assert(instance);
	return OHCI_MEM32_RD(instance->td_tail) & ED_TDTAIL_PTR_MASK;
}

static inline uint32_t ed_head_td(const ed_t *instance)
{
	assert(instance);
	return OHCI_MEM32_RD(instance->td_head) & ED_TDHEAD_PTR_MASK;
}

/**
 * Set the HeadP of ED. Do not call unless the ED is Halted.
 * @param instance ED
 */
static inline void ed_set_head_td(ed_t *instance, const td_t *td)
{
	assert(instance);
	const uintptr_t pa = addr_to_phys(td);
	OHCI_MEM32_WR(instance->td_head, pa & ED_TDHEAD_PTR_MASK);
}

/**
 * Set next ED in ED chain.
 * @param instance ED to modify
 * @param next ED to append
 */
static inline void ed_append_ed(ed_t *instance, const ed_t *next)
{
	assert(instance);
	assert(next);
	const uint32_t pa = addr_to_phys(next);
	assert((pa & ED_NEXT_PTR_MASK) << ED_NEXT_PTR_SHIFT == pa);
	OHCI_MEM32_WR(instance->next, pa);
}

static inline uint32_t ed_next(const ed_t *instance)
{
	assert(instance);
	return OHCI_MEM32_RD(instance->next) & ED_NEXT_PTR_MASK;
}

/**
 * Get toggle bit value stored in this ED
 * @param instance ED
 * @return Toggle bit value
 */
static inline int ed_toggle_get(const ed_t *instance)
{
	assert(instance);
	return !!(OHCI_MEM32_RD(instance->td_head) & ED_TDHEAD_TOGGLE_CARRY);
}

/**
 * Set toggle bit value stored in this ED
 * @param instance ED
 * @param toggle Toggle bit value
 */
static inline void ed_toggle_set(ed_t *instance, bool toggle)
{
	assert(instance);
	if (toggle) {
		OHCI_MEM32_SET(instance->td_head, ED_TDHEAD_TOGGLE_CARRY);
	} else {
		/* Clear halted flag when reseting toggle TODO: Why? */
		OHCI_MEM32_CLR(instance->td_head, ED_TDHEAD_TOGGLE_CARRY);
		OHCI_MEM32_CLR(instance->td_head, ED_TDHEAD_HALTED_FLAG);
	}
}
#endif
/**
 * @}
 */
