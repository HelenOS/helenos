/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
