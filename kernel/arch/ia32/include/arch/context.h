/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CONTEXT_H_
#define KERN_ia32_CONTEXT_H_

#include <typedefs.h>
#include <arch/context_struct.h>

#define STACK_ITEM_SIZE  4

/*
 * Both context_save() and context_restore() eat two doublewords from the stack.
 * First for pop of the saved register, second during ret instruction.
 *
 * One item is put onto stack to support CURRENT.
 */
#define SP_DELTA  (8 + STACK_ITEM_SIZE)

#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = (uintptr_t) (_pc); \
		(c)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
		(c)->ebp = 0; \
	} while (0)

#endif

/** @}
 */
