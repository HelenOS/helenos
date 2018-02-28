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
#ifndef DRV_EHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H
#define DRV_EHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H

#include <stdint.h>
#include "link_pointer.h"

/** Isochronous transfer descriptor (HS only) */
typedef struct itd {
	link_pointer_t next;

	volatile uint32_t transaction[8];
#define ITD_TRANSACTION_STATUS_ACTIVE_FLAG  (1 << 31)
#define ITD_TRANSACTION_STATUS_BUFFER_ERROR_FLAG  (1 << 30)
#define ITD_TRANSACTION_STATUS_BABBLE_FLAG   (1 << 29)
#define ITD_TRANSACTION_STATUS_TRANS_ERROR_FLAG  (1 << 28)
#define ITD_TRANSACTION_LENGHT_MASK    0xfff
#define ITD_TRANSACTION_LENGHT_SHIFT   16
#define ITD_TRANSACTION_IOC_FLAG       (1 << 15)
#define ITD_TRANSACTION_PG_MASK        0x3
#define ITD_TRANSACTION_PG_SHIFT       12
#define ITD_TRANSACTION_OFFSET_MASK    0xfff
#define ITD_TRANSACTION_OFFSET_SHIFT   0

	volatile uint32_t buffer_pointer[7];
#define ITD_BUFFER_POINTER_MASK      0xfffff000
/* First buffer pointer */
#define ITD_BUFFER_POINTER_EP_MASK      0xf
#define ITD_BUFFER_POINTER_EP_SHIFT     8
#define ITD_BUFFER_POINTER_ADDR_MASK    0x3f
#define ITD_BUFFER_POINTER_ADDR_SHIFT   0
/* Second buffer pointer */
#define ITD_BUFFER_POINTER_IN_FLAG            (1 << 11)
#define ITD_BUFFER_POINTER_MAX_PACKET_MASK    0x3ff
#define ITD_BUFFER_POINTER_MAX_PACKET_SHIFT   0
/* Third buffer pointer */
#define ITD_BUFFER_POINTER_MULTI_MASK    0x3
#define ITD_BUFFER_POINTER_MULTI_SHIFT   0

	/* 64 bit struct only */
	volatile uint32_t extended_bp[7];
} __attribute__((packed, aligned(32))) itd_t;
#endif
/**
 * @}
 */
