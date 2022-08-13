/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm32
 * @{
 */
/** @file
 *  @brief Fibrils related declarations.
 */

#ifndef _LIBC_arm32_FIBRIL_H_
#define _LIBC_arm32_FIBRIL_H_

#include <types/common.h>
#include <align.h>
#include <libarch/fibril_context.h>

/** Size of a stack item */
#define STACK_ITEM_SIZE  4

/** Stack alignment - see <a href="http://www.arm.com/support/faqdev/14269.html">ABI</a> for details */
#define STACK_ALIGNMENT  8

#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

/** Sets data to the context.
 *
 *  @param c     Context (#context_t).
 *  @param _pc   Program counter.
 *  @param stack Stack address.
 *  @param size  Stack size.
 *  @param ptls  Pointer to the TCB.
 */
#define context_set(c, _pc, stack, size, ptls) \
	do { \
		(c)->pc = (sysarg_t) (_pc); \
		(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; \
		(c)->tls = ((sysarg_t)(ptls)) + ARCH_TP_OFFSET; \
		(c)->fp = 0; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return ctx->fp;
}

#endif

/** @}
 */
