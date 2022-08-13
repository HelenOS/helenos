/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_SCR_H_
#define KERN_sparc64_SCR_H_

#include <genarch/ofw/ofw_tree.h>

typedef enum {
	SCR_UNKNOWN,
	SCR_ATYFB,
	SCR_FFB,
	SCR_CGSIX,
	SCR_XVR,
	SCR_QEMU_VGA
} scr_type_t;

extern scr_type_t scr_type;

extern void scr_init(ofw_tree_node_t *node);

#endif

/** @}
 */
