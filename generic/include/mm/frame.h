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
#include <mm/slab.h>

#define ONE_FRAME	0

#define FRAME_KA		0x1	/* skip frames conflicting with user address space */
#define FRAME_PANIC		0x2	/* panic on failure */
#define FRAME_ATOMIC 	        0x4	/* do not panic and do not sleep on failure */
#define FRAME_NO_RECLAIM        0x8     /* Do not start reclaiming when no free memory */

#define FRAME_OK		0	/* frame_alloc return status */
#define FRAME_NO_MEMORY		1	/* frame_alloc return status */
#define FRAME_ERROR		2	/* frame_alloc return status */

#define FRAME2ADDR(zone, frame)			((zone)->base + (((frame) - (zone)->frames) << FRAME_WIDTH))
#define ADDR2FRAME(zone, addr)			(&((zone)->frames[(((addr) - (zone)->base) >> FRAME_WIDTH)]))
#define FRAME_INDEX(zone, frame)		((index_t)((frame) - (zone)->frames))
#define FRAME_INDEX_ABS(zone, frame)		(((index_t)((frame) - (zone)->frames)) + (zone)->base_index)
#define FRAME_INDEX_VALID(zone, index)		(((index) >= 0) && ((index) < ((zone)->free_count + (zone)->busy_count)))
#define IS_BUDDY_ORDER_OK(index, order)		((~(((__native) -1) << (order)) & (index)) == 0)
#define IS_BUDDY_LEFT_BLOCK(zone, frame)	(((FRAME_INDEX((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK(zone, frame)	(((FRAME_INDEX((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)
#define IS_BUDDY_LEFT_BLOCK_ABS(zone, frame)	(((FRAME_INDEX_ABS((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 0)
#define IS_BUDDY_RIGHT_BLOCK_ABS(zone, frame)	(((FRAME_INDEX_ABS((zone), (frame)) >> (frame)->buddy_order) & 0x1) == 1)

#define ZONE_BLACKLIST_SIZE	8

#define frame_alloc(order, flags)				frame_alloc_generic(order, flags, NULL, NULL)
#define frame_alloc_rc(order, flags, status)			frame_alloc_generic(order, flags, status, NULL)
#define frame_alloc_rc_zone(order, flags, status, zone)		frame_alloc_generic(order, flags, status, zone)

struct zone {
	link_t link;		/**< link to previous and next zone */

	SPINLOCK_DECLARE(lock);	/**< this lock protects everything below */
	__address base;		/**< physical address of the first frame in the frames array */
	index_t base_index;	/**< frame index offset of the zone base */
	frame_t *frames;	/**< array of frame_t structures in this zone */
	count_t free_count;	/**< number of free frame_t structures */
	count_t busy_count;	/**< number of busy frame_t structures */
	
	buddy_system_t * buddy_system; /**< buddy system for the zone */
	int flags;
};

struct frame {
	count_t refcount;	/**< tracking of shared frames  */
	__u8 buddy_order;	/**< buddy system block order */
	link_t buddy_link;	/**< link to the next free block inside one order */
	void *parent;           /**< If allocated by slab, this points there */
};

struct region {
	__address base;
	size_t size;
};

extern region_t zone_blacklist[];
extern count_t zone_blacklist_count;
extern void frame_region_not_free(__address base, size_t size);
extern void zone_create_in_region(__address base, size_t size);

extern spinlock_t zone_head_lock;	/**< this lock protects zone_head list */
extern link_t zone_head;		/**< list of all zones in the system */

extern zone_t *zone_create(__address start, size_t size, int flags);
extern void zone_attach(zone_t *zone);

extern void frame_init(void);
extern void frame_initialize(frame_t *frame, zone_t *zone);

__address frame_alloc_generic(__u8 order, int flags, int * status, zone_t **pzone);


extern void frame_free(__address addr);

zone_t * get_zone_by_frame(frame_t * frame);

/*
 * Buddy system operations
 */
link_t * zone_buddy_find_buddy(buddy_system_t *b, link_t * buddy);
link_t * zone_buddy_bisect(buddy_system_t *b, link_t * block);
link_t * zone_buddy_coalesce(buddy_system_t *b, link_t * buddy_l, link_t * buddy_r);
void zone_buddy_set_order(buddy_system_t *b, link_t * block, __u8 order);
__u8 zone_buddy_get_order(buddy_system_t *b, link_t * block);
void zone_buddy_mark_busy(buddy_system_t *b, link_t * block);
extern frame_t * frame_addr2frame(__address addr);

/*
 * TODO: Implement the following functions.
 */
extern frame_t *frame_reference(frame_t *frame);
extern void frame_release(frame_t *frame);


/*
 * Console functions
 */
extern void zone_print_list(void);
extern void zone_print_one(__address base);

#endif
