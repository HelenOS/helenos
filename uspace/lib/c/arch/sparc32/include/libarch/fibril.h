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

/** @addtogroup libcsparc32
 * @{
 */
/** @file
 */

#ifndef LIBC_sparc32_FIBRIL_H_
#define LIBC_sparc32_FIBRIL_H_

#include <libarch/stack.h>
#include <sys/types.h>
#include <align.h>

#define SP_DELTA  (STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE)

#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = ((uintptr_t) _pc) - 8; \
		(c)->sp = ((uintptr_t) stack) + ALIGN_UP((size), \
		    STACK_ALIGNMENT) - (SP_DELTA); \
		(c)->fp = 0; \
		(c)->tp = (uint32_t) ptls; \
	} while (0)

/*
 * Save only registers that must be preserved across
 * function calls.
 */
typedef struct {
	uintptr_t sp;  /* %o6 */
	uintptr_t pc;  /* %o7 */
	uint32_t i0;
	uint32_t i1;
	uint32_t i2;
	uint32_t i3;
	uint32_t i4;
	uint32_t i5;
	uintptr_t fp;  /* %i6 */
	uintptr_t i7;
	uint32_t l0;
	uint32_t l1;
	uint32_t l2;
	uint32_t l3;
	uint32_t l4;
	uint32_t l5;
	uint32_t l6;
	uint32_t l7;
	uint32_t tp;  /* %g7 */
} context_t;

static inline uintptr_t context_get_fp(context_t *ctx)
{
	return ctx->sp;
}

#endif

/** @}
 */
