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
#include <list.h>
#include <synch/spinlock.h>
#include <mm/buddy.h>

#define FRAME_KA	1	/* skip frames conflicting with user address space */
#define FRAME_PANIC	2	/* panic on failure */

#define FRAME2ADDR(zone, frame)			((zone)->base + ((frame) - (zone)->frames) * FRAME_SIZE)
#define ADDR2FRAME(zone, addr)			(&((zone)->frames[((addr) - (zone)->base) / FRAME_SIZE]))
#define FRAME_INDEX(zone, frame)		((count_t)((frame) - (zone)->frames))
#define IS_BUDDY_LEFT_BLOCK(zone, frame)	((FRAME_INDEX((zone), (frame)) % (1 >> ((frame)->buddy_order + 1))) == 0)
#define IS_BUDDY_RIGHT_BLOCK(zone, frame)	((FRAME_INDEX((zone), (frame)) % (1 >> ((frame)->buddy_order + 1))) == (1 >> (frame)->buddy_order))



struct zone {
	link_t link;		/**< link to previous and next zone */

	spinlock_t lock;	/**< this lock protects everything below */
	__address base;		/**< physical address of the first frame in the frames array */
	frame_t *frames;	/**< array of frame_t structures in this zone */
	link_t free_head;	/**< list of free frame_t structures */
	count_t free_count;	/**< number of frame_t structures in free list */
	count_t busy_count;	/**< number of frame_t structures not in free list */
	
	buddy_system_t * buddy_system; /**< buddy system for the zone */
	int flags;
};

struct frame {
	count_t refcount;	/**< when == 0, the frame is in free list */
	link_t link;		/**< link to zone free list when refcount == 0 */
	__u8 buddy_order;	/**< buddy system block order */
	link_t buddy_link;	/**< link to the next free block inside one order*/
};

extern spinlock_t zone_head_lock;	/**< this lock protects zone_head list */
extern link_t zone_head;		/**< list of all zones in the system */

extern void zone_init(void);
extern zone_t *zone_create(__address start, size_t size, int flags);
extern void zone_attach(zone_t *zone);

extern void frame_init(void);
extern void frame_initialize(frame_t *frame, zone_t *zone);
__address frame_alloc(int flags);
extern void frame_free(__address addr);
extern void frame_not_free(__address addr);
extern void frame_region_not_free(__address start, __address stop);
zone_t * get_zone_by_frame(frame_t * frame);

/*
 * Buddy system operations
 */
link_t * zone_buddy_find_buddy(buddy_system_t *b, link_t * buddy);
link_t * zone_buddy_bisect(buddy_system_t *b, link_t * block);
link_t * zone_buddy_coalesce(buddy_system_t *b, link_t * buddy_l, link_t * buddy_r);
void zone_buddy_set_order(buddy_system_t *b, link_t * block, __u8 order);
__u8 zone_buddy_get_order(buddy_system_t *b, link_t * block);

__address zone_buddy_frame_alloc(int flags, __u8 order);
void zone_buddy_frame_free(__address addr);

/*
 * TODO: Implement the following functions.
 */
extern frame_t *frame_reference(frame_t *frame);
extern void frame_release(frame_t *frame);

#endif
