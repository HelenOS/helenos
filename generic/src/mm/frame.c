/*
 * Copyright (C) 2001-2005 Jakub Jermar
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

/**
 * @file	frame.c
 * @brief	Physical frame allocator.
 *
 * This file contains the physical frame allocator and memory zone management.
 * The frame allocator is built on top of the buddy allocator.
 *
 * @see buddy.c
 */

/*
 * Locking order
 *
 * In order to access particular zone, the process must first lock
 * the zones.lock, then lock the zone and then unlock the zones.lock.
 * This insures, that we can fiddle with the zones in runtime without
 * affecting the processes. 
 *
 */

#include <typedefs.h>
#include <arch/types.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <panic.h>
#include <debug.h>
#include <adt/list.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch.h>
#include <print.h>
#include <align.h>
#include <mm/slab.h>
#include <bitops.h>
#include <macros.h>

typedef struct {
	count_t refcount;	/**< tracking of shared frames  */
	__u8 buddy_order;	/**< buddy system block order */
	link_t buddy_link;	/**< link to the next free block inside one order */
	void *parent;           /**< If allocated by slab, this points there */
} frame_t;

typedef struct {
	SPINLOCK_DECLARE(lock);	/**< this lock protects everything below */
	pfn_t base;		/**< frame_no of the first frame in the frames array */
	count_t count;          /**< Size of zone */

	frame_t *frames;	/**< array of frame_t structures in this zone */
	count_t free_count;	/**< number of free frame_t structures */
	count_t busy_count;	/**< number of busy frame_t structures */
	
	buddy_system_t * buddy_system; /**< buddy system for the zone */
	int flags;
} zone_t;

/*
 * The zoneinfo.lock must be locked when accessing zoneinfo structure.
 * Some of the attributes in zone_t structures are 'read-only'
 */

struct {
	SPINLOCK_DECLARE(lock);
	int count;
	zone_t *info[ZONES_MAX];
} zones;


/*********************************/
/* Helper functions */
static inline index_t frame_index(zone_t *zone, frame_t *frame)
{
	return (index_t)(frame - zone->frames);
}
static inline index_t frame_index_abs(zone_t *zone, frame_t *frame)
{
	return (index_t)(frame - zone->frames) + zone->base;
}
static inline int frame_index_valid(zone_t *zone, index_t index)
{
	return index >= 0 && index < zone->count;
}

/** Compute pfn_t from frame_t pointer & zone pointer */
static index_t make_frame_index(zone_t *zone, frame_t *frame)
{
	return frame - zone->frames;
}

/** Initialize frame structure
 *
 * Initialize frame structure.
 *
 * @param frame Frame structure to be initialized.
 */
static void frame_initialize(frame_t *frame)
{
	frame->refcount = 1;
	frame->buddy_order = 0;
}

/*************************************/
/* Zoneinfo functions */

/**
 * Insert-sort zone into zones list
 *
 * @param newzone New zone to be inserted into zone list
 * @return zone number on success, -1 on error
 */
static int zones_add_zone(zone_t *newzone)
{
	int i,j;
	ipl_t ipl;
	zone_t *z;

	ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	/* Try to merge */
	if (zones.count + 1 == ZONES_MAX)
		panic("Maximum zone(%d) count exceeded.", ZONES_MAX);
	for (i = 0; i < zones.count; i++) {
		/* Check for overflow */
		z = zones.info[i];
		if (overlaps(newzone->base,newzone->count,
			     z->base, z->count)) {
			printf("Zones overlap!\n");
			return -1;
		}
		if (newzone->base < z->base)
			break;
	}
	/* Move other zones up */
	for (j = i;j < zones.count; j++)
		zones.info[j + 1] = zones.info[j];
	zones.info[i] = newzone;
	zones.count++;
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);

	return i;
}

/**
 * Try to find a zone where can we find the frame
 *
 * @param frame Frame number contained in zone
 * @param pzone If not null, it is used as zone hint. Zone index
 *              is filled into the variable on success. 
 * @return Pointer to LOCKED zone containing frame
 *
 * Assume interrupts disable
 */
static zone_t * find_zone_and_lock(pfn_t frame, int *pzone)
{
	int i;
	int hint = pzone ? *pzone : 0;
	zone_t *z;
	
	spinlock_lock(&zones.lock);

	if (hint >= zones.count || hint < 0)
		hint = 0;
	
	i = hint;
	do {
		z = zones.info[i];
		spinlock_lock(&z->lock);
		if (z->base <= frame && z->base + z->count > frame) {
			spinlock_unlock(&zones.lock); /* Unlock the global lock */
			if (pzone)
				*pzone = i;
			return z;
		}
		spinlock_unlock(&z->lock);

		i++;
		if (i >= zones.count)
			i = 0;
	} while(i != hint);

	spinlock_unlock(&zones.lock);
	return NULL;
}

/** @return True if zone can allocate specified order */
static int zone_can_alloc(zone_t *z, __u8 order)
{
	return buddy_system_can_alloc(z->buddy_system, order);
}

/**
 * Find AND LOCK zone that can allocate order frames
 *
 * Assume interrupts are disabled!!
 *
 * @param order Size (2^order) of free space we are trying to find
 * @param pzone Pointer to preferred zone or NULL, on return contains zone number
 */
static zone_t * find_free_zone_lock(__u8 order, int *pzone)
{
	int i;
	zone_t *z;
	int hint = pzone ? *pzone : 0;
	
	spinlock_lock(&zones.lock);
	if (hint >= zones.count)
		hint = 0;
	i = hint;
	do {
		z = zones.info[i];
		
		spinlock_lock(&z->lock);

		/* Check if the zone has 2^order frames area available  */
		if (zone_can_alloc(z, order)) {
			spinlock_unlock(&zones.lock);
			if (pzone)
				*pzone = i;
			return z;
		}
		spinlock_unlock(&z->lock);
		if (++i >= zones.count)
			i = 0;
	} while(i != hint);
	spinlock_unlock(&zones.lock);
	return NULL;
}

/********************************************/
/* Buddy system functions */

/** Buddy system find_block implementation
 *
 * Find block that is parent of current list.
 * That means go to lower addresses, until such block is found
 *
 * @param order - Order of parent must be different then this parameter!!
 */
static link_t *zone_buddy_find_block(buddy_system_t *b, link_t *child,
				     __u8 order)
{
	frame_t * frame;
	zone_t * zone;
	index_t index;
	
	frame = list_get_instance(child, frame_t, buddy_link);
	zone = (zone_t *) b->data;

	index = frame_index(zone, frame);
	do {
		if (zone->frames[index].buddy_order != order) {
			return &zone->frames[index].buddy_link;
		}
	} while(index-- > 0);
	return NULL;
}

static void zone_buddy_print_id(buddy_system_t *b, link_t *block)
{
	frame_t * frame;
	zone_t * zone;
	index_t index;

	frame = list_get_instance(block, frame_t, buddy_link);
	zone = (zone_t *) b->data;
	index = frame_index(zone, frame);
	printf("%zd", index);
}				     

/** Buddy system find_buddy implementation
 *
 * @param b Buddy system.
 * @param block Block for which buddy should be found
 *
 * @return Buddy for given block if found
 */
static link_t * zone_buddy_find_buddy(buddy_system_t *b, link_t * block) 
{
	frame_t * frame;
	zone_t * zone;
	index_t index;
	bool is_left, is_right;

	frame = list_get_instance(block, frame_t, buddy_link);
	zone = (zone_t *) b->data;
	ASSERT(IS_BUDDY_ORDER_OK(frame_index_abs(zone, frame), frame->buddy_order));
	
	is_left = IS_BUDDY_LEFT_BLOCK_ABS(zone, frame);
	is_right = IS_BUDDY_RIGHT_BLOCK_ABS(zone, frame);

	ASSERT(is_left ^ is_right);
	if (is_left) {
		index = (frame_index(zone, frame)) + (1 << frame->buddy_order);
	} else { // if (is_right)
		index = (frame_index(zone, frame)) - (1 << frame->buddy_order);
	}
	
	if (frame_index_valid(zone, index)) {
		if (zone->frames[index].buddy_order == frame->buddy_order && 
		    zone->frames[index].refcount == 0) {
			return &zone->frames[index].buddy_link;
		}
	}

	return NULL;	
}

/** Buddy system bisect implementation
 *
 * @param b Buddy system.
 * @param block Block to bisect
 *
 * @return right block
 */
static link_t * zone_buddy_bisect(buddy_system_t *b, link_t * block) {
	frame_t * frame_l, * frame_r;

	frame_l = list_get_instance(block, frame_t, buddy_link);
	frame_r = (frame_l + (1 << (frame_l->buddy_order - 1)));
	
	return &frame_r->buddy_link;
}

/** Buddy system coalesce implementation
 *
 * @param b Buddy system.
 * @param block_1 First block
 * @param block_2 First block's buddy
 *
 * @return Coalesced block (actually block that represents lower address)
 */
static link_t * zone_buddy_coalesce(buddy_system_t *b, link_t * block_1, 
				    link_t * block_2) 
{
	frame_t *frame1, *frame2;
	
	frame1 = list_get_instance(block_1, frame_t, buddy_link);
	frame2 = list_get_instance(block_2, frame_t, buddy_link);
	
	return frame1 < frame2 ? block_1 : block_2;
}

/** Buddy system set_order implementation
 *
 * @param b Buddy system.
 * @param block Buddy system block
 * @param order Order to set
 */
static void zone_buddy_set_order(buddy_system_t *b, link_t * block, __u8 order) {
	frame_t * frame;
	frame = list_get_instance(block, frame_t, buddy_link);
	frame->buddy_order = order;
}

/** Buddy system get_order implementation
 *
 * @param b Buddy system.
 * @param block Buddy system block
 *
 * @return Order of block
 */
static __u8 zone_buddy_get_order(buddy_system_t *b, link_t * block) {
	frame_t * frame;
	frame = list_get_instance(block, frame_t, buddy_link);
	return frame->buddy_order;
}

/** Buddy system mark_busy implementation
 *
 * @param b Buddy system
 * @param block Buddy system block
 *
 */
static void zone_buddy_mark_busy(buddy_system_t *b, link_t * block) {
	frame_t * frame;

	frame = list_get_instance(block, frame_t, buddy_link);
	frame->refcount = 1;
}

/** Buddy system mark_available implementation
 *
 * @param b Buddy system
 * @param block Buddy system block
 *
 */
static void zone_buddy_mark_available(buddy_system_t *b, link_t * block) {
	frame_t * frame;
	frame = list_get_instance(block, frame_t, buddy_link);
	frame->refcount = 0;
}

static struct buddy_system_operations  zone_buddy_system_operations = {
	.find_buddy = zone_buddy_find_buddy,
	.bisect = zone_buddy_bisect,
	.coalesce = zone_buddy_coalesce,
	.set_order = zone_buddy_set_order,
	.get_order = zone_buddy_get_order,
	.mark_busy = zone_buddy_mark_busy,
	.mark_available = zone_buddy_mark_available,
	.find_block = zone_buddy_find_block,
	.print_id = zone_buddy_print_id
};

/*************************************/
/* Zone functions */

/** Allocate frame in particular zone
 *
 * Assume zone is locked
 * Panics if allocation is impossible.
 *
 * @param zone  Zone to allocate from.
 * @param order Allocate exactly 2^order frames.
 *
 * @return Frame index in zone
 *
 */
static pfn_t zone_frame_alloc(zone_t *zone, __u8 order)
{
	pfn_t v;
	link_t *tmp;
	frame_t *frame;

	/* Allocate frames from zone buddy system */
	tmp = buddy_system_alloc(zone->buddy_system, order);
	
	ASSERT(tmp);
	
	/* Update zone information. */
	zone->free_count -= (1 << order);
	zone->busy_count += (1 << order);

	/* Frame will be actually a first frame of the block. */
	frame = list_get_instance(tmp, frame_t, buddy_link);
	
	/* get frame address */
	v = make_frame_index(zone, frame);
	return v;
}

/** Free frame from zone
 *
 * Assume zone is locked
 *
 * @param zone Pointer to zone from which the frame is to be freed
 * @param frame_idx Frame index relative to zone
 */
static void zone_frame_free(zone_t *zone, index_t frame_idx)
{
	frame_t *frame;
	__u8 order;

	frame = &zone->frames[frame_idx];
	
	/* remember frame order */
	order = frame->buddy_order;

	ASSERT(frame->refcount);

	if (!--frame->refcount) {
		buddy_system_free(zone->buddy_system, &frame->buddy_link);
	
		/* Update zone information. */
		zone->free_count += (1 << order);
		zone->busy_count -= (1 << order);
	}
}

/** Return frame from zone */
static frame_t * zone_get_frame(zone_t *zone, index_t frame_idx)
{
	ASSERT(frame_idx < zone->count);
	return &zone->frames[frame_idx];
}

/** Mark frame in zone unavailable to allocation */
static void zone_mark_unavailable(zone_t *zone, index_t frame_idx)
{
	frame_t *frame;
	link_t *link;

	frame = zone_get_frame(zone, frame_idx);
	if (frame->refcount)
		return;
	link = buddy_system_alloc_block(zone->buddy_system, 
					&frame->buddy_link);
	ASSERT(link);
	zone->free_count--;
}

/** 
 * Join 2 zones
 *
 * Expect zone_t *z to point to space at least zone_conf_size large
 *
 * Assume z1 & z2 are locked 
 *
 * @param z Target zone structure pointer
 * @param z1 Zone to merge
 * @param z2 Zone to merge
 */

static void _zone_merge(zone_t *z, zone_t *z1, zone_t *z2)
{
	__u8 max_order;
	int i, z2idx;
	pfn_t frame_idx;
	frame_t *frame;

	ASSERT(!overlaps(z1->base,z1->count,z2->base,z2->count));
	ASSERT(z1->base < z2->base);

	spinlock_initialize(&z->lock, "zone_lock");
	z->base = z1->base;
	z->count = z2->base+z2->count - z1->base;
	z->flags = z1->flags & z2->flags;

	z->free_count = z1->free_count + z2->free_count;
	z->busy_count = z1->busy_count + z2->busy_count;
	
	max_order = fnzb(z->count);

	z->buddy_system = (buddy_system_t *)&z[1];
	buddy_system_create(z->buddy_system, max_order, 
			    &zone_buddy_system_operations, 
			    (void *) z);

	z->frames = (frame_t *)((void *)z->buddy_system+buddy_conf_size(max_order));
	for (i = 0; i < z->count; i++) {
		/* This marks all frames busy */
		frame_initialize(&z->frames[i]);
	}
	/* Copy frames from both zones to preserve full frame orders,
	 * parents etc. Set all free frames with refcount=0 to 1, because
	 * we add all free frames to buddy allocator later again, clear
	 * order to 0. Don't set busy frames with refcount=0, as they
	 * will not be reallocated during merge and it would make later
	 * problems with allocation/free.
	 */
	for (i=0; i<z1->count; i++)
		z->frames[i] = z1->frames[i];
	for (i=0; i < z2->count; i++) {
		z2idx = i + (z2->base - z1->base);
		z->frames[z2idx] = z2->frames[i];
	}
	i = 0;
	while (i < z->count) {
		if (z->frames[i].refcount) {
			/* skip busy frames */
			i += 1 << z->frames[i].buddy_order;
		} else { /* Free frames, set refcount=1 */
			/* All free frames have refcount=0, we need not
			 * to check the order */
			z->frames[i].refcount = 1;
			z->frames[i].buddy_order = 0;
			i++;
		}
	}
	/* Add free blocks from the 2 original zones */
	while (zone_can_alloc(z1, 0)) {
		frame_idx = zone_frame_alloc(z1, 0);
		frame = &z->frames[frame_idx];
		frame->refcount = 0;
		buddy_system_free(z->buddy_system, &frame->buddy_link);
	}
	while (zone_can_alloc(z2, 0)) {
		frame_idx = zone_frame_alloc(z2, 0);
		frame = &z->frames[frame_idx + (z2->base-z1->base)];
		frame->refcount = 0;
		buddy_system_free(z->buddy_system, &frame->buddy_link);
	}
}

/** Return old configuration frames into the zone
 *
 * We have several cases
 * - the conf. data is outside of zone -> exit, shall we call frame_free??
 * - the conf. data was created by zone_create or 
 *   updated with reduce_region -> free every frame
 *
 * @param newzone The actual zone where freeing should occur
 * @param oldzone Pointer to old zone configuration data that should
 *                be freed from new zone
 */
static void return_config_frames(zone_t *newzone, zone_t *oldzone)
{
	pfn_t pfn;
	frame_t *frame;
	count_t cframes;
	int i;

	pfn = ADDR2PFN((__address)KA2PA(oldzone));
	cframes = SIZE2FRAMES(zone_conf_size(oldzone->count));
	
	if (pfn < newzone->base || pfn >= newzone->base + newzone->count)
		return;

	frame = &newzone->frames[pfn - newzone->base];
	ASSERT(!frame->buddy_order);

	for (i=0; i < cframes; i++) {
		newzone->busy_count++;
		zone_frame_free(newzone, pfn+i-newzone->base);
	}
}

/** Reduce allocated block to count of order 0 frames
 *
 * The allocated block need 2^order frames of space. Reduce all frames
 * in block to order 0 and free the unneeded frames. This means, that 
 * when freeing the previously allocated block starting with frame_idx, 
 * you have to free every frame.
 *
 * @param zone 
 * @param frame_idx Index to block 
 * @param count Allocated space in block
 */
static void zone_reduce_region(zone_t *zone, pfn_t frame_idx, count_t count)
{
	count_t i;
	__u8 order;
	frame_t *frame;
	
	ASSERT(frame_idx+count < zone->count);

	order = zone->frames[frame_idx].buddy_order;
	ASSERT((1 << order) >= count);

	/* Reduce all blocks to order 0 */
	for (i=0; i < (1 << order); i++) {
		frame = &zone->frames[i + frame_idx];
		frame->buddy_order = 0;
		if (! frame->refcount)
			frame->refcount = 1;
		ASSERT(frame->refcount == 1);
	}
	/* Free unneeded frames */
	for (i=count; i < (1 << order); i++) {
		zone_frame_free(zone, i + frame_idx);
	}
}

/** Merge zones z1 and z2
 *
 * - the zones must be 2 zones with no zone existing in between,
 *   which means that z2 = z1+1
 *
 * - When you create a new zone, the frame allocator configuration does
 *   not to be 2^order size. Once the allocator is running it is no longer
 *   possible, merged configuration data occupies more space :-/
 */
void zone_merge(int z1, int z2)
{
	ipl_t ipl;
	zone_t *zone1, *zone2, *newzone;
	int cframes;
	__u8 order;
	int i;
	pfn_t pfn;

	ipl = interrupts_disable();
	spinlock_lock(&zones.lock);

	if (z1 < 0 || z1 >= zones.count || z2 < 0 || z2 >= zones.count)
		goto errout;
	/* We can join only 2 zones with none existing inbetween */
	if (z2-z1 != 1)
		goto errout;

	zone1 = zones.info[z1];
	zone2 = zones.info[z2];
	spinlock_lock(&zone1->lock);
	spinlock_lock(&zone2->lock);

	cframes = SIZE2FRAMES(zone_conf_size(zone2->base+zone2->count-zone1->base));
	order = fnzb(cframes) + 1;

	/* Allocate zonedata inside one of the zones */
	if (zone_can_alloc(zone1, order))
		pfn = zone1->base + zone_frame_alloc(zone1, order);
	else if (zone_can_alloc(zone2, order))
		pfn = zone2->base + zone_frame_alloc(zone2, order);
	else
		goto errout2;

	newzone = (zone_t *)PA2KA(PFN2ADDR(pfn));

	_zone_merge(newzone, zone1, zone2);

	/* Free unneeded config frames */
	zone_reduce_region(newzone, pfn - newzone->base,  cframes);
	/* Subtract zone information from busy frames */
	newzone->busy_count -= cframes;

	/* Replace existing zones in zoneinfo list */
	zones.info[z1] = newzone;
	for (i = z2 + 1; i < zones.count; i++)
		zones.info[i - 1] = zones.info[i];
	zones.count--;

	/* Free old zone information */
	return_config_frames(newzone, zone1);
	return_config_frames(newzone, zone2);
errout2:
	/* Nobody is allowed to enter to zone, so we are safe
	 * to touch the spinlocks last time */
	spinlock_unlock(&zone1->lock);
	spinlock_unlock(&zone2->lock);
errout:
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

/**
 * Merge all zones into one big zone
 *
 * It is reasonable to do this on systems whose bios reports parts in chunks,
 * so that we could have 1 zone (it's faster).
 */
void zone_merge_all(void)
{
	int count = zones.count;

	while (zones.count > 1 && --count) {
		zone_merge(0,1);
		break;
	}
}

/** Create frame zone
 *
 * Create new frame zone.
 *
 * @param start Physical address of the first frame within the zone.
 * @param count Count of frames in zone
 * @param z Address of configuration information of zone
 * @param flags Zone flags.
 *
 * @return Initialized zone.
 */
static void zone_construct(pfn_t start, count_t count, zone_t *z, int flags)
{
	int i;
	__u8 max_order;

	spinlock_initialize(&z->lock, "zone_lock");
	z->base = start;
	z->count = count;
	z->flags = flags;
	z->free_count = count;
	z->busy_count = 0;

	/*
	 * Compute order for buddy system, initialize
	 */
	max_order = fnzb(count);
	z->buddy_system = (buddy_system_t *)&z[1];
	
	buddy_system_create(z->buddy_system, max_order, 
			    &zone_buddy_system_operations, 
			    (void *) z);
	
	/* Allocate frames _after_ the conframe */
	/* Check sizes */
	z->frames = (frame_t *)((void *)z->buddy_system+buddy_conf_size(max_order));
	for (i = 0; i<count; i++) {
		frame_initialize(&z->frames[i]);
	}
	
	/* Stuffing frames */
	for (i = 0; i < count; i++) {
		z->frames[i].refcount = 0;
		buddy_system_free(z->buddy_system, &z->frames[i].buddy_link);
	}
}

/** Compute configuration data size for zone
 *
 * @param count Size of zone in frames
 * @return Size of zone configuration info (in bytes)
 */
__address zone_conf_size(count_t count)
{
	int size = sizeof(zone_t) + count*sizeof(frame_t);
	int max_order;

	max_order = fnzb(count);
	size += buddy_conf_size(max_order);
	return size;
}

/** Create and add zone to system
 *
 * @param start First frame number (absolute)
 * @param count Size of zone in frames
 * @param confframe Where configuration frames are supposed to be.
 *                  Automatically checks, that we will not disturb the 
 *                  kernel and possibly init. 
 *                  If confframe is given _outside_ this zone, it is expected,
 *                  that the area is already marked BUSY and big enough
 *                  to contain zone_conf_size() amount of data.
 *                  If the confframe is inside the area, the zone free frame
 *                  information is modified not to include it.
 *
 * @return Zone number or -1 on error
 */
int zone_create(pfn_t start, count_t count, pfn_t confframe, int flags)
{
	zone_t *z;
	__address addr;
	count_t confcount;
	int i;
	int znum;

	/* Theoretically we could have here 0, practically make sure
	 * nobody tries to do that. If some platform requires, remove
	 * the assert
	 */
	ASSERT(confframe);
	/* If conframe is supposed to be inside our zone, then make sure
	 * it does not span kernel & init
	 */
	confcount = SIZE2FRAMES(zone_conf_size(count));
	if (confframe >= start && confframe < start+count) {
		for (;confframe < start + count; confframe++) {
			addr = PFN2ADDR(confframe);
			if (overlaps(addr, PFN2ADDR(confcount), KA2PA(config.base), config.kernel_size))
				continue;
			
			bool overlap = false;
			count_t i;
			for (i = 0; i < init.cnt; i++)
				if (overlaps(addr, PFN2ADDR(confcount), KA2PA(init.tasks[i].addr), init.tasks[i].size)) {
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

	z = (zone_t *)PA2KA(PFN2ADDR(confframe));
	zone_construct(start, count, z, flags);
	znum = zones_add_zone(z);
	if (znum == -1)
		return -1;

	/* If confdata in zone, mark as unavailable */
	if (confframe >= start && confframe < start+count)
		for (i=confframe; i<confframe+confcount; i++) {
			zone_mark_unavailable(z, i - z->base);
		}
	return znum;
}

/***************************************/
/* Frame functions */

/** Set parent of frame */
void frame_set_parent(pfn_t pfn, void *data, int hint)
{
	zone_t *zone = find_zone_and_lock(pfn, &hint);

	ASSERT(zone);

	zone_get_frame(zone, pfn-zone->base)->parent = data;
	spinlock_unlock(&zone->lock);
}

void * frame_get_parent(pfn_t pfn, int hint)
{
	zone_t *zone = find_zone_and_lock(pfn, &hint);
	void *res;

	ASSERT(zone);
	res = zone_get_frame(zone, pfn - zone->base)->parent;
	
	spinlock_unlock(&zone->lock);
	return res;
}

/** Allocate power-of-two frames of physical memory.
 *
 * @param order  Allocate exactly 2^order frames.
 * @param flags  Flags for host zone selection and address processing.
 * @param status Allocation status (FRAME_OK on success), unused if NULL.
 * @param pzone  Preferred zone
 *
 * @return Allocated frame.
 *
 */
pfn_t frame_alloc_generic(__u8 order, int flags, int *status, int *pzone) 
{
	ipl_t ipl;
	int freed;
	pfn_t v;
	zone_t *zone;
	
loop:
	ipl = interrupts_disable();
	
	/*
	 * First, find suitable frame zone.
	 */
	zone = find_free_zone_lock(order, pzone);
	
	/* If no memory, reclaim some slab memory,
	   if it does not help, reclaim all */
	if (!zone && !(flags & FRAME_NO_RECLAIM)) {
		freed = slab_reclaim(0);
		if (freed)
			zone = find_free_zone_lock(order, pzone);
		if (!zone) {
			freed = slab_reclaim(SLAB_RECLAIM_ALL);
			if (freed)
				zone = find_free_zone_lock(order, pzone);
		}
	}
	if (!zone) {
		if (flags & FRAME_PANIC)
			panic("Can't allocate frame.\n");
		
		/*
		 * TODO: Sleep until frames are available again.
		 */
		interrupts_restore(ipl);

		if (flags & FRAME_ATOMIC) {
			ASSERT(status != NULL);
			if (status)
				*status = FRAME_NO_MEMORY;
			return NULL;
		}
		
		panic("Sleep not implemented.\n");
		goto loop;
	}
	
	v = zone_frame_alloc(zone, order);
	v += zone->base;

	spinlock_unlock(&zone->lock);
	interrupts_restore(ipl);

	if (status)
		*status = FRAME_OK;
	return v;
}

/** Free a frame.
 *
 * Find respective frame structure for supplied PFN.
 * Decrement frame reference count.
 * If it drops to zero, move the frame structure to free list.
 *
 * @param frame Frame number to be freed.
 */
void frame_free(pfn_t pfn)
{
	ipl_t ipl;
	zone_t *zone;

	ipl = interrupts_disable();
	
	/*
	 * First, find host frame zone for addr.
	 */
	zone = find_zone_and_lock(pfn,NULL);
	ASSERT(zone);
	
	zone_frame_free(zone, pfn-zone->base);
	
	spinlock_unlock(&zone->lock);
	interrupts_restore(ipl);
}

/** Add reference to frame.
 *
 * Find respective frame structure for supplied PFN and
 * increment frame reference count.
 *
 * @param frame Frame no to be freed.
 */
void frame_reference_add(pfn_t pfn)
{
	ipl_t ipl;
	zone_t *zone;
	frame_t *frame;

	ipl = interrupts_disable();
	
	/*
	 * First, find host frame zone for addr.
	 */
	zone = find_zone_and_lock(pfn,NULL);
	ASSERT(zone);
	
	frame = &zone->frames[pfn-zone->base];
	frame->refcount++;
	
	spinlock_unlock(&zone->lock);
	interrupts_restore(ipl);
}

/** Mark given range unavailable in frame zones */
void frame_mark_unavailable(pfn_t start, count_t count)
{
	int i;
	zone_t *zone;
	int prefzone = 0;
	
	for (i=0; i < count; i++) {
		zone = find_zone_and_lock(start+i,&prefzone);
		if (!zone) /* PFN not found */
			continue;
		zone_mark_unavailable(zone, start+i-zone->base);

		spinlock_unlock(&zone->lock);
	}
}

/** Initialize physical memory management
 *
 * Initialize physical memory managemnt.
 */
void frame_init(void)
{
	if (config.cpu_active == 1) {
		zones.count = 0;
		spinlock_initialize(&zones.lock,"zones_glob_lock");
	}
	/* Tell the architecture to create some memory */
	frame_arch_init();
	if (config.cpu_active == 1) {
		pfn_t firstframe = ADDR2PFN(KA2PA(config.base));
		pfn_t lastframe = ADDR2PFN(KA2PA(config.base+config.kernel_size));
		frame_mark_unavailable(firstframe,lastframe-firstframe+1);
		
		count_t i;
		for (i = 0; i < init.cnt; i++)
			frame_mark_unavailable(ADDR2PFN(KA2PA(init.tasks[i].addr)), SIZE2FRAMES(init.tasks[i].size));
	}
}



/** Prints list of zones
 *
 */
void zone_print_list(void) {
	zone_t *zone = NULL;
	int i;
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&zones.lock);
	printf("#  Base address\tFree Frames\tBusy Frames\n");
	printf("   ------------\t-----------\t-----------\n");
	for (i = 0; i < zones.count; i++) {
		zone = zones.info[i];
		spinlock_lock(&zone->lock);
		printf("%d: %.*p \t%10zd\t%10zd\n", i, sizeof(__address) * 2, PFN2ADDR(zone->base), zone->free_count, zone->busy_count);
		spinlock_unlock(&zone->lock);
	}
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

/** Prints zone details
 *
 * @param base Zone base address OR zone number
 */
void zone_print_one(int num) {
	zone_t *zone = NULL;
	ipl_t ipl;
	int i;

	ipl = interrupts_disable();
	spinlock_lock(&zones.lock);

	for (i = 0; i < zones.count; i++) {
		if (i == num || PFN2ADDR(zones.info[i]->base) == num) {
			zone = zones.info[i];
			break;
		}
	}
	if (!zone) {
		printf("Zone not found.\n");
		goto out;
	}
	
	spinlock_lock(&zone->lock);
	printf("Memory zone information\n");
	printf("Zone base address: %#.*p\n", sizeof(__address) * 2, PFN2ADDR(zone->base));
	printf("Zone size: %zd frames (%zdK)\n", zone->count, ((zone->count) * FRAME_SIZE) >> 10);
	printf("Allocated space: %zd frames (%zdK)\n", zone->busy_count, (zone->busy_count * FRAME_SIZE) >> 10);
	printf("Available space: %zd (%zdK)\n", zone->free_count, (zone->free_count * FRAME_SIZE) >> 10);
	buddy_system_structure_print(zone->buddy_system, FRAME_SIZE);
	
	spinlock_unlock(&zone->lock);
out:
	spinlock_unlock(&zones.lock);
	interrupts_restore(ipl);
}

