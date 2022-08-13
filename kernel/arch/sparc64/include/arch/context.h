/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CONTEXT_H_
#define KERN_sparc64_CONTEXT_H_

#include <arch/stack.h>
#include <arch/context_struct.h>
#include <typedefs.h>
#include <align.h>

#define SP_DELTA  (STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE)

#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = ((uintptr_t) _pc) - 8; \
		(c)->sp = ((uintptr_t) stack) + ALIGN_UP((size), \
		    STACK_ALIGNMENT) - (STACK_BIAS + SP_DELTA); \
		(c)->fp = -STACK_BIAS; \
	} while (0)

#endif

/** @}
 */
