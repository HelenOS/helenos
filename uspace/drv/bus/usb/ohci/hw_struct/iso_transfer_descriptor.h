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

#ifndef DRV_OHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H
#define DRV_OHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H

#include <stdint.h>

#include "completion_codes.h"

typedef struct itd {
	volatile uint32_t status;
#define ITD_STATUS_SF_MASK (0xffff) /* starting frame */
#define ITD_STATUS_SF_SHIFT (0)
#define ITD_STATUS_DI_MASK (0x7) /* delay int, wait DI frames before int */
#define ITD_STATUS_DI_SHIFT (21)
#define ITD_STATUS_DI_NO_INTERRUPT (0x7)
#define ITD_STATUS_FC_MASK (0x7) /* frame count */
#define ITD_STATUS_FC_SHIFT (24)
#define ITD_STATUS_CC_MASK (0xf) /* condition code */
#define ITD_STATUS_CC_SHIFT (28)

	volatile uint32_t page;   /* page number of the first byte in buffer */
#define ITD_PAGE_BP0_MASK (0xfffff000)
#define ITD_PAGE_BP0_SHIFT (0)

	volatile uint32_t next;
#define ITD_NEXT_PTR_MASK (0xfffffff0)
#define ITD_NEXT_PTR_SHIFT (0)

	volatile uint32_t be; /* buffer end, address of the last byte */

	volatile uint16_t offset[8];
#define ITD_OFFSET_SIZE_MASK (0x3ff)
#define ITD_OFFSET_SIZE_SHIFT (0)
#define ITD_OFFSET_CC_MASK (0xf)
#define ITD_OFFSET_CC_SHIFT (12)

} __attribute__((packed, aligned(32))) itd_t;

#endif

/**
 * @}
 */
