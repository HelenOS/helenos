/*
 * Copyright (c) 2013 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H
#define DRV_EHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H

#include <stddef.h>
#include <stdint.h>
#include <macros.h>
#include "link_pointer.h"
#include "mem_access.h"

/** Transfer descriptor (non-ISO) */
typedef struct td {
	link_pointer_t next;
	link_pointer_t alternate;

	volatile uint32_t status;
#define TD_STATUS_TOGGLE_FLAG   (1 << 31)
#define TD_STATUS_TOTAL_MASK    0x7fff
#define TD_STATUS_TOTAL_SHIFT   16
#define TD_STATUS_IOC_FLAG      (1 << 15)
#define TD_STATUS_C_PAGE_MASK   0x7
#define TD_STATUS_C_PAGE_SHIFT  12
#define TD_STATUS_CERR_MASK     0x3
#define TD_STATUS_CERR_SHIFT    10
#define TD_STATUS_PID_MASK      0x3
#define TD_STATUS_PID_SHIFT     8
#define TD_STATUS_PID_OUT       0x0
#define TD_STATUS_PID_IN        0x1
#define TD_STATUS_PID_SETUP     0x2
#define TD_STATUS_ACTIVE_FLAG   (1 << 7)
#define TD_STATUS_HALTED_FLAG   (1 << 6)
#define TD_STATUS_BUFF_ERROR_FLAG  (1 << 5)
#define TD_STATUS_BABBLE_FLAG   (1 << 4)
#define TD_STATUS_TRANS_ERR_FLAG   (1 << 3)
#define TD_STATUS_MISSED_FLAG   (1 << 2)
#define TD_STATUS_SPLIT_FLAG    (1 << 1)
#define TD_STATUS_PING_FLAG     (1 << 0)

	volatile uint32_t buffer_pointer[5];
#define TD_BUFFER_POINTER_MASK   0xfffff000
/* Only the first page pointer */
#define TD_BUFFER_POINTER_OFFSET_MASK    0xfff

	/* 64 bit struct only */
	volatile uint32_t extended_bp[5];

} __attribute__((packed,aligned(32))) td_t;

static_assert(sizeof(td_t) % 32 == 0);

static inline bool td_active(const td_t *td)
{
	assert(td);
	return (EHCI_MEM32_RD(td->status) & TD_STATUS_HALTED_FLAG) != 0;
}

static inline size_t td_remain_size(const td_t *td)
{
	assert(td);
	return (EHCI_MEM32_RD(td->status) >> TD_STATUS_TOTAL_SHIFT) &
	    TD_STATUS_TOTAL_MASK;
}

errno_t td_error(const td_t *td);

void td_init(td_t *td, uintptr_t next_phys, uintptr_t buf, usb_direction_t dir,
    size_t buf_size, int toggle, bool ioc);

#endif
/**
 * @}
 */

