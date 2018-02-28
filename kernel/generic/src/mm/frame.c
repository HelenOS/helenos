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
 * The frame allocator is built on top of the two-level bitmap structure.
 *
 */

#include <typedefs.h>
#include <mm/frame.h>
#include <mm/reserve.h>
#include <mm/as.h>
#include <panic.h>
#include <assert.h>
#include <adt/list.h>
#include <synch/mutex.h>
#include <synch/condvar.h>
#include <arch/asm.h>
#include <arch.h>
#include <print.h>
#include <log.h>
#include <align.h>
#include <mm/slab.h>
#include <bitops.h>
#include <macros.h>
#include <config.h>
#include <str.h>
#include <proc/thread.h> /* THREAD */

zones_t zones;

/*
 * Synchronization primitives used to sleep when there is no memory
 * available.
 */
static mutex_t mem_avail_mtx;
static condvar_t mem_avail_cv;
static size_t mem_avail_req = 0;  /**< Number of frames requested. */
static size_t mem_avail_gen = 0;  /**< Generation counter. */

/** Initialize frame structure.
 *
 * @param frame Frame structure to be initialized.
 *
 */
NO_TRACE static void frame_initialize(frame_t *frame)
{
	frame->refcount = 0;
	frame->parent = NULL;
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
NO_TRACE static size_t zones_insert_zone(pfn_t base, size_t count,
    zone_flags_t flags)
{
	if (zones.count + 1 == ZONES_MAX) {
		log(LF_OTHER, LVL_ERROR, "Maximum zone count %u exceeded!",
		    ZONES_MAX);
		return (size_t) -1;
	}

	size_t i;
	for (i = 0; i < zones.count; i++) {
		/* Check for overlap */
		if (overlaps(zones.info[i].base, zones.info[i].count,
		    base, count)) {

			/*
			 * If the overlaping zones are of the same type
			 * and the new zone is completely within the previous
			 * one, then quietly ignore the new zone.
			 *
			 */

			if ((zones.info[i].flags != flags) ||
			    (!iswithin(zones.info[i].base, zones.info[i].count,
			    base, count))) {
				log(LF_OTHER, LVL_WARN,
				    "Zone (%p, %p) overlaps "
				    "with previous zone (%p %p)!",
				    (void *) PFN2ADDR(base), (void *) PFN2ADDR(count),
				    (void *) PFN2ADDR(zones.info[i].base),
				    (void *) PFN2ADDR(zones.info[i].count));
			}

			return (size_t) -1;
		}
		if (base < zones.info[i].base)
			break;
	}

	/* Move other zones up */
	for (size_t j = zones.count; j > i; j--)
		zones.info[j] = zones.info[j - 1];

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
NO_TRACE static size_t frame_total_free_get_internal(void)
{
	size_t total = 0;
	size_t i;

	for (i = 0; i < zones.count; i++)
		total += zones.info[i].free_count;

	return total;
}

NO_TRACE size_t frame_total_free_get(void)
{
	size_t total;

	irq_spinlock_lock(&zones.lock, true);
	total = frame_total_free_get_internal();
	irq_spinlock_unlock(&zones.lock, true);

	return total;
}


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
NO_TRACE size_t find_zone(pfn_t frame, size_t count, size_t hint)
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

/** @return True if zone can allocate specified number of frames */
NO_TRACE static bool zone_can_alloc(zone_t *zone, size_t count,
    pfn_t constraint)
{
	/*
	 * The function bitmap_allocate_range() does not modify
	 * the bitmap if the last argument is NULL.
	 */

	return ((zone->flags & ZONE_AVAILABLE) &&
	    bitmap_allocate_range(&zone->bitmap, count, zone->base,
	    FRAME_LOWPRIO, constraint, NULL));
}

/** Find a zone that can allocate specified number of frames
 *
 * This function searches among all zones. Assume interrupts are
 * disabled and zones lock is locked.
 *
 * @param count      Number of free frames we are trying to find.
 * @param flags      Required flags of the zone.
 * @param constraint Indication of bits that cannot be set in the
 *                   physical frame number of the first allocated frame.
 * @param hint       Preferred zone.
 *
 * @return Zone that can allocate specified number of frames.
 * @return -1 if no zone can satisfy the request.
 *
 */
NO_TRACE static size_t find_free_zone_all(size_t count, zone_flags_t flags,
    pfn_t constraint, size_t hint)
{
	for (size_t pos = 0; pos < zones.count; pos++) {
		size_t i = (pos + hint) % zones.count;

		/* Check whether the zone meets the search criteria. */
		if (!ZONE_FLAGS_MATCH(zones.info[i].flags, flags))
			continue;

		/* Check if the zone can satisfy the allocation request. */
		if (zone_can_alloc(&zones.info[i], count, constraint))
			return i;
	}

	return (size_t) -1;
}

/** Check if frame range  priority memory
 *
 * @param pfn   Starting frame.
 * @param count Number of frames.
 *
 * @return True if the range contains only priority memory.
 *
 */
NO_TRACE static bool is_high_priority(pfn_t base, size_t count)
{
	return (base + count <= FRAME_LOWPRIO);
}

/** Find a zone that can allocate specified number of frames
 *
 * This function ignores zones that contain only high-priority
 * memory. Assume interrupts are disabled and zones lock is locked.
 *
 * @param count      Number of free frames we are trying to find.
 * @param flags      Required flags of the zone.
 * @param constraint Indication of bits that cannot be set in the
 *                   physical frame number of the first allocated frame.
 * @param hint       Preferred zone.
 *
 * @return Zone that can allocate specified number of frames.
 * @return -1 if no low-priority zone can satisfy the request.
 *
 */
NO_TRACE static size_t find_free_zone_lowprio(size_t count, zone_flags_t flags,
    pfn_t constraint, size_t hint)
{
	for (size_t pos = 0; pos < zones.count; pos++) {
		size_t i = (pos + hint) % zones.count;

		/* Skip zones containing only high-priority memory. */
		if (is_high_priority(zones.info[i].base, zones.info[i].count))
			continue;

		/* Check whether the zone meets the search criteria. */
		if (!ZONE_FLAGS_MATCH(zones.info[i].flags, flags))
			continue;

		/* Check if the zone can satisfy the allocation request. */
		if (zone_can_alloc(&zones.info[i], count, constraint))
			return i;
	}

	return (size_t) -1;
}

/** Find a zone that can allocate specified number of frames
 *
 * Assume interrupts are disabled and zones lock is
 * locked.
 *
 * @param count      Number of free frames we are trying to find.
 * @param flags      Required flags of the target zone.
 * @param constraint Indication of bits that cannot be set in the
 *                   physical frame number of the first allocated frame.
 * @param hint       Preferred zone.
 *
 * @return Zone that can allocate specified number of frames.
 * @return -1 if no zone can satisfy the request.
 *
 */
NO_TRACE static size_t find_free_zone(size_t count, zone_flags_t flags,
    pfn_t constraint, size_t hint)
{
	if (hint >= zones.count)
		hint = 0;

	/*
	 * Prefer zones with low-priority memory over
	 * zones with high-priority memory.
	 */

	size_t znum = find_free_zone_lowprio(count, flags, constraint, hint);
	if (znum != (size_t) -1)
		return znum;

	/* Take all zones into account */
	return find_free_zone_all(count, flags, constraint, hint);
}

/******************/
/* Zone functions */
/******************/

/** Return frame from zone. */
NO_TRACE static frame_t *zone_get_frame(zone_t *zone, size_t index)
{
	assert(index < zone->count);

	return &zone->frames[index];
}

/** Allocate frame in particular zone.
 *
 * Assume zone is locked and is available for allocation.
 * Panics if allocation is impossible.
 *
 * @param zone       Zone to allocate from.
 * @param count      Number of frames to allocate
 * @param constraint Indication of bits that cannot be set in the
 *                   physical frame number of the first allocated frame.
 *
 * @return Frame index in zone.
 *
 */
NO_TRACE static size_t zone_frame_alloc(zone_t *zone, size_t count,
    pfn_t constraint)
{
	assert(zone->flags & ZONE_AVAILABLE);

	/* Allocate frames from zone */
	size_t index = (size_t) -1;
	int avail = bitmap_allocate_range(&zone->bitmap, count, zone->base,
	    FRAME_LOWPRIO, constraint, &index);

	assert(avail);
	assert(index != (size_t) -1);

	/* Update frame reference count */
	for (size_t i = 0; i < count; i++) {
		frame_t *frame = zone_get_frame(zone, index + i);

		assert(frame->refcount == 0);
		frame->refcount = 1;
	}

	/* Update zone information. */
	zone->free_count -= count;
	zone->busy_count += count;

	return index;
}

/** Free frame from zone.
 *
 * Assume zone is locked and is available for deallocation.
 *
 * @param zone  Pointer to zone from which the frame is to be freed.
 * @param index Frame index relative to zone.
 *
 * @return Number of freed frames.
 *
 */
NO_TRACE static size_t zone_frame_free(zone_t *zone, size_t index)
{
	assert(zone->flags & ZONE_AVAILABLE);

	frame_t *frame = zone_get_frame(zone, index);

	assert(frame->refcount > 0);

	if (!--frame->refcount) {
		bitmap_set(&zone->bitmap, index, 0);

		/* Update zone information. */
		zone->free_count++;
		zone->busy_count--;

		return 1;
	}

	return 0;
}

/** Mark frame in zone unavailable to allocation. */
NO_TRACE static void zone_mark_unavailable(zone_t *zone, size_t index)
{
	assert(zone->flags & ZONE_AVAILABLE);

	frame_t *frame = zone_get_frame(zone, index);
	if (frame->refcount > 0)
		return;

	frame->refcount = 1;
	bitmap_set_range(&zone->bitmap, index, 1);

	zone->free_count--;
	reserve_force_alloc(1);
}

/** Merge two zones.
 *
 * Assume z1 & z2 are locked and compatible and zones lock is
 * locked.
 *
 * @param z1       First zone to merge.
 * @param z2       Second zone to merge.
 * @param old_z1   Original data of the first zone.
 * @param confdata Merged zone configuration data.
 *
 */
NO_TRACE static void zone_merge_internal(size_t z1, size_t z2, zone_t *old_z1,
    void *confdata)
{
	assert(zones.info[z1].flags & ZONE_AVAILABLE);
	assert(zones.info[z2].flags & ZONE_AVAILABLE);
	assert(zones.info[z1].flags == zones.info[z2].flags);
	assert(zones.info[z1].base < zones.info[z2].base);
	assert(!overlaps(zones.info[z1].base, zones.info[z1].count,
	    zones.info[z2].base, zones.info[z2].count));

	/* Difference between zone bases */
	pfn_t base_diff = zones.info[z2].base - zones.info[z1].base;

	zones.info[z1].count = base_diff + zones.info[z2].count;
	zones.info[z1].free_count += zones.info[z2].free_count;
	zones.info[z1].busy_count += zones.info[z2].busy_count;

	bitmap_initialize(&zones.info[z1].bitmap, zones.info[z1].count,
	    confdata + (sizeof(frame_t) * zones.info[z1].count));
	bitmap_clear_range(&zones.info[z1].bitmap, 0, zones.info[z1].count);

	zones.info[z1].frames = (frame_t *) confdata;

	/*
	 * Copy frames and bits from both zones to preserve parents, etc.
	 */

	for (size_t i = 0; i < old_z1->count; i++) {
		bitmap_set(&zones.info[z1].bitmap, i,
		    bitmap_get(&old_z1->bitmap, i));
		zones.info[z1].frames[i] = old_z1->frames[i];
	}

	for (size_t i = 0; i < zones.info[z2].count; i++) {
		bitmap_set(&zones.info[z1].bitmap, base_diff + i,
		    bitmap_get(&zones.info[z2].bitmap, i));
		zones.info[z1].frames[base_diff + i] =
		    zones.info[z2].frames[i];
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
NO_TRACE static void return_config_frames(size_t znum, pfn_t pfn, size_t count)
{
	assert(zones.info[znum].flags & ZONE_AVAILABLE);

	size_t cframes = SIZE2FRAMES(zone_conf_size(count));

	if ((pfn < zones.info[znum].base) ||
	    (pfn >= zones.info[znum].base + zones.info[znum].count))
		return;

	for (size_t i = 0; i < cframes; i++)
		(void) zone_frame_free(&zones.info[znum],
		    pfn - zones.info[znum].base + i);
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
 */
bool zone_merge(size_t z1, size_t z2)
{
	irq_spinlock_lock(&zones.lock, true);

	bool ret = true;

	/*
	 * We can join only 2 zones with none existing inbetween,
	 * the zones have to be available and with the same
	 * set of flags
	 */
	if ((z1 >= zones.count) || (z2 >= zones.count) || (z2 - z1 != 1) ||
	    (zones.info[z1].flags != zones.info[z2].flags)) {
		ret = false;
		goto errout;
	}

	pfn_t cframes = SIZE2FRAMES(zone_conf_size(
	    zones.info[z2].base - zones.info[z1].base
	    + zones.info[z2].count));

	/* Allocate merged zone data inside one of the zones */
	pfn_t pfn;
	if (zone_can_alloc(&zones.info[z1], cframes, 0)) {
		pfn = zones.info[z1].base +
		    zone_frame_alloc(&zones.info[z1], cframes, 0);
	} else if (zone_can_alloc(&zones.info[z2], cframes, 0)) {
		pfn = zones.info[z2].base +
		    zone_frame_alloc(&zones.info[z2], cframes, 0);
	} else {
		ret = false;
		goto errout;
	}

	/* Preserve original data from z1 */
	zone_t old_z1 = zones.info[z1];

	/* Do zone merging */
	zone_merge_internal(z1, z2, &old_z1, (void *) PA2KA(PFN2ADDR(pfn)));

	/* Subtract zone information from busy frames */
	zones.info[z1].busy_count -= cframes;

	/* Free old zone information */
	return_config_frames(z1,
	    ADDR2PFN(KA2PA((uintptr_t) old_z1.frames)), old_z1.count);
	return_config_frames(z1,
	    ADDR2PFN(KA2PA((uintptr_t) zones.info[z2].frames)),
	    zones.info[z2].count);

	/* Move zones down */
	for (size_t i = z2 + 1; i < zones.count; i++)
		zones.info[i - 1] = zones.info[i];

	zones.count--;

errout:
	irq_spinlock_unlock(&zones.lock, true);

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
	size_t i = 1;

	while (i < zones.count) {
		if (!zone_merge(i - 1, i))
			i++;
	}
}

/** Create new frame zone.
 *
 * @param zone     Zone to construct.
 * @param start    Physical address of the first frame within the zone.
 * @param count    Count of frames in zone.
 * @param flags    Zone flags.
 * @param confdata Configuration data of the zone.
 *
 * @return Initialized zone.
 *
 */
NO_TRACE static void zone_construct(zone_t *zone, pfn_t start, size_t count,
    zone_flags_t flags, void *confdata)
{
	zone->base = start;
	zone->count = count;
	zone->flags = flags;
	zone->free_count = count;
	zone->busy_count = 0;

	if (flags & ZONE_AVAILABLE) {
		/*
		 * Initialize frame bitmap (located after the array of
		 * frame_t structures in the configuration space).
		 */

		bitmap_initialize(&zone->bitmap, count, confdata +
		    (sizeof(frame_t) * count));
		bitmap_clear_range(&zone->bitmap, 0, count);

		/*
		 * Initialize the array of frame_t structures.
		 */

		zone->frames = (frame_t *) confdata;

		for (size_t i = 0; i < count; i++)
			frame_initialize(&zone->frames[i]);
	} else {
		bitmap_initialize(&zone->bitmap, 0, NULL);
		zone->frames = NULL;
	}
}

/** Compute configuration data size for zone.
 *
 * @param count Size of zone in frames.
 *
 * @return Size of zone configuration info (in bytes).
 *
 */
size_t zone_conf_size(size_t count)
{
	return (count * sizeof(frame_t) + bitmap_size(count));
}

/** Allocate external configuration frames from low memory. */
pfn_t zone_external_conf_alloc(size_t count)
{
	size_t frames = SIZE2FRAMES(zone_conf_size(count));

	return ADDR2PFN((uintptr_t)
	    frame_alloc(frames, FRAME_LOWMEM | FRAME_ATOMIC, 0));
}

/** Create and add zone to system.
 *
 * @param start     First frame number (absolute).
 * @param count     Size of zone in frames.
 * @param confframe Where configuration frames are supposed to be.
 *                  Automatically checks that we will not disturb the
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
size_t zone_create(pfn_t start, size_t count, pfn_t confframe,
    zone_flags_t flags)
{
	irq_spinlock_lock(&zones.lock, true);

	if (flags & ZONE_AVAILABLE) {  /* Create available zone */
		/*
		 * Theoretically we could have NULL here, practically make sure
		 * nobody tries to do that. If some platform requires, remove
		 * the assert
		 */
		assert(confframe != ADDR2PFN((uintptr_t ) NULL));

		/* Update the known end of physical memory. */
		config.physmem_end = max(config.physmem_end, PFN2ADDR(start + count));

		/*
		 * If confframe is supposed to be inside our zone, then make sure
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
				for (size_t i = 0; i < init.cnt; i++) {
					if (overlaps(addr, PFN2ADDR(confcount),
					    init.tasks[i].paddr,
					    init.tasks[i].size)) {
						overlap = true;
						break;
					}
				}

				if (overlap)
					continue;

				break;
			}

			if (confframe >= start + count)
				panic("Cannot find configuration data for zone.");
		}

		size_t znum = zones_insert_zone(start, count, flags);
		if (znum == (size_t) -1) {
			irq_spinlock_unlock(&zones.lock, true);
			return (size_t) -1;
		}

		void *confdata = (void *) PA2KA(PFN2ADDR(confframe));
		zone_construct(&zones.info[znum], start, count, flags, confdata);

		/* If confdata in zone, mark as unavailable */
		if ((confframe >= start) && (confframe < start + count)) {
			for (size_t i = confframe; i < confframe + confcount; i++)
				zone_mark_unavailable(&zones.info[znum],
				    i - zones.info[znum].base);
		}

		irq_spinlock_unlock(&zones.lock, true);

		return znum;
	}

	/* Non-available zone */
	size_t znum = zones_insert_zone(start, count, flags);
	if (znum == (size_t) -1) {
		irq_spinlock_unlock(&zones.lock, true);
		return (size_t) -1;
	}

	zone_construct(&zones.info[znum], start, count, flags, NULL);

	irq_spinlock_unlock(&zones.lock, true);

	return znum;
}

/*******************/
/* Frame functions */
/*******************/

/** Set parent of frame. */
void frame_set_parent(pfn_t pfn, void *data, size_t hint)
{
	irq_spinlock_lock(&zones.lock, true);

	size_t znum = find_zone(pfn, 1, hint);

	assert(znum != (size_t) -1);

	zone_get_frame(&zones.info[znum],
	    pfn - zones.info[znum].base)->parent = data;

	irq_spinlock_unlock(&zones.lock, true);
}

void *frame_get_parent(pfn_t pfn, size_t hint)
{
	irq_spinlock_lock(&zones.lock, true);

	size_t znum = find_zone(pfn, 1, hint);

	assert(znum != (size_t) -1);

	void *res = zone_get_frame(&zones.info[znum],
	    pfn - zones.info[znum].base)->parent;

	irq_spinlock_unlock(&zones.lock, true);

	return res;
}

/** Allocate frames of physical memory.
 *
 * @param count      Number of continuous frames to allocate.
 * @param flags      Flags for host zone selection and address processing.
 * @param constraint Indication of physical address bits that cannot be
 *                   set in the address of the first allocated frame.
 * @param pzone      Preferred zone.
 *
 * @return Physical address of the allocated frame.
 *
 */
uintptr_t frame_alloc_generic(size_t count, frame_flags_t flags,
    uintptr_t constraint, size_t *pzone)
{
	assert(count > 0);

	size_t hint = pzone ? (*pzone) : 0;
	pfn_t frame_constraint = ADDR2PFN(constraint);

	/*
	 * If not told otherwise, we must first reserve the memory.
	 */
	if (!(flags & FRAME_NO_RESERVE))
		reserve_force_alloc(count);

loop:
	irq_spinlock_lock(&zones.lock, true);

	/*
	 * First, find suitable frame zone.
	 */
	size_t znum = find_free_zone(count, FRAME_TO_ZONE_FLAGS(flags),
	    frame_constraint, hint);

	/*
	 * If no memory, reclaim some slab memory,
	 * if it does not help, reclaim all.
	 */
	if ((znum == (size_t) -1) && (!(flags & FRAME_NO_RECLAIM))) {
		irq_spinlock_unlock(&zones.lock, true);
		size_t freed = slab_reclaim(0);
		irq_spinlock_lock(&zones.lock, true);

		if (freed > 0)
			znum = find_free_zone(count, FRAME_TO_ZONE_FLAGS(flags),
			    frame_constraint, hint);

		if (znum == (size_t) -1) {
			irq_spinlock_unlock(&zones.lock, true);
			freed = slab_reclaim(SLAB_RECLAIM_ALL);
			irq_spinlock_lock(&zones.lock, true);

			if (freed > 0)
				znum = find_free_zone(count, FRAME_TO_ZONE_FLAGS(flags),
				    frame_constraint, hint);
		}
	}

	if (znum == (size_t) -1) {
		if (flags & FRAME_ATOMIC) {
			irq_spinlock_unlock(&zones.lock, true);

			if (!(flags & FRAME_NO_RESERVE))
				reserve_free(count);

			return 0;
		}

		size_t avail = frame_total_free_get_internal();

		irq_spinlock_unlock(&zones.lock, true);

		if (!THREAD)
			panic("Cannot wait for %zu frames to become available "
			    "(%zu available).", count, avail);

		/*
		 * Sleep until some frames are available again.
		 */

#ifdef CONFIG_DEBUG
		log(LF_OTHER, LVL_DEBUG,
		    "Thread %" PRIu64 " waiting for %zu frames "
		    "%zu available.", THREAD->tid, count, avail);
#endif

		/*
		 * Since the mem_avail_mtx is an active mutex, we need to
		 * disable interrupts to prevent deadlock with TLB shootdown.
		 */
		ipl_t ipl = interrupts_disable();
		mutex_lock(&mem_avail_mtx);

		if (mem_avail_req > 0)
			mem_avail_req = min(mem_avail_req, count);
		else
			mem_avail_req = count;

		size_t gen = mem_avail_gen;

		while (gen == mem_avail_gen)
			condvar_wait(&mem_avail_cv, &mem_avail_mtx);

		mutex_unlock(&mem_avail_mtx);
		interrupts_restore(ipl);

#ifdef CONFIG_DEBUG
		log(LF_OTHER, LVL_DEBUG, "Thread %" PRIu64 " woken up.",
		    THREAD->tid);
#endif

		goto loop;
	}

	pfn_t pfn = zone_frame_alloc(&zones.info[znum], count,
	    frame_constraint) + zones.info[znum].base;

	irq_spinlock_unlock(&zones.lock, true);

	if (pzone)
		*pzone = znum;

	return PFN2ADDR(pfn);
}

uintptr_t frame_alloc(size_t count, frame_flags_t flags, uintptr_t constraint)
{
	return frame_alloc_generic(count, flags, constraint, NULL);
}

/** Free frames of physical memory.
 *
 * Find respective frame structures for supplied physical frames.
 * Decrement each frame reference count. If it drops to zero, mark
 * the frames as available.
 *
 * @param start Physical Address of the first frame to be freed.
 * @param count Number of frames to free.
 * @param flags Flags to control memory reservation.
 *
 */
void frame_free_generic(uintptr_t start, size_t count, frame_flags_t flags)
{
	size_t freed = 0;

	irq_spinlock_lock(&zones.lock, true);

	for (size_t i = 0; i < count; i++) {
		/*
		 * First, find host frame zone for addr.
		 */
		pfn_t pfn = ADDR2PFN(start) + i;
		size_t znum = find_zone(pfn, 1, 0);

		assert(znum != (size_t) -1);

		freed += zone_frame_free(&zones.info[znum],
		    pfn - zones.info[znum].base);
	}

	irq_spinlock_unlock(&zones.lock, true);

	/*
	 * Signal that some memory has been freed.
	 * Since the mem_avail_mtx is an active mutex,
	 * we need to disable interruptsto prevent deadlock
	 * with TLB shootdown.
	 */

	ipl_t ipl = interrupts_disable();
	mutex_lock(&mem_avail_mtx);

	if (mem_avail_req > 0)
		mem_avail_req -= min(mem_avail_req, freed);

	if (mem_avail_req == 0) {
		mem_avail_gen++;
		condvar_broadcast(&mem_avail_cv);
	}

	mutex_unlock(&mem_avail_mtx);
	interrupts_restore(ipl);

	if (!(flags & FRAME_NO_RESERVE))
		reserve_free(freed);
}

void frame_free(uintptr_t frame, size_t count)
{
	frame_free_generic(frame, count, 0);
}

void frame_free_noreserve(uintptr_t frame, size_t count)
{
	frame_free_generic(frame, count, FRAME_NO_RESERVE);
}

/** Add reference to frame.
 *
 * Find respective frame structure for supplied PFN and
 * increment frame reference count.
 *
 * @param pfn Frame number of the frame to be freed.
 *
 */
NO_TRACE void frame_reference_add(pfn_t pfn)
{
	irq_spinlock_lock(&zones.lock, true);

	/*
	 * First, find host frame zone for addr.
	 */
	size_t znum = find_zone(pfn, 1, 0);

	assert(znum != (size_t) -1);

	zones.info[znum].frames[pfn - zones.info[znum].base].refcount++;

	irq_spinlock_unlock(&zones.lock, true);
}

/** Mark given range unavailable in frame zones.
 *
 */
NO_TRACE void frame_mark_unavailable(pfn_t start, size_t count)
{
	irq_spinlock_lock(&zones.lock, true);

	for (size_t i = 0; i < count; i++) {
		size_t znum = find_zone(start + i, 1, 0);

		if (znum == (size_t) -1)  /* PFN not found */
			continue;

		zone_mark_unavailable(&zones.info[znum],
		    start + i - zones.info[znum].base);
	}

	irq_spinlock_unlock(&zones.lock, true);
}

/** Initialize physical memory management.
 *
 */
void frame_init(void)
{
	if (config.cpu_active == 1) {
		zones.count = 0;
		irq_spinlock_initialize(&zones.lock, "frame.zones.lock");
		mutex_initialize(&mem_avail_mtx, MUTEX_ACTIVE);
		condvar_initialize(&mem_avail_cv);
	}

	/* Tell the architecture to create some memory */
	frame_low_arch_init();

	if (config.cpu_active == 1) {
		frame_mark_unavailable(ADDR2PFN(KA2PA(config.base)),
		    SIZE2FRAMES(config.kernel_size));
		frame_mark_unavailable(ADDR2PFN(KA2PA(config.stack_base)),
		    SIZE2FRAMES(config.stack_size));

		for (size_t i = 0; i < init.cnt; i++)
			frame_mark_unavailable(ADDR2PFN(init.tasks[i].paddr),
			    SIZE2FRAMES(init.tasks[i].size));

		if (ballocs.size)
			frame_mark_unavailable(ADDR2PFN(KA2PA(ballocs.base)),
			    SIZE2FRAMES(ballocs.size));

		/*
		 * Blacklist first frame, as allocating NULL would
		 * fail in some places
		 */
		frame_mark_unavailable(0, 1);
	}

	frame_high_arch_init();
}

/** Adjust bounds of physical memory region according to low/high memory split.
 *
 * @param low[in]      If true, the adjustment is performed to make the region
 *                     fit in the low memory. Otherwise the adjustment is
 *                     performed to make the region fit in the high memory.
 * @param basep[inout] Pointer to a variable which contains the region's base
 *                     address and which may receive the adjusted base address.
 * @param sizep[inout] Pointer to a variable which contains the region's size
 *                     and which may receive the adjusted size.
 *
 * @return True if the region still exists even after the adjustment.
 * @return False otherwise.
 *
 */
bool frame_adjust_zone_bounds(bool low, uintptr_t *basep, size_t *sizep)
{
	uintptr_t limit = KA2PA(config.identity_base) + config.identity_size;

	if (low) {
		if (*basep > limit)
			return false;

		if (*basep + *sizep > limit)
			*sizep = limit - *basep;
	} else {
		if (*basep + *sizep <= limit)
			return false;

		if (*basep <= limit) {
			*sizep -= limit - *basep;
			*basep = limit;
		}
	}

	return true;
}

/** Return total size of all zones.
 *
 */
uint64_t zones_total_size(void)
{
	irq_spinlock_lock(&zones.lock, true);

	uint64_t total = 0;

	for (size_t i = 0; i < zones.count; i++)
		total += (uint64_t) FRAMES2SIZE(zones.info[i].count);

	irq_spinlock_unlock(&zones.lock, true);

	return total;
}

void zones_stats(uint64_t *total, uint64_t *unavail, uint64_t *busy,
    uint64_t *free)
{
	assert(total != NULL);
	assert(unavail != NULL);
	assert(busy != NULL);
	assert(free != NULL);

	irq_spinlock_lock(&zones.lock, true);

	*total = 0;
	*unavail = 0;
	*busy = 0;
	*free = 0;

	for (size_t i = 0; i < zones.count; i++) {
		*total += (uint64_t) FRAMES2SIZE(zones.info[i].count);

		if (zones.info[i].flags & ZONE_AVAILABLE) {
			*busy += (uint64_t) FRAMES2SIZE(zones.info[i].busy_count);
			*free += (uint64_t) FRAMES2SIZE(zones.info[i].free_count);
		} else
			*unavail += (uint64_t) FRAMES2SIZE(zones.info[i].count);
	}

	irq_spinlock_unlock(&zones.lock, true);
}

/** Prints list of zones.
 *
 */
void zones_print_list(void)
{
#ifdef __32_BITS__
	printf("[nr] [base addr] [frames    ] [flags ] [free frames ] [busy frames ]\n");
#endif

#ifdef __64_BITS__
	printf("[nr] [base address    ] [frames    ] [flags ] [free frames ] [busy frames ]\n");
#endif

	/*
	 * Because printing may require allocation of memory, we may not hold
	 * the frame allocator locks when printing zone statistics. Therefore,
	 * we simply gather the statistics under the protection of the locks and
	 * print the statistics when the locks have been released.
	 *
	 * When someone adds/removes zones while we are printing the statistics,
	 * we may end up with inaccurate output (e.g. a zone being skipped from
	 * the listing).
	 */

	size_t free_lowmem = 0;
	size_t free_highmem = 0;
	size_t free_highprio = 0;

	for (size_t i = 0;; i++) {
		irq_spinlock_lock(&zones.lock, true);

		if (i >= zones.count) {
			irq_spinlock_unlock(&zones.lock, true);
			break;
		}

		pfn_t fbase = zones.info[i].base;
		uintptr_t base = PFN2ADDR(fbase);
		size_t count = zones.info[i].count;
		zone_flags_t flags = zones.info[i].flags;
		size_t free_count = zones.info[i].free_count;
		size_t busy_count = zones.info[i].busy_count;

		bool available = ((flags & ZONE_AVAILABLE) != 0);
		bool lowmem = ((flags & ZONE_LOWMEM) != 0);
		bool highmem = ((flags & ZONE_HIGHMEM) != 0);
		bool highprio = is_high_priority(fbase, count);

		if (available) {
			if (lowmem)
				free_lowmem += free_count;

			if (highmem)
				free_highmem += free_count;

			if (highprio) {
				free_highprio += free_count;
			} else {
				/*
				 * Walk all frames of the zone and examine
				 * all high priority memory to get accurate
				 * statistics.
				 */

				for (size_t index = 0; index < count; index++) {
					if (is_high_priority(fbase + index, 0)) {
						if (!bitmap_get(&zones.info[i].bitmap, index))
							free_highprio++;
					} else
						break;
				}
			}
		}

		irq_spinlock_unlock(&zones.lock, true);

		printf("%-4zu", i);

#ifdef __32_BITS__
		printf("  %p", (void *) base);
#endif

#ifdef __64_BITS__
		printf(" %p", (void *) base);
#endif

		printf(" %12zu %c%c%c%c%c    ", count,
		    available ? 'A' : '-',
		    (flags & ZONE_RESERVED) ? 'R' : '-',
		    (flags & ZONE_FIRMWARE) ? 'F' : '-',
		    (flags & ZONE_LOWMEM) ? 'L' : '-',
		    (flags & ZONE_HIGHMEM) ? 'H' : '-');

		if (available)
			printf("%14zu %14zu",
			    free_count, busy_count);

		printf("\n");
	}

	printf("\n");

	uint64_t size;
	const char *size_suffix;

	bin_order_suffix(FRAMES2SIZE(free_lowmem), &size, &size_suffix,
	    false);
	printf("Available low memory:    %zu frames (%" PRIu64 " %s)\n",
	    free_lowmem, size, size_suffix);

	bin_order_suffix(FRAMES2SIZE(free_highmem), &size, &size_suffix,
	    false);
	printf("Available high memory:   %zu frames (%" PRIu64 " %s)\n",
	    free_highmem, size, size_suffix);

	bin_order_suffix(FRAMES2SIZE(free_highprio), &size, &size_suffix,
	    false);
	printf("Available high priority: %zu frames (%" PRIu64 " %s)\n",
	    free_highprio, size, size_suffix);
}

/** Prints zone details.
 *
 * @param num Zone base address or zone number.
 *
 */
void zone_print_one(size_t num)
{
	irq_spinlock_lock(&zones.lock, true);
	size_t znum = (size_t) -1;

	for (size_t i = 0; i < zones.count; i++) {
		if ((i == num) || (PFN2ADDR(zones.info[i].base) == num)) {
			znum = i;
			break;
		}
	}

	if (znum == (size_t) -1) {
		irq_spinlock_unlock(&zones.lock, true);
		printf("Zone not found.\n");
		return;
	}

	size_t free_lowmem = 0;
	size_t free_highmem = 0;
	size_t free_highprio = 0;

	pfn_t fbase = zones.info[znum].base;
	uintptr_t base = PFN2ADDR(fbase);
	zone_flags_t flags = zones.info[znum].flags;
	size_t count = zones.info[znum].count;
	size_t free_count = zones.info[znum].free_count;
	size_t busy_count = zones.info[znum].busy_count;

	bool available = ((flags & ZONE_AVAILABLE) != 0);
	bool lowmem = ((flags & ZONE_LOWMEM) != 0);
	bool highmem = ((flags & ZONE_HIGHMEM) != 0);
	bool highprio = is_high_priority(fbase, count);

	if (available) {
		if (lowmem)
			free_lowmem = free_count;

		if (highmem)
			free_highmem = free_count;

		if (highprio) {
			free_highprio = free_count;
		} else {
			/*
			 * Walk all frames of the zone and examine
			 * all high priority memory to get accurate
			 * statistics.
			 */

			for (size_t index = 0; index < count; index++) {
				if (is_high_priority(fbase + index, 0)) {
					if (!bitmap_get(&zones.info[znum].bitmap, index))
						free_highprio++;
				} else
					break;
			}
		}
	}

	irq_spinlock_unlock(&zones.lock, true);

	uint64_t size;
	const char *size_suffix;

	bin_order_suffix(FRAMES2SIZE(count), &size, &size_suffix, false);

	printf("Zone number:             %zu\n", znum);
	printf("Zone base address:       %p\n", (void *) base);
	printf("Zone size:               %zu frames (%" PRIu64 " %s)\n", count,
	    size, size_suffix);
	printf("Zone flags:              %c%c%c%c%c\n",
	    available ? 'A' : '-',
	    (flags & ZONE_RESERVED) ? 'R' : '-',
	    (flags & ZONE_FIRMWARE) ? 'F' : '-',
	    (flags & ZONE_LOWMEM) ? 'L' : '-',
	    (flags & ZONE_HIGHMEM) ? 'H' : '-');

	if (available) {
		bin_order_suffix(FRAMES2SIZE(busy_count), &size, &size_suffix,
		    false);
		printf("Allocated space:         %zu frames (%" PRIu64 " %s)\n",
		    busy_count, size, size_suffix);

		bin_order_suffix(FRAMES2SIZE(free_count), &size, &size_suffix,
		    false);
		printf("Available space:         %zu frames (%" PRIu64 " %s)\n",
		    free_count, size, size_suffix);

		bin_order_suffix(FRAMES2SIZE(free_lowmem), &size, &size_suffix,
		    false);
		printf("Available low memory:    %zu frames (%" PRIu64 " %s)\n",
		    free_lowmem, size, size_suffix);

		bin_order_suffix(FRAMES2SIZE(free_highmem), &size, &size_suffix,
		    false);
		printf("Available high memory:   %zu frames (%" PRIu64 " %s)\n",
		    free_highmem, size, size_suffix);

		bin_order_suffix(FRAMES2SIZE(free_highprio), &size, &size_suffix,
		    false);
		printf("Available high priority: %zu frames (%" PRIu64 " %s)\n",
		    free_highprio, size, size_suffix);
	}
}

/** @}
 */
