/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup libcarm64
 * @{
 */
/** @file
 * @brief Fibrils related declarations.
 */

#ifndef _LIBC_arm64_FIBRIL_H_
#define _LIBC_arm64_FIBRIL_H_

#include <types/common.h>
#include <align.h>
#include <libarch/fibril_context.h>

/** Size of a stack item. */
#define STACK_ITEM_SIZE	 8

/** Required stack alignment. */
#define STACK_ALIGNMENT	 16

#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

/** Sets data to the context.
 *
 * @param c Context (#context_t).
 * @param _pc Program counter.
 * @param stack Stack address.
 * @param size Stack size.
 * @param ptls Pointer to the TCB.
 */
#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = (uint64_t) (_pc); \
		(c)->sp = ((uint64_t) (stack)) + (size) - SP_DELTA; \
		(c)->tls = ((uint64_t) (ptls)) + ARCH_TP_OFFSET; \
		/* Set frame pointer too. */ \
		(c)->x29 = 0; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->x29;
}

#endif

/** @}
 */
