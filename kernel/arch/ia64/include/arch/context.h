/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_CONTEXT_H_
#define KERN_ia64_CONTEXT_H_

#include <typedefs.h>
#include <arch/register.h>
#include <align.h>
#include <arch/stack.h>
#include <arch/context_struct.h>

/*
 * context_save_arch() and context_restore_arch() are both leaf procedures.
 * No need to allocate scratch area.
 *
 * One item is put onto the stack to support CURRENT.
 */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

extern void *__gp;
/* RSE stack starts at the bottom of memory stack, hence the division by 2. */
#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = (uintptr_t) _pc; \
		(c)->bsp = ((uintptr_t) stack) + ALIGN_UP((size / 2), REGISTER_STACK_ALIGNMENT); \
		(c)->ar_pfs &= PFM_MASK; \
		(c)->ar_fpsr = FPSR_TRAPS_ALL | FPSR_SF1_CTRL; \
		(c)->sp = ((uintptr_t) stack) + ALIGN_UP((size / 2), STACK_ALIGNMENT) - SP_DELTA; \
		(c)->r1 = (uintptr_t) &__gp; \
	} while (0)

#endif

/** @}
 */
