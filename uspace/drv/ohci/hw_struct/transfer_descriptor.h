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

#include <stdint.h>
#include "utils/malloc32.h"

#include "completion_codes.h"

/* OHCI TDs can handle up to 8KB buffers */
#define OHCI_TD_MAX_TRANSFER (8 * 1024)

typedef struct td {
	volatile uint32_t status;
#define TD_STATUS_ROUND_FLAG (1 << 18)
#define TD_STATUS_DP_MASK (0x3) /* direction/PID */
#define TD_STATUS_DP_SHIFT (19)
#define TD_STATUS_DP_SETUP (0x0)
#define TD_STATUS_DP_IN (0x1)
#define TD_STATUS_DP_OUT (0x2)
#define TD_STATUS_DI_MASK (0x7) /* delay interrupt, wait DI frames before int */
#define TD_STATUS_DI_SHIFT (21)
#define TD_STATUS_DI_NO_INTERRUPT (0x7)
#define TD_STATUS_T_MASK (0x3)  /* data toggle 1x = use ED toggle carry */
#define TD_STATUS_T_SHIFT (24)
#define TD_STATUS_T_0 (0x2)
#define TD_STATUS_T_1 (0x3)
#define TD_STATUS_EC_MASK (0x3) /* error count */
#define TD_STATUS_EC_SHIFT (26)
#define TD_STATUS_CC_MASK (0xf) /* condition code */
#define TD_STATUS_CC_SHIFT (28)

	volatile uint32_t cbp; /* current buffer ptr, data to be transfered */
	volatile uint32_t next;
#define TD_NEXT_PTR_MASK (0xfffffff0)
#define TD_NEXT_PTR_SHIFT (0)

	volatile uint32_t be; /* buffer end, address of the last byte */
} __attribute__((packed)) td_t;

void td_init(
    td_t *instance, usb_direction_t dir, void *buffer, size_t size, int toggle);

inline static void td_set_next(td_t *instance, td_t *next)
{
	assert(instance);
	instance->next = addr_to_phys(next) & TD_NEXT_PTR_MASK;
}
#endif
/**
 * @}
 */
