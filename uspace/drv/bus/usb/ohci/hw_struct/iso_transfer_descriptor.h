/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
