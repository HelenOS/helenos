/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Stack constants.
 */

#ifndef KERN_arm64_STACK_H_
#define KERN_arm64_STACK_H_

#include <config.h>

#define MEM_STACK_SIZE  STACK_SIZE

/** Size of a stack item. */
#define STACK_ITEM_SIZE	 8

/** Required stack alignment. */
#define STACK_ALIGNMENT	 16

#endif

/** @}
 */
