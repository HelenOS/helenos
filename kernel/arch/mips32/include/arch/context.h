/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CONTEXT_H_
#define KERN_mips32_CONTEXT_H_

#include <align.h>
#include <arch/stack.h>
#include <arch/context_struct.h>

/*
 * Put one item onto the stack to support CURRENT and align it up.
 */
#define SP_DELTA  (ABI_STACK_FRAME + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define context_set(ctx, pc, stack, size) \
    context_set_generic(ctx, pc, stack, size)

#endif

/** @}
 */
