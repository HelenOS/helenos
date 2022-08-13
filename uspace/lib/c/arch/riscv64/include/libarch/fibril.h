/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcriscv64
 * @{
 */
/** @file
 */

#ifndef _LIBC_riscv64_FIBRIL_H_
#define _LIBC_riscv64_FIBRIL_H_

#include <stdint.h>
#include <libarch/fibril_context.h>

#define SP_DELTA  0

#define context_set(ctx, _pc, stack, size, ptls) \
	do { \
		(ctx)->pc = (uintptr_t) (_pc); \
		(ctx)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
		(ctx)->x4 = ((uintptr_t) (ptls)) + sizeof(tcb_t); \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	// FIXME: No frame pointer in the standard ABI.
	return 0;
}

#endif

/** @}
 */
