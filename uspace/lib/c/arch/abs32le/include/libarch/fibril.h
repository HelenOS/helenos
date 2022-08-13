/*
 * SPDX-FileCopyrightText: 2010 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
