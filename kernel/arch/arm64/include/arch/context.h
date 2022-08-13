/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Thread context.
 */

#ifndef KERN_arm64_CONTEXT_H_
#define KERN_arm64_CONTEXT_H_

#include <align.h>
#include <arch/context_struct.h>
#include <arch/stack.h>

/* Put one item onto the stack to support CURRENT and align it up. */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = (uint64_t) (_pc); \
		(c)->sp = ((uint64_t) (stack)) + (size) - SP_DELTA; \
		/* Set frame pointer too. */ \
		(c)->x29 = 0; \
	} while (0)

#endif

/** @}
 */
