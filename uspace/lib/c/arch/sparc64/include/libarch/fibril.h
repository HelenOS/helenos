/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef _LIBC_sparc64_FIBRIL_H_
#define _LIBC_sparc64_FIBRIL_H_

#include <libarch/stack.h>
#include <libarch/fibril_context.h>
#include <stdint.h>
#include <align.h>

#define SP_DELTA  (STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE)

#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = ((uintptr_t) _pc) - 8; \
		(c)->sp = ((uintptr_t) stack) + ALIGN_UP((size), \
		    STACK_ALIGNMENT) - (STACK_BIAS + SP_DELTA); \
		(c)->fp = -STACK_BIAS; \
		(c)->tp = (uint64_t) ptls; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->sp + STACK_BIAS;
}

#endif

/** @}
 */
