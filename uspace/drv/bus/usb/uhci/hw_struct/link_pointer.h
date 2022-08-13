/*
 * SPDX-FileCopyrightText: 2010 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */

#ifndef DRV_UHCI_HW_STRUCT_LINK_POINTER_H
#define DRV_UHCI_HW_STRUCT_LINK_POINTER_H

#include <stdint.h>

/** UHCI link pointer, used by many data structures */
typedef uint32_t link_pointer_t;

#define LINK_POINTER_TERMINATE_FLAG (1 << 0)
#define LINK_POINTER_QUEUE_HEAD_FLAG (1 << 1)
#define LINK_POINTER_ZERO_BIT_FLAG (1 << 2)
#define LINK_POINTER_VERTICAL_FLAG (1 << 2)
#define LINK_POINTER_RESERVED_FLAG (1 << 3)

#define LINK_POINTER_ADDRESS_MASK 0xfffffff0 /* upper 28 bits */

#define LINK_POINTER_QH(address) \
	((address & LINK_POINTER_ADDRESS_MASK) | LINK_POINTER_QUEUE_HEAD_FLAG)

#define LINK_POINTER_TD(address) \
	(address & LINK_POINTER_ADDRESS_MASK)

#define LINK_POINTER_TERM \
	((link_pointer_t)LINK_POINTER_TERMINATE_FLAG)

#endif

/**
 * @}
 */
