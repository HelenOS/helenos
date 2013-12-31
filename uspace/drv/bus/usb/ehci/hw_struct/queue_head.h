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
#include <sys/types.h>

#include "link_pointer.h"

/** This structure is defined in EHCI design guide p. 46 */
typedef struct queue_head {
	link_pointer_t horizontal;

	volatile uint32_t ep_char;
#define QH_EP_CHAR_RL_MASK    0xf
#define QH_EP_CHAR_RL_SHIFT   28
#define QH_EP_CHAR_C_FLAG     (1 << 27)
#define QH_EP_CHAR_LENGTH_MASK   0x7ff
#define QH_EP_CHAR_LENGTH_SHIFT  16
#define QH_EP_CHAR_H_FLAG     (1 << 15)
#define QH_EP_CHAR_DTC_FLAG   (1 << 14)
#define QH_EP_CHAR_EPS_MASK   0x3
#define QH_EP_CHAR_EPS_SHIFT  12
#define QH_EP_CHAR_EPS_FS     0x0
#define QH_EP_CHAR_EPS_LS     0x1
#define QH_EP_CHAR_EPS_HS     0x2
#define QH_EP_CHAR_EP_MASK    0xf
#define QH_EP_CHAR_EP_SHIFT   8
#define QH_EP_CHAR_INACT_FLAG (1 << 7)
#define QH_EP_CHAR_ADDR_MASK  0x3f
#define QH_EP_CHAR_ADDR_SHIFT 0

	volatile uint32_t ep_cap;
#define QH_EP_CAP_MULTI_MASK   0x3
#define QH_EP_CAP_MULTI_SHIFT  30
#define QH_EP_CAP_PORT_MASK    0x7f
#define QH_EP_CAP_PORT_SHIFT   23
#define QH_EP_CAP_HUB_MASK     0x7f
#define QH_EP_CAP_HUB_SHIFT    16
#define QH_EP_CAP_C_MASK_MASK  0xff
#define QH_EP_CAP_C_MASK_SHIFT 8
#define QH_EP_CAP_S_MASK_MASK  0xff
#define QH_EP_CAP_S_MASL_SHIFT 0

	link_pointer_t current;
/* Transfer overlay starts here */
	link_pointer_t next;
	link_pointer_t alternate;
#define QH_ALTERNATE_NACK_CNT_MASK   0x7
#define QH_ALTERNATE_NACK_CNT_SHIFT  1

	volatile uint32_t status;
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

	volatile uint32_t buffer_pointer[5];
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

} qh_t;

#endif
/**
 * @}
 */
