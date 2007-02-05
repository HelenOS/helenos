/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2005 Sergey Bondari
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_FRAME_H_
#define KERN_FRAME_H_

#include <arch/types.h>
#include <adt/list.h>
#include <synch/spinlock.h>
#include <mm/buddy.h>
#include <arch/mm/page.h>
#include <arch/mm/frame.h>

#define ONE_FRAME	0
#define TWO_FRAMES	1

#ifdef ARCH_STACK_FRAMES
#define STACK_FRAMES ARCH_STACK_FRAMES
#else
#define STACK_FRAMES ONE_FRAME
#endif

/** Maximum number of zones in system. */
#define ZONES_MAX       	16

/** If possible, merge with neighbouring zones. */
#define ZONE_JOIN       	0x1

/** Convert the frame address to kernel va. */
#define FRAME_KA		0x1
/** Do not panic and do not sleep on failure. */
#define FRAME_ATOMIC 	        0x2
/** Do not start reclaiming when no free memory. */
#define FRAME_NO_RECLAIM        0x4

static inline uintptr_t PFN2ADDR(pfn_t frame)
{
	return (uintptr_t) (frame << FRAME_WIDTH);
}

static inline pfn_t ADDR2PFN(uintptr_t addr)
{
	return (pfn_t) (addr >> FRAME_WIDTH);
}

static inline count_t SIZE2FRAMES(size_t size)
{
	if (!size)
		return 0;
	return (count_t) ((size - 1) >> FRAME_WIDTH) + 1;
}

#define IS_BUDDY_ORDER_OK(index, order)		\
	((~(((unative_t) -1) << (order)) & (index)) == 0)
#define IS_BUDDY_LEFT_BLOCK(zone, frame)	\
	(((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK(zone, frame)	\
	(((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)
#define IS_BUDDY_LEFT_BLOCK_ABS(zone, frame)	\
	(((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK_ABS(zone, frame)	\
	(((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)

#define frame_alloc(order, flags)		\
	frame_alloc_generic(order, flags, NULL)

extern void frame_init(void);
extern void *frame_alloc_generic(uint8_t order, int flags, unsigned int *pzone);
extern void frame_free(uintptr_t frame);
extern void frame_reference_add(pfn_t pfn);

extern int zone_create(pfn_t start, count_t count, pfn_t confframe, int flags);
void *frame_get_parent(pfn_t frame, unsigned int hint);
void frame_set_parent(pfn_t frame, void *data, unsigned int hint);
void frame_mark_unavailable(pfn_t start, count_t count);
uintptr_t zone_conf_size(count_t count);
void zone_merge(unsigned int z1, unsigned int z2);
void zone_merge_all(void);

/*
 * Console functions
 */
extern void zone_print_list(void);
void zone_print_one(unsigned int znum);

#endif

/** @}
 */
