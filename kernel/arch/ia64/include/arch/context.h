/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
