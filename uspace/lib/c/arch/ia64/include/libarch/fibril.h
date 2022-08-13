/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia64
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia64_FIBRIL_H_
#define _LIBC_ia64_FIBRIL_H_

#include <stdint.h>
#include <align.h>
#include <libarch/stack.h>
#include <types/common.h>
#include <libarch/fibril_context.h>

/*
 * __context_save() and __context_restore() are both leaf procedures.
 * No need to allocate scratch area.
 */
#define SP_DELTA  (0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))

#define PFM_MASK  (~0x3fffffffff)

/* Stack is divided into two equal parts (for memory stack and register stack). */
#define FIBRIL_INITIAL_STACK_DIVISION  2

#define context_set(c, _pc, stack, size, tls) \
	do { \
		(c)->pc = (uint64_t) _pc; \
		(c)->bsp = ((uint64_t) stack) + \
		    size / FIBRIL_INITIAL_STACK_DIVISION; \
		(c)->ar_pfs &= PFM_MASK; \
		(c)->sp = ((uint64_t) stack) + \
		    ALIGN_UP((size / FIBRIL_INITIAL_STACK_DIVISION), STACK_ALIGNMENT) - \
		    SP_DELTA; \
		(c)->tp = (uint64_t) tls; \
	} while (0)

static inline uintptr_t _context_get_fp(context_t *ctx)
{
	return 0;	/* FIXME */
}

#endif

/** @}
 */
