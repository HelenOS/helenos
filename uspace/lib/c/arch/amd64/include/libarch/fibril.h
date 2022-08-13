/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcamd64
 * @{
 */
/** @file
 */

#ifndef _LIBC_amd64_FIBRIL_H_
#define _LIBC_amd64_FIBRIL_H_

#include <libarch/fibril_context.h>

/*
 * According to ABI the stack MUST be aligned on
 * 16-byte boundary. If it is not, the va_arg calling will
 * panic sooner or later
 */
#define SP_DELTA  16

#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = (sysarg_t) (_pc); \
		(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; \
		(c)->tls = (sysarg_t) (ptls); \
		(c)->rbp = 0; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->rbp;
}

#endif

/** @}
 */
