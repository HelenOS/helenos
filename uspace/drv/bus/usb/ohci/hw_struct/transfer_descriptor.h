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

#ifndef DRV_OHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H
#define DRV_OHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "mem_access.h"
#include "completion_codes.h"

/* OHCI TDs can handle up to 8KB buffers, however, it can use max 2 pages.
 * Using 4KB buffers guarantees the page count condition.
 * (OHCI assumes 4KB pages) */
#define OHCI_TD_MAX_TRANSFER (4 * 1024)

/**
 * Transfer Descriptor representation.
 *
 * See OHCI spec chapter 4.3.1 General Transfer Descriptor on page 19
 * (pdf page 33) for details.
 */
typedef struct td {
	/** Status field. Do not touch on active TDs. */
	volatile uint32_t status;
#define TD_STATUS_ROUND_FLAG (1 << 18)
#define TD_STATUS_DP_MASK (0x3) /* Direction/PID */
#define TD_STATUS_DP_SHIFT (19)
#define TD_STATUS_DP_SETUP (0x0)
#define TD_STATUS_DP_OUT (0x1)
#define TD_STATUS_DP_IN (0x2)
#define TD_STATUS_DI_MASK (0x7) /* Delay interrupt, wait n frames before irq */
#define TD_STATUS_DI_SHIFT (21)
#define TD_STATUS_DI_NO_INTERRUPT (0x7)
#define TD_STATUS_T_FLAG (1 << 24) /* Explicit toggle bit value for this TD */
#define TD_STATUS_T_USE_TD_FLAG (1 << 25) /* 1 = use bit 24 as toggle bit */
#define TD_STATUS_EC_MASK (0x3) /* Error count */
#define TD_STATUS_EC_SHIFT (26)
#define TD_STATUS_CC_MASK (0xf) /* Condition code */
#define TD_STATUS_CC_SHIFT (28)

	/**
	 * Current buffer pointer.
	 * Phys address of the first byte to be transferred. */
	volatile uint32_t cbp;

	/** Pointer to the next TD in chain. 16-byte aligned. */
	volatile uint32_t next;
#define TD_NEXT_PTR_MASK (0xfffffff0)
#define TD_NEXT_PTR_SHIFT (0)

	/**
	 * Buffer end.
	 * Phys address of the last byte of the transfer.
	 * @note this does not have to be on the same page as cbp.
	 */
	volatile uint32_t be;
} __attribute__((packed, aligned(32))) td_t;

void td_init(td_t *, const td_t *, usb_direction_t, const void *, size_t, int);
void td_set_next(td_t *, const td_t *);

/**
 * Check TD for completion.
 * @param instance TD structure.
 * @return true if the TD was accessed and processed by hw, false otherwise.
 */
inline static bool td_is_finished(const td_t *instance)
{
	assert(instance);
	const int cc = (OHCI_MEM32_RD(instance->status) >> TD_STATUS_CC_SHIFT) &
	    TD_STATUS_CC_MASK;
	/* This value is changed on transfer completion,
	 * either to CC_NOERROR or and error code.
	 * See OHCI spec 4.3.1.3.5 p. 23 (pdf 37) */
	if (cc != CC_NOACCESS1 && cc != CC_NOACCESS2) {
		return true;
	}
	return false;
}

/**
 * Get error code that indicates transfer status.
 * @param instance TD structure.
 * @return Error code.
 */
static inline errno_t td_error(const td_t *instance)
{
	assert(instance);
	const int cc = (OHCI_MEM32_RD(instance->status) >>
	    TD_STATUS_CC_SHIFT) & TD_STATUS_CC_MASK;
	return cc_to_rc(cc);
}

/**
 * Get remaining portion of buffer to be read/written
 * @param instance TD structure
 * @return size of remaining buffer.
 */
static inline size_t td_remain_size(td_t *instance)
{
	assert(instance);
	/* Current buffer pointer is cleared on successful transfer */
	if (instance->cbp == 0)
		return 0;
	/* Buffer end points to the last byte of transfer buffer, so add 1 */
	return OHCI_MEM32_RD(instance->be) - OHCI_MEM32_RD(instance->cbp) + 1;
}

#endif

/**
 * @}
 */
