/*
 * Copyright (c) 2001-2005 Jakub Jermar
 * Copyright (c) 2005 Sergey Bondari
 * Copyright (c) 2009 Martin Decky
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

/**
 * @file
 * @brief Physical frame allocator.
 *
 * This file contains the physical frame allocator and memory zone management.
 * The frame allocator is built on top of the buddy allocator.
 *
 * @see buddy.c
 */

#include <typedefs.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <panic.h>
#include <debug.h>
#include <adt/list.h>
#include <synch/mutex.h>
#include <synch/condvar.h>
#include <arch/asm.h>
#include <arch.h>
#include <print.h>
#include <align.h>
#include <mm/slab.h>
#include <bitops.h>
#include <macros.h>
#include <config.h>

zones_t zones;

/*
 * Synchronization primitives used to sleep when there is no memory
 * available.
 */
mutex_t mem_avail_mtx;
condvar_t mem_avail_cv;
size_t mem_avail_req = 0;  /**< Number of frames requested. */
size_t mem_avail_gen = 0;  /**< Generation counter. */

/********************/
/* Helper functions */
/********************/

static inline size_t frame_index(zone_t *zone, frame_t *frame)
{
	return (size_t) (frame - zone->frames);
}

static inline size_t frame_index_abs(zone_t *zone, frame_t *frame)
{
	return (size_t) (frame - zone->frames) + zone->base;
}

static inline bool frame_index_valid(zone_t *zone, size_t index)
{
	return (index < zone->count);
}

static inline size_t make_frame_index(zone_t *zone, frame_t *frame)
{
	return (frame - zone->frames);
}

/** Initialize frame structure.
 *
 * @param frame Frame structure to be initialized.
 *
 */
static void frame_initialize(frame_t *frame)
{
	frame->refcount = 1;
	frame->buddy_order = 0;
}

/*******************/
/* Zones functions */
/*******************/

/** Insert-sort zone into zones list.
 *
 * Assume interrupts are disabled and zones lock is
 * locked.
 *
 * @param base  Base frame of the newly inserted zone.
 * @param count Number of frames of the newly inserted zone.
 *
 * @return Zone number on success, -1 on error.
 *
 */
static size_t zones_insert_zone(pfn_t base, size_t count)
{
	if (zones.count + 1 == ZONES_MAX) {
		printf("Maximum zone count %u exceeded!\n", ZONES_MAX);
		return (size_t) -1;
	}
	
	size_t i;
	for (i = 0; i < zones.count; i++) {
		/* Check for overlap */
		if (overlaps(base, count,
		    zones.info[i].base, zones.info[i].count)) {
			printf("Zones overlap!\n");
			return (size_t) -1;
		}
		if (base < zones.info[i].base)
			break;
	}
	
	/* Move other zones up */
	size_t j;
	for (j = zones.count; j > i; j--) {
		zones.info[j] = zones.info[j - 1];
		zones.info[j].buddy_system->data =
		    (void *) &zones.info[j - 1];
	}
	
	zones.count++;
	
	return i;
}

/** Get total available frames.
 *
 * Assume interrupts are disabled and zones lock is
 * locked.
 *
 * @return Total number of available frames.
 *
 */
#ifdef CONFIG_DEBUG
static size_t total_frames_free(void)
{
	size_t total = 0;
	size_t i;
	for (i = 0; i < zones.count; i++)
		total += zones.info[i].free_count;
	
	return total;
}
#endif

/** Find a zone with a given frames.
 *
 * Assume interrupts are disabled and zones lock is
 * locked.
 *
 * @param frame Frame number contained in zone.
 * @param count Number of frames to look for.
 * @param hint  Used as zone hint.
 *
 * @return Zone index or -1 if not found.
 *
 */
size_t find_zone(pfn_t frame, size_t count, size_t hint)
{
	if (hint >= zones.count)
		hint = 0;
	
	size_t i = hint;
	do {
		if ((zones.info[i].base <= frame)
		    && (zones.info[i].base + zones.info[i].count >= frame + count))
			return i;
		
		i++;
		if (i >= zones.count)
			i = 0;
	} while (i != hint);
	
	return (size_t) -1;
}

/** @return True if zone can allocate specified order */
static bool zone_can_alloc(zone_t *zone, uint8_t order)
{
	return (zone_flags_available(zone->flags)
	    && buddy_system_can_alloc(zone->buddy_system, order));
}

/** Find a zone that can allocate order frames.
 *
 * Assume interrupts are disabled and zones lock is
 * locked.
 *
 * @param order Size (2^order) of free space we are trying to find.
 * @param flags Required flags of the target zone.
 * @param hind  Preferred zone.
 *
 */
static size_t find_free_zone(uint8_t order, zone_flags_t flags, size_t hint)
{
	if (hint >= zones.count)
		hint = 0;
	
	size_t i = hint;
	do {
		/*
		 * Check whether the zone meets the search criteria.
		 */
		if ((zones.info[i].flags & flags) == flags) {
			/*
			 * Check if the zone has 2^order frames area available.
			 */
			if (zone_can_alloc(&zones.info[i], order))
				return i;
		}
		
		i++;
		if (i >= zones.count)
			i = 0;
	} while (i != hint);
	
	return (size_t) -1;
}

/**************************/
/* Buddy system functions */
/**************************/

/** Buddy system find_block implementation.
 *
 * Find block that is parent of current list.
 * That means go to lower addresses, until such block is found
 *
 * @param order Order of parent must be different then this
 *              parameter!!
 *
 */
static link_t *zone_buddy_find_block(buddy_system_t *buddy, link_t *child,
    uint8_t order)
{
	frame_t *frame = list_get_instance(child, frame_t, buddy_link);
	zone_t *zone = (zone_t *) buddy->data;
	
	size_t index = frame_index(zone, frame);
	do {
		if (zone->frames[index].buddy_order != order)
			return &zone->frames[index].buddy_link;
	} while (index-- > 0);
	
	return NULL;
}

/** Buddy system find_buddy implementation.
 *
 * @param buddy Buddy system.
 * @param block Block for which buddy should be found.
 *
 * @return Buddy for given block if found.
 *
 */
static link_t *zone_buddy_find_buddy(buddy_system_t *buddy, link_t *block) 
{
	frame_t *frame = list_get_instance(block, frame_t, buddy_link);
	zone_t *zone = (zone_t *) buddy->data;
	ASSERT(IS_BUDDY_ORDER_OK(frame_index_abs(zone, frame),
	    frame->buddy_order));
	
	bool is_left = IS_BUDDY_LEFT_BLOCK_ABS(zone, frame);
	
	size_t index;
	if (is_left) {
		index = (frame_index(zone, frame)) +
		    (1 << frame->buddy_order);
	} else {	/* is_right */
		index = (frame_index(zone, frame)) -
		    (1 << frame->buddy_order);
	}
	
	if (frame_index_valid(zone, index)) {
		if ((zone->frames[index].buddy_order == frame->buddy_order) &&
		    (zone->frames[index].refcount == 0)) {
			return &zone->frames[index].buddy_link;
		}
	}
	
	return NULL;
}

/** Buddy system bisect implementation.
 *
 * @param buddy Buddy system.
 * @param block Block to bisect.
 *
 * @return Right block.
 *
 */
static link_t *zone_buddy_bisect(buddy_system_t *buddy, link_t *block)
{
	frame_t *frame_l = list_get_instance(block, frame_t, buddy_link);
	frame_t *frame_r = (frame_l + (1 << (frame_l->buddy_order - 1)));
	
	return &frame_r->buddy_link;
}

/** Buddy system coalesce implementation.
 *
 * @param buddy   Buddy system.
 * @param block_1 First block.
 * @param block_2 First block's buddy.
 *
 * @return Coalesced block (actually block that represents lower
 *         address).
 *
 */
static link_t *zone_buddy_coalesce(buddy_system_t *buddy, link_t *block_1,
    link_t *block_2)
{
	frame_t *frame1 = list_get_instance(block_1, frame_t, buddy_link);
	frame_t *frame2 = list_get_instance(block_2, frame_t, buddy_link);
	
	return ((frame1 < frame2) ? block_1 : block_2);
}

/** Buddy system set_order implementation.
 *
 * @param buddy Buddy system.
 * @param block Buddy system block.
 * @param order Order to set.
 *
 */
static void zone_buddy_set_order(buddy_system_t *buddy, link_t *block,
    uint8_t order)
{
	list_get_instance(block, frame_t, buddy_link)->buddy_order = order;
}

/** Buddy system get_order implementation.
 *
 * @param buddy Buddy system.
 * @param block Buddy system block.
 *
 * @return Order of block.
 *
 */
static uint8_t zone_buddy_get_order(buddy_system_t *buddy, link_t *block)
{
	return list_get_instance(block, frame_t, buddy_link)->buddy_order;
}

/** Buddy system mark_busy implementation.
 *
 * @param buddy Buddy system.
 * @param block Buddy system block.
 *
 */
static void zone_buddy_mark_busy(buddy_system_t *buddy, link_t * block)
{
	list_get_instance(block, frame_t, buddy_link)->refcount = 1;
}

/** Buddy system mark_available implementation.
 *
 * @param buddy Buddy system.
 * @param block Buddy system block.
 */
static void zone_buddy_mark_available(buddy_system_t *buddy, link_t *block)
{
	list_get_instance(block, frame_t, buddy_link)->refcount = 0;
}

static buddy_system_operations_t zone_buddy_system_operations = {
	.find_buddy = zone_buddy_find_buddy,
	.bisect = zone_buddy_bisect,
	.coalesce = zone_buddy_coalesce,
	.set_order = zone_buddy_set_order,
	.get_order = zone_buddy_get_order,
	.mark_busy = zone_buddy_mark_busy,
	.mark_available = zone_buddy_mark_available,
	.find_block = zone_buddy_find_block
};

/******************/
/* Zone functions */
/******************/

/** Allocate frame in particular zone.
 *
 * Assume zone is locked and is available for allocation.
 * Panics if allocation is impossible.
 *
 * @param zone  Zone to allocate from.
 * @param order Allocate exactly 2^order frames.
 *
 * @return Frame index in zone.
 *
 */
static pfn_t zone_frame_alloc(zone_t *zone, uint8_t order)
{
	ASSERT(zone_flags_available(zone->flags));
	
	/* Allocate frames from zone buddy system */
	link_t *link = buddy_system_alloc(zone->buddy_system, order);
	
	ASSERT(link);
	
	/* Update zone information. */
	zone->free_count -= (1 << order);
	zone->busy_count += (1 << order);
	
	/* Frame will be actually a first frame of the block. */
	frame_t *frame = list_get_instance(link, frame_t, buddy_link);
	
	/* Get frame address */
	return make_frame_index(zone, frame);
}

/** Free frame from zone.
 *
 * Assume zone is locked and is available for deallocation.
 *
 * @param zone      Pointer to zone from which the frame is to be freed.
 * @param frame_idx Frame index relative to zone.
 *
 */
static void zone_frame_free(zone_t *zone, size_t frame_idx)
{
	ASSERT(zone_flags_available(zone->flags));
	
	frame_t *frame = &zone->frames[frame_idx];
	
	/* Remember frame order */
	uint8_t order = frame->buddy_order;
	
	ASSERT(frame->refcount);
	
	if (!--frame->refcount) {
		buddy_system_free(zone->buddy_system, &frame->buddy_link);
		
		/* Update zone information. */
		zone->free_count += (1 << order);
		zone->busy_count -= (1 << order);
	}
}

/** Return frame from zone. */
static frame_t *zone_get_frame(zone_t *zone, size_t frame_idx)
{
	ASSERT(frame_idx < zone->count);
	return &zone->frames[frame_idx];
}

/** Mark frame in zone unavailable to allocation. */
static void zone_mark_unavailable(zone_t *zone, size_t frame_idx)
{
	ASSERT(zone_flags_available(zone->flags));
	
	frame_t *frame = zone_get_frame(zone, frame_idx);
	if (frame->refcount)
		return;
	
	link_t *link __attribute__ ((unused));
	
	link = buddy_system_alloc_block(zone->buddy_system,
	    &frame->buddy_link);
	
	ASSERT(link);
	zone->free_count--;
}

/** Merge two zones.
 *
 * Expect buddy to point to space at least zone_conf_size large.
 * Assume z1 & z2 are locked and compatible and zones lock is
 * locked.
 *
 * @param z1     First zone to merge.
 * @param z2     Second zone to merge.
 * @param old_z1 Original date of the first zone.
 * @param buddy  Merged zone buddy.
 *
 */
static void zone_merge_internal(size_t z1, size_t z2, zone_t *old_z1, buddy_system_t *buddy)
{
	ASSERT(zone_flags_available(zones.info[z1].flags));
	ASSERT(zone_flags_available(zones.info[z2].flags));
	ASSERT(zones.info[z1].flags == zones.info[z2].flags);
	ASSERT(zones.info[z1].base < zones.info[z2].base);
	ASSERT(!overlaps(zones.info[z1].base, zones.info[z1].count,
	    zones.info[z2].base, zones.info[z2].count));
	
	/* Difference between zone bases */
	pfn_t base_diff = zones.info[z2].base - zones.info[z1].base;
	
	zones.info[z1].count = base_diff + zones.info[z2].count;
	zones.info[z1].free_count += zones.info[z2].free_count;
	zones.info[z1].busy_count += zones.info[z2].busy_count;
	zones.info[z1].buddy_system = buddy;
	
	uint8_t order = fnzb(zones.info[z1].count);
	buddy_system_create(zones.info[z1].buddy_system, order,
	    &zone_buddy_system_operations, (void *) &zones.info[z1]);
	
	zones.info[z1].frames =
	    (frame_t *) ((uint8_t *) zones.info[z1].buddy_system
	    + buddy_conf_size(order));
	
	/* This marks all frames busy */
	size_t i;
	for (i = 0; i < zones.info[z1].count; i++)
		frame_initialize(&zones.info[z1].frames[i]);
	
	/* Copy frames from both zones to preserve full frame orders,
	 * parents etc. Set all free frames with refcount = 0 to 1, because
	 * we add all free frames to buddy allocator later again, clearing
	 * order to 0. Don't set busy frames with refcount = 0, as they
	 * will not be reallocated during merge and it would make later
	 * problems with allocation/free.
	 */
	for (i = 0; i < old_z1->count; i++)
		zones.info[z1].frames[i] = old_z1->frames[i];
	
	for (i = 0; i < zones.info[z2].count; i++)
		zones.info[z1].frames[base_diff + i]
		    = zones.info[z2].frames[i];
	
	i = 0;
	while (i < zones.info[z1].count) {
		if (zones.info[z1].frames[i].refcount) {
			/* Skip busy frames */
			i += 1 << zones.info[z1].frames[i].buddy_order;
		} else {
			/* Free frames, set refcount = 1
			 * (all free frames have refcount == 0, we need not
			 * to check the order)
			 */
			zones.info[z1].frames[i].refcount = 1;
			zones.info[z1].frames[i].buddy_order = 0;
			i++;
		}
	}
	
	/* Add free blocks from the original zone z1 */
	while (zone_can_alloc(old_z1, 0)) {
		/* Allocate from the original zone */
		pfn_t frame_idx = zone_frame_alloc(old_z1, 0);
		
		/* Free the frame from the merged zone */
		frame_t *frame = &zones.info[z1].frames[frame_idx];
		frame->refcount = 0;
		buddy_system_free(zones.info[z1].buddy_system, &frame->buddy_link);
	}
	
	/* Add free blocks from the original zone z2 */
	while (zone_can_alloc(&zones.info[z2], 0)) {
		/* Allocate from the original zone */
		pfn_t frame_idx = zone_frame_alloc(&zones.info[z2], 0);
		
		/* Free the frame from the merged zone */
		frame_t *frame = &zones.info[z1].frames[base_diff + frame_idx];
		frame->refcount = 0;
		buddy_system_free(zones.info[z1].buddy_system, &frame->buddy_link);
	}
}

/** Return old configuration frames into the zone.
 *
 * We have two cases:
 * - The configuration data is outside the zone
 *   -> do nothing (perhaps call frame_free?)
 * - The configuration data was created by zone_create
 *   or updated by reduce_region -> free every frame
 *
 * @param znum  The actual zone where freeing should occur.
 * @param pfn   Old zone configuration frame.
 * @param count Old zone frame count.
 *
 */
static void return_config_frames(size_t znum, pfn_t pfn, size_t count)
{
	ASSERT(zone_flags_available(zones.info[znum].flags));
	
	size_t cframes = SIZE2FRAMES(zone_conf_size(count));
	
	if ((pfn < zones.info[znum].base)
	    || (pfn >= zones.info[znum].base + zones.info[znum].count))
		return;
	
	frame_t *frame __attribute__ ((unused));

	frame = &zones.info[znum].frames[pfn - zones.info[znum].base];
	ASSERT(!frame->buddy_order);
	
	size_t i;
	for (i = 0; i < cframes; i++) {
		zones.info[znum].busy_count++;
		zone_frame_free(&zones.info[znum],
		    pfn - zones.info[znum].base + i);
	}
}

/** Reduce allocated block to count of order 0 frames.
 *
 * The allocated block needs 2^order frames. Reduce all frames
 * in the block to order 0 and free the unneeded frames. This means that
 * when freeing the previously allocated block starting with frame_idx,
 * you have to free every frame.
 *
 * @param znum      Zone.
 * @param frame_idx Index the first frame of the block.
 * @param count     Allocated frames in block.
 *
 */
static void zone_reduce_region(size_t znum, pfn_t frame_idx, size_t count)
{
	ASSERT(zone_flags_available(zones.info[znum].flags));
	ASSERT(frame_idx + count < zones.info[znum].count);
	
	uint8_t order = zones.info[znum].frames[frame_idx].buddy_order;
	ASSERT((size_t) (1 << order) >= count);
	
	/* Reduce all blocks to order 0 */
	size_t i;
	for (i = 0; i < (size_t) (1 << order); i++) {
		frame_t *frame = &zones.info[znum].frames[i + frame_idx];
		frame->buddy_order = 0;
		if (!frame->refcount)
			frame->refcount = 1;
		ASSERT(frame->refcount == 1);
	}
	
	/* Free unneeded frames */
	for (i = count; i < (size_t) (1 << order); i++)
		zone_frame_free(&zones.info[znum], i + frame_idx);
}

/** Merge zones z1 and z2.
 *
 * The merged zones must be 2 zones with no zone existing in between
 * (which means that z2 = z1 + 1). Both zones must be available zones
 * with the same flags.
 *
 * When you create a new zone, the frame allocator configuration does
 * not to be 2^order size. Once the allocator is running it is no longer
 * possible, merged configuration data occupies more space :-/
 *
 * The function uses
 *
 */
bool zone_merge(size_t z1, size_t z2)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	bool ret = true;
	
	/* We can join only 2 zones with none existing inbetween,
	 * the zones have to be available and with the same
	 * set of flags
	 */
	if ((z1 >= zones.count) || (z2 >= zones.count)
	    || (z2 - z1 != 1)
	    || (!zone_flags_available(zones.info[z1].flags))
	    || (!zone_flags_available(zones.info[z2].flags))
	    || (zones.info[z1].flags != zones.info[z2].flags)) {
		ret = false;
		goto errout;
	}
	
	pfn_t cframes = SIZE2FRAMES(zone_conf_size(
	    zones.info[z2].base - zones.info[z1].base
	    + zones.info[z2].count));
	
	uint8_t order;
	if (cframes == 1)
		order = 0;
	else
		order = fnzb(cframes - 1) + 1;
	
	/* Allocate merged zone data inside one of the zones */
	pfn_t pfn;
	if (zone_can_alloc(&zones.info[z1], order)) {
		pfn = zones.info[z1].base + zone_frame_alloc(&zones.info[z1], order);
	} else if (zone_can_alloc(&zones.info[z2], order)) {
		pfn = zones.info[z2].base + zone_frame_alloc(&zones.info[z2], order);
	} else {
		ret = false;
		goto errout;
	}
	
	/* Preserve original data from z1 */
	zone_t old_z1 = zones.info[z1];
	old_z1.buddy_system->data = (void *) &old_z1;
	
	/* Do zone merging */
	buddy_system_t *buddy = (buddy_system_t *) PA2KA(PFN2ADDR(pfn));
	zone_merge_internal(z1, z2, &old_z1, buddy);
	
	/* Free unneeded config frames */
	zone_reduce_region(z1, pfn - zones.info[z1].base, cframes);
	
	/* Subtract zone information from busy frames */
	zones.info[z1].busy_count -= cframes;
	
	/* Free old zone information */
	return_config_frames(z1,
	    ADDR2PFN(KA2PA((uintptr_t) old_z1.frames)), old_z1.count);
	return_config_frames(z1,
	    ADDR2PFN(KA2PA((uintptr_t) zones.info[z2].frames)),
	    zones.info[z2].count);
	
	/* Move zones down */
	size_t i;
	for (i = z2 + 1; i < zones.count; i++) {
		zones.info[i - 1] = zones.info[i];
		zones.info[i - 1].buddy_system->data =
		    (void *) &zones.info[i - 1];
	}
	
	zones.count--;
	
errout:
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	return ret;
}

/** Merge all mergeable zones into one big zone.
 *
 * It is reasonable to do this on systems where
 * BIOS reports parts in chunks, so that we could
 * have 1 zone (it's faster).
 *
 */
void zone_merge_all(void)
{
	size_t i = 0;
	while (i < zones.count) {
		if (!zone_merge(i, i + 1))
			i++;
	}
}

/** Create new frame zone.
 *
 * @param zone  Zone to construct.
 * @param buddy Address of buddy system configuration information.
 * @param start Physical address of the first frame within the zone.
 * @param count Count of frames in zone.
 * @param flags Zone flags.
 *
 * @return Initialized zone.
 *
 */
static void zone_construct(zone_t *zone, buddy_system_t *buddy, pfn_t start, size_t count, zone_flags_t flags)
{
	zone->base = start;
	zone->count = count;
	zone->flags = flags;
	zone->free_count = count;
	zone->busy_count = 0;
	zone->buddy_system = buddy;
	
	if (zone_flags_available(flags)) {
		/*
		 * Compute order for buddy system and initialize
		 */
		uint8_t order = fnzb(count);
		buddy_system_create(zone->buddy_system, order,
		    &zone_buddy_system_operations, (void *) zone);
		
		/* Allocate frames _after_ the confframe */
		
		/* Check sizes */
		zone->frames = (frame_t *) ((uint8_t *) zone->buddy_system +
		    buddy_conf_size(order));
		
		size_t i;
		for (i = 0; i < count; i++)
			frame_initialize(&zone->frames[i]);
		
		/* Stuffing frames */
		for (i = 0; i < count; i++) {
			zone->frames[i].refcount = 0;
			buddy_system_free(zone->buddy_system, &zone->frames[i].buddy_link);
		}
	} else
		zone->frames = NULL;
}

/** Compute configuration data size for zone.
 *
 * @param count Size of zone in frames.
 *
 * @return Size of zone configuration info (in bytes).
 *
 */
uintptr_t zone_conf_size(size_t count)
{
	return (count * sizeof(frame_t) + buddy_conf_size(fnzb(count)));
}

/** Create and add zone to system.
 *
 * @param start     First frame number (absolute).
 * @param count     Size of zone in frames.
 * @param confframe Where configuration frames are supposed to be.
 *                  Automatically checks, that we will not disturb the
 *                  kernel and possibly init. If confframe is given
 *                  _outside_ this zone, it is expected, that the area is
 *                  already marked BUSY and big enough to contain
 *                  zone_conf_size() amount of data. If the confframe is
 *                  inside the area, the zone free frame information is
 *                  modified not to include it.
 *
 * @return Zone number or -1 on error.
 *
 */
size_t zone_create(pfn_t start, size_t count, pfn_t confframe, zone_flags_t flags)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	if (zone_flags_available(flags)) {  /* Create available zone */
		/* Theoretically we could have NULL here, practically make sure
		 * nobody tries to do that. If some platform requires, remove
		 * the assert
		 */
		ASSERT(confframe != NULL);
		
		/* If confframe is supposed to be inside our zone, then make sure
		 * it does not span kernel & init
		 */
		size_t confcount = SIZE2FRAMES(zone_conf_size(count));
		if ((confframe >= start) && (confframe < start + count)) {
			for (; confframe < start + count; confframe++) {
				uintptr_t addr = PFN2ADDR(confframe);
				if (overlaps(addr, PFN2ADDR(confcount),
				    KA2PA(config.base), config.kernel_size))
					continue;
				
				if (overlaps(addr, PFN2ADDR(confcount),
				    KA2PA(config.stack_base), config.stack_size))
					continue;
				
				bool overlap = false;
				size_t i;
				for (i = 0; i < init.cnt; i++)
					if (overlaps(addr, PFN2ADDR(confcount),
					    KA2PA(init.tasks[i].addr),
					    init.tasks[i].size)) {
						overlap = true;
						break;
					}
				if (overlap)
					continue;
				
				break;
			}
			
			if (confframe >= start + count)
				panic("Cannot find configuration data for zone.");
		}
		
		size_t znum = zones_insert_zone(start, count);
		if (znum == (size_t) -1) {
			spinlock_unlock(&zones.lock);
			interrupts_restore(ipl);
			return (size_t) -1;
		}
		
		buddy_system_t *buddy = (buddy_system_t *) PA2KA(PFN2ADDR(confframe));
		zone_construct(&zones.info[znum], buddy, start, count, flags);
		
		/* If confdata in zone, mark as unavailable */
		if ((confframe >= start) && (confframe < start + count)) {
			size_t i;
			for (i = confframe; i < confframe + confcount; i++)
				zone_mark_unavailable(&zones.info[znum],
				    i - zones.info[znum].base);
		}
		
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		
		return znum;
	}
	
	/* Non-available zone */
	size_t znum = zones_insert_zone(start, count);
	if (znum == (size_t) -1) {
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		return (size_t) -1;
	}
	zone_construct(&zones.info[znum], NULL, start, count, flags);
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	return znum;
}

/*******************/
/* Frame functions */
/*******************/

/** Set parent of frame. */
void frame_set_parent(pfn_t pfn, void *data, size_t hint)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	size_t znum = find_zone(pfn, 1, hint);
	
	ASSERT(znum != (size_t) -1);
	
	zone_get_frame(&zones.info[znum],
	    pfn - zones.info[znum].base)->parent = data;
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

void *frame_get_parent(pfn_t pfn, size_t hint)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	size_t znum = find_zone(pfn, 1, hint);
	
	ASSERT(znum != (size_t) -1);
	
	void *res = zone_get_frame(&zones.info[znum],
	    pfn - zones.info[znum].base)->parent;
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	return res;
}

/** Allocate power-of-two frames of physical memory.
 *
 * @param order Allocate exactly 2^order frames.
 * @param flags Flags for host zone selection and address processing.
 * @param pzone Preferred zone.
 *
 * @return Physical address of the allocated frame.
 *
 */
void *frame_alloc_generic(uint8_t order, frame_flags_t flags, size_t *pzone)
{
	size_t size = ((size_t) 1) << order;
	ipl_t ipl;
	size_t hint = pzone ? (*pzone) : 0;
	
loop:
	ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	/*
	 * First, find suitable frame zone.
	 */
	size_t znum = find_free_zone(order,
	    FRAME_TO_ZONE_FLAGS(flags), hint);
	
	/* If no memory, reclaim some slab memory,
	   if it does not help, reclaim all */
	if ((znum == (size_t) -1) && (!(flags & FRAME_NO_RECLAIM))) {
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		
		size_t freed = slab_reclaim(0);
		
		ipl = interrupts_disable();
		spinlock_lock(&zones.lock);
		
		if (freed > 0)
			znum = find_free_zone(order,
			    FRAME_TO_ZONE_FLAGS(flags), hint);
		
		if (znum == (size_t) -1) {
			spinlock_unlock(&zones.lock);
			interrupts_restore(ipl);
			
			freed = slab_reclaim(SLAB_RECLAIM_ALL);
			
			ipl = interrupts_disable();
			spinlock_lock(&zones.lock);
			
			if (freed > 0)
				znum = find_free_zone(order,
				    FRAME_TO_ZONE_FLAGS(flags), hint);
		}
	}
	
	if (znum == (size_t) -1) {
		if (flags & FRAME_ATOMIC) {
			spinlock_unlock(&zones.lock);
			interrupts_restore(ipl);
			return NULL;
		}
		
#ifdef CONFIG_DEBUG
		size_t avail = total_frames_free();
#endif
		
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		
		/*
		 * Sleep until some frames are available again.
		 */
		
#ifdef CONFIG_DEBUG
		printf("Thread %" PRIu64 " waiting for %" PRIs " frames, "
		    "%" PRIs " available.\n", THREAD->tid, size, avail);
#endif
		
		mutex_lock(&mem_avail_mtx);
		
		if (mem_avail_req > 0)
			mem_avail_req = min(mem_avail_req, size);
		else
			mem_avail_req = size;
		size_t gen = mem_avail_gen;
		
		while (gen == mem_avail_gen)
			condvar_wait(&mem_avail_cv, &mem_avail_mtx);
		
		mutex_unlock(&mem_avail_mtx);
		
#ifdef CONFIG_DEBUG
		printf("Thread %" PRIu64 " woken up.\n", THREAD->tid);
#endif
		
		goto loop;
	}
	
	pfn_t pfn = zone_frame_alloc(&zones.info[znum], order)
	    + zones.info[znum].base;
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	if (pzone)
		*pzone = znum;
	
	if (flags & FRAME_KA)
		return (void *) PA2KA(PFN2ADDR(pfn));
	
	return (void *) PFN2ADDR(pfn);
}

/** Free a frame.
 *
 * Find respective frame structure for supplied physical frame address.
 * Decrement frame reference count. If it drops to zero, move the frame
 * structure to free list.
 *
 * @param frame Physical Address of of the frame to be freed.
 *
 */
void frame_free(uintptr_t frame)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	/*
	 * First, find host frame zone for addr.
	 */
	pfn_t pfn = ADDR2PFN(frame);
	size_t znum = find_zone(pfn, 1, NULL);
	
	ASSERT(znum != (size_t) -1);
	
	zone_frame_free(&zones.info[znum], pfn - zones.info[znum].base);
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	/*
	 * Signal that some memory has been freed.
	 */
	mutex_lock(&mem_avail_mtx);
	if (mem_avail_req > 0)
		mem_avail_req--;
	
	if (mem_avail_req == 0) {
		mem_avail_gen++;
		condvar_broadcast(&mem_avail_cv);
	}
	mutex_unlock(&mem_avail_mtx);
}

/** Add reference to frame.
 *
 * Find respective frame structure for supplied PFN and
 * increment frame reference count.
 *
 * @param pfn Frame number of the frame to be freed.
 *
 */
void frame_reference_add(pfn_t pfn)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	/*
	 * First, find host frame zone for addr.
	 */
	size_t znum = find_zone(pfn, 1, NULL);
	
	ASSERT(znum != (size_t) -1);
	
	zones.info[znum].frames[pfn - zones.info[znum].base].refcount++;
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

/** Mark given range unavailable in frame zones. */
void frame_mark_unavailable(pfn_t start, size_t count)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	size_t i;
	for (i = 0; i < count; i++) {
		size_t znum = find_zone(start + i, 1, 0);
		if (znum == (size_t) -1)  /* PFN not found */
			continue;
		
		zone_mark_unavailable(&zones.info[znum],
		    start + i - zones.info[znum].base);
	}
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

/** Initialize physical memory management. */
void frame_init(void)
{
	if (config.cpu_active == 1) {
		zones.count = 0;
		spinlock_initialize(&zones.lock, "zones.lock");
		mutex_initialize(&mem_avail_mtx, MUTEX_ACTIVE);
		condvar_initialize(&mem_avail_cv);
	}
	
	/* Tell the architecture to create some memory */
	frame_arch_init();
	if (config.cpu_active == 1) {
		frame_mark_unavailable(ADDR2PFN(KA2PA(config.base)),
		    SIZE2FRAMES(config.kernel_size));
		frame_mark_unavailable(ADDR2PFN(KA2PA(config.stack_base)),
		    SIZE2FRAMES(config.stack_size));
		
		size_t i;
		for (i = 0; i < init.cnt; i++) {
			pfn_t pfn = ADDR2PFN(KA2PA(init.tasks[i].addr));
			frame_mark_unavailable(pfn,
			    SIZE2FRAMES(init.tasks[i].size));
		}
		
		if (ballocs.size)
			frame_mark_unavailable(ADDR2PFN(KA2PA(ballocs.base)),
			    SIZE2FRAMES(ballocs.size));
		
		/* Black list first frame, as allocating NULL would
		 * fail in some places
		 */
		frame_mark_unavailable(0, 1);
	}
}

/** Return total size of all zones. */
uint64_t zones_total_size(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	uint64_t total = 0;
	size_t i;
	for (i = 0; i < zones.count; i++)
		total += (uint64_t) FRAMES2SIZE(zones.info[i].count);
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	return total;
}

void zones_stats(uint64_t *total, uint64_t *unavail, uint64_t *busy,
    uint64_t *free)
{
	ASSERT(total != NULL);
	ASSERT(unavail != NULL);
	ASSERT(busy != NULL);
	ASSERT(free != NULL);
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	
	*total = 0;
	*unavail = 0;
	*busy = 0;
	*free = 0;
	
	size_t i;
	for (i = 0; i < zones.count; i++) {
		*total += (uint64_t) FRAMES2SIZE(zones.info[i].count);
		
		if (zone_flags_available(zones.info[i].flags)) {
			*busy += (uint64_t) FRAMES2SIZE(zones.info[i].busy_count);
			*free += (uint64_t) FRAMES2SIZE(zones.info[i].free_count);
		} else
			*unavail += (uint64_t) FRAMES2SIZE(zones.info[i].count);
	}
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

/** Prints list of zones. */
void zones_print_list(void)
{
#ifdef __32_BITS__
	printf("#  base address frames       flags    free frames  busy frames\n");
	printf("-- ------------ ------------ -------- ------------ ------------\n");
#endif

#ifdef __64_BITS__
	printf("#  base address          frames      flags    free frames  busy frames\n");
	printf("-- -------------------- ------------ -------- ------------ ------------\n");
#endif
	
	/*
	 * Because printing may require allocation of memory, we may not hold
	 * the frame allocator locks when printing zone statistics.  Therefore,
	 * we simply gather the statistics under the protection of the locks and
	 * print the statistics when the locks have been released.
	 *
	 * When someone adds/removes zones while we are printing the statistics,
	 * we may end up with inaccurate output (e.g. a zone being skipped from
	 * the listing).
	 */
	
	size_t i;
	for (i = 0;; i++) {
		ipl_t ipl = interrupts_disable();
		spinlock_lock(&zones.lock);
		
		if (i >= zones.count) {
			spinlock_unlock(&zones.lock);
			interrupts_restore(ipl);
			break;
		}
		
		uintptr_t base = PFN2ADDR(zones.info[i].base);
		size_t count = zones.info[i].count;
		zone_flags_t flags = zones.info[i].flags;
		size_t free_count = zones.info[i].free_count;
		size_t busy_count = zones.info[i].busy_count;
		
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		
		bool available = zone_flags_available(flags);
		
		printf("%-2" PRIs, i);
		
#ifdef __32_BITS__
		printf("   %10p", base);
#endif
		
#ifdef __64_BITS__
		printf("   %18p", base);
#endif
		
		printf(" %12" PRIs " %c%c%c      ", count,
		    available ? 'A' : ' ',
		    (flags & ZONE_RESERVED) ? 'R' : ' ',
		    (flags & ZONE_FIRMWARE) ? 'F' : ' ');
		
		if (available)
			printf("%12" PRIs " %12" PRIs,
			    free_count, busy_count);
		
		printf("\n");
	}
}

/** Prints zone details.
 *
 * @param num Zone base address or zone number.
 *
 */
void zone_print_one(size_t num)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	size_t znum = (size_t) -1;
	
	size_t i;
	for (i = 0; i < zones.count; i++) {
		if ((i == num) || (PFN2ADDR(zones.info[i].base) == num)) {
			znum = i;
			break;
		}
	}
	
	if (znum == (size_t) -1) {
		spinlock_unlock(&zones.lock);
		interrupts_restore(ipl);
		printf("Zone not found.\n");
		return;
	}
	
	uintptr_t base = PFN2ADDR(zones.info[i].base);
	zone_flags_t flags = zones.info[i].flags;
	size_t count = zones.info[i].count;
	size_t free_count = zones.info[i].free_count;
	size_t busy_count = zones.info[i].busy_count;
	
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
	
	bool available = zone_flags_available(flags);
	
	printf("Zone number:       %" PRIs "\n", znum);
	printf("Zone base address: %p\n", base);
	printf("Zone size:         %" PRIs " frames (%" PRIs " KiB)\n", count,
	    SIZE2KB(FRAMES2SIZE(count)));
	printf("Zone flags:        %c%c%c\n",
	    available ? 'A' : ' ',
	    (flags & ZONE_RESERVED) ? 'R' : ' ',
	    (flags & ZONE_FIRMWARE) ? 'F' : ' ');
	
	if (available) {
		printf("Allocated space:   %" PRIs " frames (%" PRIs " KiB)\n",
		    busy_count, SIZE2KB(FRAMES2SIZE(busy_count)));
		printf("Available space:   %" PRIs " frames (%" PRIs " KiB)\n",
		    free_count, SIZE2KB(FRAMES2SIZE(free_count)));
	}
}

/** @}
 */
