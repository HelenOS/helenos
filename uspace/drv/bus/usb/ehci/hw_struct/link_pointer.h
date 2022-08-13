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
#ifndef DRV_EHCI_HW_STRUCT_LINK_POINTER_H
#define DRV_EHCI_HW_STRUCT_LINK_POINTER_H

#include <stdint.h>

/** EHCI link pointer, used by many data structures */
typedef volatile uint32_t link_pointer_t;

#define LINK_POINTER_ADDRESS_MASK   0xfffffff0 /* upper 28 bits */

#define LINK_POINTER_TERMINATE_FLAG   (1 << 0)

enum {
	LINK_POINTER_TYPE_iTD  = 0x0 << 1,
	LINK_POINTER_TYPE_QH   = 0x1 << 1,
	LINK_POINTER_TYPE_siTD = 0x2 << 1,
	LINK_POINTER_TYPE_FSTN = 0x3 << 1,
	LINK_POINTER_TYPE_MASK = 0x3 << 1,
};

#define LINK_POINTER_QH(address) \
	((address & LINK_POINTER_ADDRESS_MASK) | LINK_POINTER_TYPE_QH)

#define LINK_POINTER_TD(address) \
	(address & LINK_POINTER_ADDRESS_MASK)

#define LINK_POINTER_TERM \
	((link_pointer_t)LINK_POINTER_TERMINATE_FLAG)

#endif
/**
 * @}
 */
