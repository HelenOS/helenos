/*
 * Copyright (c) 2010 Ondrej Palkovsky
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

/** @addtogroup libcabs32le
 * @{
 */
/** @file
 */

#ifndef _LIBC_abs32le_FIBRIL_H_
#define _LIBC_abs32le_FIBRIL_H_

#include <stdint.h>
#include <libarch/fibril_context.h>

#define SP_DELTA  0

#define context_set(ctx, _pc, stack, size, ptls) \
	do { \
		(ctx)->pc = (uintptr_t) (_pc); \
		(ctx)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
		(ctx)->fp = 0; \
		(ctx)->tls = ((uintptr_t) (ptls)) + sizeof(tcb_t); \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	/* On real hardware, this function returns the frame pointer. */
	return ctx->fp;
}

#endif

/** @}
 */
