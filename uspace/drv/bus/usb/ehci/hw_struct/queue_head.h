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
#ifndef DRV_EHCI_HW_STRUCT_QH_H
#define DRV_EHCI_HW_STRUCT_QH_H

#include <assert.h>
#include <stdint.h>
#include <usb/host/endpoint.h>
#include <usb/host/utils/malloc32.h>

#include "link_pointer.h"
#include "transfer_descriptor.h"
#include "mem_access.h"

/** This structure is defined in EHCI design guide p. 46 */
typedef struct queue_head {
	link_pointer_t horizontal;

	volatile uint32_t ep_char;
	volatile uint32_t ep_cap;

	link_pointer_t current;
	/* Transfer overlay starts here */
	link_pointer_t next;
	link_pointer_t alternate;
	volatile uint32_t status;
	volatile uint32_t buffer_pointer[5];

	/* 64 bit struct only */
	volatile uint32_t extended_bp[5];
} __attribute__((packed, aligned(32))) qh_t;

/*
 * qh_t.ep_char
 */
#define QH_EP_CHAR_RL_MASK    0xf
#define QH_EP_CHAR_RL_SHIFT   28
#define QH_EP_CHAR_C_FLAG     (1 << 27)
#define QH_EP_CHAR_MAX_LENGTH_MASK   0x7ff
#define QH_EP_CHAR_MAX_LENGTH_SHIFT  16
#define QH_EP_CHAR_MAX_LENGTH_SET(len) \
    (((len) & QH_EP_CHAR_MAX_LENGTH_MASK) << QH_EP_CHAR_MAX_LENGTH_SHIFT)
#define QH_EP_CHAR_MAX_LENGTH_GET(val) \
    (((val) >> QH_EP_CHAR_MAX_LENGTH_SHIFT) & QH_EP_CHAR_MAX_LENGTH_MASK)
#define QH_EP_CHAR_H_FLAG     (1 << 15)
#define QH_EP_CHAR_DTC_FLAG   (1 << 14)
#define QH_EP_CHAR_EPS_FS     (0x0 << 12)
#define QH_EP_CHAR_EPS_LS     (0x1 << 12)
#define QH_EP_CHAR_EPS_HS     (0x2 << 12)
#define QH_EP_CHAR_EPS_MASK   (0x3 << 12)
#define QH_EP_CHAR_EP_MASK    0xf
#define QH_EP_CHAR_EP_SHIFT   8
#define QH_EP_CHAR_EP_SET(num) \
    (((num) & QH_EP_CHAR_EP_MASK) << QH_EP_CHAR_EP_SHIFT)
#define QH_EP_CHAR_ADDR_GET(val) \
    (((val) >> QH_EP_CHAR_ADDR_SHIFT) & QH_EP_CHAR_ADDR_MASK)
#define QH_EP_CHAR_INACT_FLAG (1 << 7)
#define QH_EP_CHAR_ADDR_MASK  0x3f
#define QH_EP_CHAR_ADDR_SHIFT 0
#define QH_EP_CHAR_ADDR_SET(addr) \
    (((addr) & QH_EP_CHAR_ADDR_MASK) << QH_EP_CHAR_ADDR_SHIFT)
#define QH_EP_CHAR_ADDR_GET(val) \
    (((val) >> QH_EP_CHAR_ADDR_SHIFT) & QH_EP_CHAR_ADDR_MASK)

/*
 * qh_t.ep_cap
 */
#define QH_EP_CAP_MULTI_MASK   0x3
#define QH_EP_CAP_MULTI_SHIFT  30
#define QH_EP_CAP_MULTI_SET(count) \
	(((count) & QH_EP_CAP_MULTI_MASK) << QH_EP_CAP_MULTI_SHIFT)
#define QH_EP_CAP_PORT_MASK    0x7f
#define QH_EP_CAP_PORT_SHIFT   23
#define QH_EP_CAP_TT_PORT_SET(addr) \
	(((addr) & QH_EP_CAP_HUB_MASK) << QH_EP_CAP_HUB_SHIFT)
#define QH_EP_CAP_HUB_MASK     0x7f
#define QH_EP_CAP_HUB_SHIFT    16
#define QH_EP_CAP_TT_ADDR_SET(addr) \
	(((addr) & QH_EP_CAP_HUB_MASK) << QH_EP_CAP_HUB_SHIFT)
#define QH_EP_CAP_C_MASK_MASK  0xff
#define QH_EP_CAP_C_MASK_SHIFT 8
#define QH_EP_CAP_C_MASK_SET(val) \
	(((val) & QH_EP_CAP_C_MASK_MASK) << QH_EP_CAP_C_MASK_SHIFT)
#define QH_EP_CAP_S_MASK_MASK  0xff
#define QH_EP_CAP_S_MASK_SHIFT 0
#define QH_EP_CAP_S_MASK_SET(val) \
	(((val) & QH_EP_CAP_S_MASK_MASK) << QH_EP_CAP_S_MASK_SHIFT)

/*
 * qh_t.alternate
 */
#define QH_ALTERNATE_NACK_CNT_MASK   0x7
#define QH_ALTERNATE_NACK_CNT_SHIFT  1

/*
 * qh_t.status
 */
#define QH_STATUS_TOGGLE_FLAG   (1 << 31)
#define QH_STATUS_TOTAL_MASK    0x7fff
#define QH_STATUS_TOTAL_SHIFT   16
#define QH_STATUS_IOC_FLAG      (1 << 15)
#define QH_STATUS_C_PAGE_MASK   0x7
#define QH_STATUS_C_PAGE_SHIFT  12
#define QH_STATUS_CERR_MASK     0x3
#define QH_STATUS_CERR_SHIFT    10
#define QH_STATUS_PID_MASK      0x3
#define QH_STATUS_PID_SHIFT     8
#define QH_STATUS_ACTIVE_FLAG   (1 << 7)
#define QH_STATUS_HALTED_FLAG   (1 << 6)
#define QH_STATUS_BUFF_ERROR_FLAG  (1 << 5)
#define QH_STATUS_BABBLE_FLAG   (1 << 4)
#define QH_STATUS_TRANS_ERR_FLAG   (1 << 3)
#define QH_STATUS_MISSED_FLAG   (1 << 2)
#define QH_STATUS_SPLIT_FLAG    (1 << 1)
#define QH_STATUS_PING_FLAG     (1 << 0)

/*
 * qh_t.buffer_pointer
 */
#define QH_BUFFER_POINTER_MASK   0xfffff000
/* Only the first buffer pointer */
#define QH_BUFFER_POINTER_OFFSET_MASK   0xfff
#define QH_BUFFER_POINTER_OFFSET_SHIFT  0
/* Only the second buffer pointer */
#define QH_BUFFER_POINTER_C_MASK_MASK   0xff
#define QH_BUFFER_POINTER_C_MASK_SHIFFT 0
/* Only the third buffer pointer */
#define QH_BUFFER_POINTER_S_MASK      0x7f
#define QH_BUFFER_POINTER_S_SHIFT     5
#define QH_BUFFER_POINTER_FTAG_MASK   0x1f
#define QH_BUFFER_POINTER_FTAG_SHIFT  0

static inline void qh_append_qh(qh_t *qh, const qh_t *next)
{
	assert(qh);
	assert(next);
	const uint32_t pa = addr_to_phys(next);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);
	EHCI_MEM32_WR(qh->horizontal, LINK_POINTER_QH(pa));
}

static inline uintptr_t qh_next(const qh_t *qh)
{
	assert(qh);
	return (EHCI_MEM32_RD(qh->horizontal) & LINK_POINTER_ADDRESS_MASK);
}

static inline bool qh_toggle_from_td(const qh_t *qh)
{
	assert(qh);
	return (EHCI_MEM32_RD(qh->ep_cap) & QH_EP_CHAR_DTC_FLAG);
}

static inline void qh_toggle_set(qh_t *qh, int toggle)
{
	assert(qh);
	if (toggle)
		EHCI_MEM32_SET(qh->status, QH_STATUS_TOGGLE_FLAG);
	else
		EHCI_MEM32_CLR(qh->status, QH_STATUS_TOGGLE_FLAG);
}

static inline int qh_toggle_get(const qh_t *qh)
{
	assert(qh);
	return (EHCI_MEM32_RD(qh->status) & QH_STATUS_TOGGLE_FLAG) ? 1 : 0;
}

static inline bool qh_halted(const qh_t *qh)
{
	assert(qh);
	return (EHCI_MEM32_RD(qh->status) & QH_STATUS_HALTED_FLAG);
}

static inline void qh_clear_halt(qh_t *qh)
{
	assert(qh);
	EHCI_MEM32_CLR(qh->status, QH_STATUS_HALTED_FLAG);
}

static inline void qh_set_next_td(qh_t *qh, uintptr_t td)
{
	assert(qh);
	assert(td);
	EHCI_MEM32_WR(qh->next, LINK_POINTER_TD(td));
}

static inline bool qh_transfer_active(const qh_t *qh)
{
	assert(qh);
	return (EHCI_MEM32_RD(qh->status) & QH_STATUS_ACTIVE_FLAG);
}

static inline bool qh_transfer_pending(const qh_t *qh)
{
	assert(qh);
	return !(EHCI_MEM32_RD(qh->next) & LINK_POINTER_TERMINATE_FLAG);
}

extern void qh_init(qh_t *instance, const endpoint_t *ep);

#endif
/**
 * @}
 */
