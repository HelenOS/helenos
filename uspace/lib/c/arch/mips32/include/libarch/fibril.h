/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcmips32
 * @{
 */
/** @file
 * @ingroup libcmips32
 */

#ifndef _LIBC_mips32_FIBRIL_H_
#define _LIBC_mips32_FIBRIL_H_

#include <stdint.h>
#include <libarch/fibril_context.h>
#include <libarch/stack.h>
#include <align.h>

#define SP_DELTA  (ABI_STACK_FRAME + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

/*
 * We define our own context_set, because we need to set
 * the TLS pointer to the tcb + 0x7000
 *
 * See tls_set in thread.h
 */
#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = (sysarg_t) (_pc); \
		(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; \
		(c)->tls = ((sysarg_t)(ptls)) + 0x7000 + sizeof(tcb_t); \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->sp;
}

#endif

/** @}
 */
