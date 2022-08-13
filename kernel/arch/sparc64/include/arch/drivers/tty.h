/*
 * SPDX-FileCopyrightText: 2019 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TTY_H_
#define KERN_sparc64_TTY_H_

#include <genarch/ofw/ofw_tree.h>

extern void tty_init(ofw_tree_node_t *node);

#endif

/** @}
 */
