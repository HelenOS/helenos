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

/** @addtogroup libcia64
 * @{
 */
/** @file
 */

#ifndef LIBC_ia64_FIBRIL_H_
#define LIBC_ia64_FIBRIL_H_

#include <stdint.h>
#include <align.h>
#include <libarch/stack.h>
#include <types/common.h>
#include <libarch/fibril_context.h>

/*
 * __setjmp() and __longjmp() are both leaf procedures.
 * No need to allocate scratch area.
 */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define PFM_MASK  (~0x3fffffffff)

/* Stack is divided into two equal parts (for memory stack and register stack). */
#define FIBRIL_INITIAL_STACK_DIVISION  2

#define context_set(c, _pc, stack, size, tls) \
	do { \
		(c)->pc = (uint64_t) _pc; \
		(c)->bsp = ((uint64_t) stack) + \
		    size / FIBRIL_INITIAL_STACK_DIVISION; \
		(c)->ar_pfs &= PFM_MASK; \
		(c)->sp = ((uint64_t) stack) + \
		    ALIGN_UP((size / FIBRIL_INITIAL_STACK_DIVISION), STACK_ALIGNMENT) - \
		    SP_DELTA; \
		(c)->tp = (uint64_t) tls; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return 0;	/* FIXME */
}

#endif

/** @}
 */
