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

#include <typedefs.h>
#include <adt/list.h>
#include <mm/buddy.h>
#include <synch/spinlock.h>
#include <arch/mm/page.h>
#include <arch/mm/frame.h>

#define ONE_FRAME    0
#define TWO_FRAMES   1
#define FOUR_FRAMES  2


#ifdef ARCH_STACK_FRAMES
	#define STACK_FRAMES  ARCH_STACK_FRAMES
#else
	#define STACK_FRAMES  ONE_FRAME
#endif

/** Maximum number of zones in the system. */
#define ZONES_MAX  32

typedef uint8_t frame_flags_t;

/** Convert the frame address to kernel VA. */
#define FRAME_KA          0x01
/** Do not panic and do not sleep on failure. */
#define FRAME_ATOMIC      0x02
/** Do not start reclaiming when no free memory. */
#define FRAME_NO_RECLAIM  0x04

typedef uint8_t zone_flags_t;

/** Available zone (free for allocation) */
#define ZONE_AVAILABLE  0x00
/** Zone is reserved (not available for allocation) */
#define ZONE_RESERVED   0x08
/** Zone is used by firmware (not available for allocation) */
#define ZONE_FIRMWARE   0x10

/** Currently there is no equivalent zone flags
    for frame flags */
#define FRAME_TO_ZONE_FLAGS(frame_flags)  0

typedef struct {
	size_t refcount;     /**< Tracking of shared frames */
	uint8_t buddy_order;  /**< Buddy system block order */
	link_t buddy_link;    /**< Link to the next free block inside
                               one order */
	void *parent;         /**< If allocated by slab, this points there */
} frame_t;

typedef struct {
	pfn_t base;                    /**< Frame_no of the first frame
                                        in the frames array */
	size_t count;                 /**< Size of zone */
	size_t free_count;            /**< Number of free frame_t
                                        structures */
	size_t busy_count;            /**< Number of busy frame_t
                                        structures */
	zone_flags_t flags;            /**< Type of the zone */
	
	frame_t *frames;               /**< Array of frame_t structures
                                        in this zone */
	buddy_system_t *buddy_system;  /**< Buddy system for the zone */
} zone_t;

/*
 * The zoneinfo.lock must be locked when accessing zoneinfo structure.
 * Some of the attributes in zone_t structures are 'read-only'
 */
typedef struct {
	SPINLOCK_DECLARE(lock);
	size_t count;
	zone_t info[ZONES_MAX];
} zones_t;

extern zones_t zones;

static inline uintptr_t PFN2ADDR(pfn_t frame)
{
	return (uintptr_t) (frame << FRAME_WIDTH);
}

static inline pfn_t ADDR2PFN(uintptr_t addr)
{
	return (pfn_t) (addr >> FRAME_WIDTH);
}

static inline size_t SIZE2FRAMES(size_t size)
{
	if (!size)
		return 0;
	return (size_t) ((size - 1) >> FRAME_WIDTH) + 1;
}

static inline size_t FRAMES2SIZE(size_t frames)
{
	return (size_t) (frames << FRAME_WIDTH);
}

static inline bool zone_flags_available(zone_flags_t flags)
{
	return ((flags & (ZONE_RESERVED | ZONE_FIRMWARE)) == 0);
}

#define IS_BUDDY_ORDER_OK(index, order) \
    ((~(((unative_t) -1) << (order)) & (index)) == 0)
#define IS_BUDDY_LEFT_BLOCK(zone, frame) \
    (((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x01) == 0)
#define IS_BUDDY_RIGHT_BLOCK(zone, frame) \
    (((frame_index((zone), (frame)) >> (frame)->buddy_order) & 0x01) == 1)
#define IS_BUDDY_LEFT_BLOCK_ABS(zone, frame) \
    (((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x01) == 0)
#define IS_BUDDY_RIGHT_BLOCK_ABS(zone, frame) \
    (((frame_index_abs((zone), (frame)) >> (frame)->buddy_order) & 0x01) == 1)

#define frame_alloc(order, flags) \
    frame_alloc_generic(order, flags, NULL)

extern void frame_init(void);
extern void *frame_alloc_generic(uint8_t, frame_flags_t, size_t *);
extern void frame_free(uintptr_t);
extern void frame_reference_add(pfn_t);

extern size_t find_zone(pfn_t frame, size_t count, size_t hint);
extern size_t zone_create(pfn_t, size_t, pfn_t, zone_flags_t);
extern void *frame_get_parent(pfn_t, size_t);
extern void frame_set_parent(pfn_t, void *, size_t);
extern void frame_mark_unavailable(pfn_t, size_t);
extern uintptr_t zone_conf_size(size_t);
extern bool zone_merge(size_t, size_t);
extern void zone_merge_all(void);
extern uint64_t zone_total_size(void);

/*
 * Console functions
 */
extern void zone_print_list(void);
extern void zone_print_one(size_t);

#endif

/** @}
 */
