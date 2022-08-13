/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_HW_STRUCT_SPLIT_ISO_TRANSFER_DESCRIPTOR_H
#define DRV_EHCI_HW_STRUCT_SPLIT_ISO_TRANSFER_DESCRIPTOR_H

#include <stdint.h>
#include "link_pointer.h"

/** Isochronous transfer descriptor (split only) */
typedef struct sitd {
	link_pointer_t next;

	volatile uint32_t ep;
	volatile uint32_t uframe;
	volatile uint32_t status;
	volatile uint32_t buffer_pointer[2];
	link_pointer_t back;

	/* 64 bit struct only */
	volatile uint32_t extended_bp[2];
} __attribute__((packed, aligned(32))) sitd_t;

/*
 * sitd_t.ep
 */
#define SITD_EP_IN_FLAG         (1 << 31)
#define SITD_EP_PORT_MASK       0x3f
#define SITD_EP_PORT_SHIFT      24
#define SITD_EP_HUB_ADDR_MASK   0x3f
#define SITD_EP_HUB_ADDR_SHIFT  16
#define SITD_EP_EP_MASK         0xf
#define SITD_EP_EP_SHIFT        8
#define SITD_EP_ADDR_MASK       0x3f
#define SITD_EP_ADDR_SHIFT      0

/*
 * sitd_t.uframe
 */
#define SITD_uFRAME_CMASK_MASK    0xff
#define SITD_uFRAME_CMASK_SHIFT   8
#define SITD_uFRAME_SMASK_MASK    0xff
#define SITD_uFRAME_SMASK_SHIFT   0

/*
 * sitd_t.status
 */
#define SITD_STATUS_IOC_FLAG            (1 << 31)
#define SITD_STATUS_PAGE_FLAG           (1 << 30)
#define SITD_STATUS_TOTAL_MASK          0x3ff
#define SITD_STATUS_TOTAL_SHIFT         16
#define SITD_STATUS_uFRAME_CMASK_MASK   0xff
#define SITD_STATUS_uFRAME_CMAKS_SHIFT  8
#define SITD_STATUS_ACTIVE_FLAG         (1 << 7)
#define SITD_STATUS_ERR_FLAG            (1 << 6)
#define SITD_STATUS_DATA_ERROR_FLAG     (1 << 5)
#define SITD_STATUS_BABBLE_FLAG         (1 << 4)
#define SITD_STATUS_TRANS_ERROR_FLAG    (1 << 3)
#define SITD_STATUS_MISSED_uFRAME_FLAG  (1 << 2)
#define SITD_STATUS_SPLIT_COMPLETE_FLAG (1 << 1)

/*
 * sitd_t.buffer_pointer
 */
#define SITD_BUFFER_POINTER_MASK   0xfffff000
/* Only the first page pointer */
#define SITD_BUFFER_POINTER_CURRENT_MASK    0xfff
#define SITD_BUFFER_POINTER_CURRENT_SHIFT   0
/* Only the second page pointer */
#define SITD_BUFFER_POINTER_TP_MASK       0x3
#define SITD_BUFFER_POINTER_TP_SHIFT      3
#define SITD_BUFFER_POINTER_COUNT_MASK    0x7
#define SITD_BUFFER_POINTER_COUNT_SHIFT   0

#endif

/**
 * @}
 */
