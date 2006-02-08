/*
 * Copyright (C) 2005 Jakub Jermar
 * Copyright (C) 2005 Sergey Bondari
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

#ifndef __FRAME_H__
#define __FRAME_H__

#include <arch/types.h>
#include <typedefs.h>
#include <adt/list.h>
#include <synch/spinlock.h>
#include <mm/buddy.h>
#include <arch/mm/page.h>

#define ONE_FRAME	0

#define ZONES_MAX       16      /**< Maximum number of zones in system */

#define ZONE_JOIN       0x1	/**< If possible, merge with neighberhood zones */

#define FRAME_KA		0x1	/* skip frames conflicting with user address space */
#define FRAME_PANIC		0x2	/* panic on failure */
#define FRAME_ATOMIC 	        0x4	/* do not panic and do not sleep on failure */
#define FRAME_NO_RECLAIM        0x8     /* do not start reclaiming when no free memory */

#define FRAME_OK		0	/* frame_alloc return status */
#define FRAME_NO_MEMORY		1	/* frame_alloc return status */
#define FRAME_ERROR		2	/* frame_alloc return status */

/* Return true if the interlvals overlap */
static inline int overlaps(__address s1,__address sz1, __address s2, __address sz2)
{
	__address e1 = s1+sz1;
	__address e2 = s2+sz2;
	if (s1 >= s2 && s1 < e2)
		return 1;
	if (e1 >= s2 && e1 < e2)
		return 1;
	if ((s1 < s2) && (e1 >= e2))
		return 1;
	return 0;
}

static inline __address PFN2ADDR(pfn_t frame)
{
	return (__address)(frame << FRAME_WIDTH);
}

static inline pfn_t ADDR2PFN(__address addr)
{
	return (pfn_t)(addr >> FRAME_WIDTH);
}

static inline count_t SIZE2FRAMES(size_t size)
{
	if (!size)
		return 0;
	return (count_t)((size-1) >> FRAME_WIDTH)+1;
}

#define IS_BUDDY_ORDER_OK(index, order)		((~(((__native) -1) << (order)) & (index)) == 0)
#define IS_BUDDY_LEFT_BLOCK(zone, frame)	(((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK(zone, frame)	(((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)
#define IS_BUDDY_LEFT_BLOCK_ABS(zone, frame)	(((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK_ABS(zone, frame)	(((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)

#define frame_alloc(order, flags)				frame_alloc_generic(order, flags, NULL, NULL)
#define frame_alloc_rc(order, flags, status)			frame_alloc_generic(order, flags, status, NULL)
#define frame_alloc_rc_zone(order, flags, status, zone)		frame_alloc_generic(order, flags, status, zone)

extern void frame_init(void);
__address frame_alloc_generic(__u8 order, int flags, int * status, int *pzone);
extern void frame_free(__address addr);

extern int zone_create(pfn_t start, count_t count, pfn_t confframe, int flags);
void * frame_get_parent(pfn_t frame, int hint);
void frame_set_parent(pfn_t frame, void *data, int hint);
void frame_mark_unavailable(pfn_t start, count_t count);
__address zone_conf_size(count_t count);
void zone_merge(int z1, int z2);
void zone_merge_all(void);

/*
 * Console functions
 */
extern void zone_print_list(void);
void zone_print_one(int znum);

#endif
