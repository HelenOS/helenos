/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia32
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia32_FIBRIL_H_
#define _LIBC_ia32_FIBRIL_H_

#include <types/common.h>
#include <libarch/fibril_context.h>

/*
 * According to ABI the stack MUST be aligned on
 * 16-byte boundary. If it is not, the va_arg calling will
 * panic sooner or later
 */
#define SP_DELTA  8

#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = (sysarg_t) (_pc); \
		(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; \
		(c)->tls = (sysarg_t) (ptls); \
		(c)->ebp = 0; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->ebp;
}

#endif

/** @}
 */
