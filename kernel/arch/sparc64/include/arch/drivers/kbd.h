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

#ifndef KERN_sparc64_KBD_H_
#define KERN_sparc64_KBD_H_

#include <genarch/ofw/ofw_tree.h>

extern void kbd_init(ofw_tree_node_t *node);

#endif

/** @}
 */
