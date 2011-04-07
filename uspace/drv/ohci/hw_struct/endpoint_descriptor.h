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

#include <stdint.h>

#include "completion_codes.h"

typedef struct ed {
	volatile uint32_t status;
#define ED_STATUS_FA_MASK (0x7f)   /* USB device address   */
#define ED_STATUS_FA_SHIFT (0)
#define ED_STATUS_EN_MASK (0xf)    /* USB endpoint address */
#define ED_STATUS_EN_SHIFT (6)
#define ED_STATUS_D_MASK (0x3)     /* direction */
#define ED_STATUS_D_SHIFT (10)
#define ED_STATUS_D_IN (0x1)
#define ED_STATUS_D_OUT (0x2)

#define ED_STATUS_S_FLAG (1 << 13) /* speed flag */
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

	volatile uint32_t next;
#define ED_NEXT_PTR_MASK (0xfffffff0)
#define ED_NEXT_PTR_SHIFT (0)
} __attribute__((packed)) ed_t;
#endif
/**
 * @}
 */
