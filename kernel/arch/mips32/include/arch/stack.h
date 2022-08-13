/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_STACK_H_
#define KERN_mips32_STACK_H_

#include <config.h>

#define MEM_STACK_SIZE	STACK_SIZE

#define STACK_ITEM_SIZE  4
#define STACK_ALIGNMENT  8
#define ABI_STACK_FRAME  32

#endif

/** @}
 */
