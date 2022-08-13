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
#ifndef DRV_EHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H
#define DRV_EHCI_HW_STRUCT_ISO_TRANSFER_DESCRIPTOR_H

#include <stdint.h>
#include "link_pointer.h"

/** Isochronous transfer descriptor (HS only) */
typedef struct itd {
	link_pointer_t next;

	volatile uint32_t transaction[8];
	volatile uint32_t buffer_pointer[7];

	/* 64 bit struct only */
	volatile uint32_t extended_bp[7];
} __attribute__((packed, aligned(32))) itd_t;

/*
 * itd_t.transaction
 */
#define ITD_TRANSACTION_STATUS_ACTIVE_FLAG  (1 << 31)
#define ITD_TRANSACTION_STATUS_BUFFER_ERROR_FLAG  (1 << 30)
#define ITD_TRANSACTION_STATUS_BABBLE_FLAG   (1 << 29)
#define ITD_TRANSACTION_STATUS_TRANS_ERROR_FLAG  (1 << 28)
#define ITD_TRANSACTION_LENGTH_MASK    0xfff
#define ITD_TRANSACTION_LENGTH_SHIFT   16
#define ITD_TRANSACTION_IOC_FLAG       (1 << 15)
#define ITD_TRANSACTION_PG_MASK        0x3
#define ITD_TRANSACTION_PG_SHIFT       12
#define ITD_TRANSACTION_OFFSET_MASK    0xfff
#define ITD_TRANSACTION_OFFSET_SHIFT   0

/*
 * itd_t.buffer_pointer
 */
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

#endif

/**
 * @}
 */
