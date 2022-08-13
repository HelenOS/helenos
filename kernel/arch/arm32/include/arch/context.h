/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Thread context.
 */

#ifndef KERN_arm32_CONTEXT_H_
#define KERN_arm32_CONTEXT_H_

#include <align.h>
#include <arch/stack.h>
#include <arch/context_struct.h>
#include <arch/regutils.h>

/* Put one item onto the stack to support CURRENT and align it up. */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = (uintptr_t) (_pc); \
		(c)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
		(c)->fp = 0; \
		(c)->cpu_mode = SUPERVISOR_MODE; \
	} while (0)

#ifndef __ASSEMBLER__

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
