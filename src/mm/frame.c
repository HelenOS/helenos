/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/types.h>
#include <func.h>

#include <mm/heap.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/vm.h>
#include <arch/mm/page.h>

#include <config.h>
#include <memstr.h>

#include <panic.h>
#include <debug.h>

#include <synch/spinlock.h>

#include <arch/asm.h>
#include <arch.h>

count_t frames = 0;
count_t frames_free;

__u8 *frame_bitmap;
count_t frame_bitmap_octets;

static spinlock_t framelock;

spinlock_t zone_head_lock;       /**< this lock protects zone_head list */
link_t zone_head;                /**< list of all zones in the system */


void frame_init(void)
{
	if (config.cpu_active == 1) {
		zone_init();

		/*
		 * The bootstrap processor will allocate all necessary memory for frame allocation.
		 */

		frames = config.memory_size / FRAME_SIZE;
		frame_bitmap_octets = frames / 8 + (frames % 8 > 0);
		frame_bitmap = (__u8 *) malloc(frame_bitmap_octets);
		if (!frame_bitmap)
			panic("malloc/frame_bitmap\n");

		/*
		 * Mark all frames free.
		 */
		memsetb((__address) frame_bitmap, frame_bitmap_octets, 0);
		frames_free = frames;
	}

	/*
	 * No frame allocations/reservations prior this point.
	 */

	frame_arch_init();

	if (config.cpu_active == 1) {
		/*
		 * Create the memory address space map. Marked frames and frame
		 * regions cannot be used for allocation.
		 */
		frame_region_not_free(config.base, config.base + config.kernel_size);
	}
}

/*
 * Allocate a frame.
 */
__address frame_alloc(int flags)
{
	int i;
	pri_t pri;
	
loop:
	pri = cpu_priority_high();
	spinlock_lock(&framelock);
	if (frames_free) {
		for (i=0; i < frames; i++) {
			int m, n;
		
			m = i / 8;
			n = i % 8;

			if ((frame_bitmap[m] & (1<<n)) == 0) {
				frame_bitmap[m] |= (1<<n);
				frames_free--;
				spinlock_unlock(&framelock);
				cpu_priority_restore(pri);
				if (flags & FRAME_KA) return PA2KA(i*FRAME_SIZE);
				return i*FRAME_SIZE;
			}
		}
		panic("frames_free inconsistent (%d)\n", frames_free);
	}
	spinlock_unlock(&framelock);
	cpu_priority_restore(pri);

	if (flags & FRAME_PANIC)
		panic("unable to allocate frame\n");
		
	/* TODO: implement sleeping logic here */
	panic("sleep not supported\n");
	
	goto loop;
}

/*
 * Free a frame.
 */
void frame_free(__address addr)
{
	pri_t pri;
	__u32 frame;

	pri = cpu_priority_high();
	spinlock_lock(&framelock);
	
	frame = IS_KA(addr) ? KA2PA(addr) : addr;
	frame /= FRAME_SIZE;
	if (frame < frames) {
		int m, n;
	
		m = frame / 8;
		n = frame % 8;
	
		if (frame_bitmap[m] & (1<<n)) {
			frame_bitmap[m] &= ~(1<<n);
			frames_free++;
		}
		else panic("frame already free\n");
	}
	else panic("frame number too big\n");
	
	spinlock_unlock(&framelock);
	cpu_priority_restore(pri);
}

/*
 * Don't use this function for normal allocation. Use frame_alloc() instead.
 * Use this function to declare that some special frame is not free.
 */
void frame_not_free(__address addr)
{
	pri_t pri;
	__u32 frame;
	
	pri = cpu_priority_high();
	spinlock_lock(&framelock);
	frame = IS_KA(addr) ? KA2PA(addr) : addr;
	frame /= FRAME_SIZE;
	if (frame < frames) {
		int m, n;

		m = frame / 8;
		n = frame % 8;
	
		if ((frame_bitmap[m] & (1<<n)) == 0) {	
			frame_bitmap[m] |= (1<<n);
			frames_free--;	
		}
	}
	spinlock_unlock(&framelock);
	cpu_priority_restore(pri);
}

void frame_region_not_free(__address start, __address stop)
{
	__address a;

	start /= FRAME_SIZE;
	stop /= FRAME_SIZE;
	for (a = start; a <= stop; a++)
		frame_not_free(a * FRAME_SIZE);
}

/** Initialize zonekeeping
 *
 * Initialize zonekeeping.
 */
void zone_init(void)
{
	spinlock_initialize(&zone_head_lock);
	list_initialize(&zone_head);
}

/** Create frame zone
 *
 * Create new frame zone.
 *
 * @param start Physical address of the first frame within the zone.
 * @param size Size of the zone. Must be a multiple of FRAME_SIZE.
 * @param flags Zone flags.
 *
 * @return Initialized zone.
 */
zone_t *zone_create(__address start, size_t size, int flags)
{
	zone_t *z;
	count_t cnt;
	int i;
	
	ASSERT(start % FRAME_SIZE == 0);
	ASSERT(size % FRAME_SIZE == 0);
	
	cnt = size / FRAME_SIZE;
	
	z = (zone_t *) malloc(sizeof(zone_t));
	if (z) {
		link_initialize(&z->link);
		spinlock_initialize(&z->lock);
	
		z->base = start;
		z->flags = flags;

		z->free_count = cnt;
		list_initialize(&z->free_head);

		z->busy_count = 0;
		list_initialize(&z->busy_head);
		
		z->frames = (frame_t *) malloc(cnt * sizeof(frame_t));
		if (!z->frames) {
			free(z);
			return NULL;
		}
		
		for (i = 0; i<cnt; i++) {
			frame_initialize(&z->frames[i], z);
			list_append(&z->frames[i].link, &z->free_head);
		}
		
	}
	
	return z;
}

/** Attach frame zone
 *
 * Attach frame zone to zone list.
 *
 * @param zone Zone to be attached.
 */
void zone_attach(zone_t *zone)
{
	pri_t pri;
	
	pri = cpu_priority_high();
	spinlock_lock(&zone_head_lock);
	
	list_append(&zone->link, &zone_head);
	
	spinlock_unlock(&zone_head_lock);
	cpu_priority_restore(pri);
}

/** Initialize frame structure
 *
 * Initialize frame structure.
 *
 * @param frame Frame structure to be initialized.
 * @param zone Host frame zone.
 */
void frame_initialize(frame_t *frame, zone_t *zone)
{
	frame->refcount = 0;
	link_initialize(&frame->link);
	frame->zone = zone;
}

/** Get address of physical frame from its frame structure
 *
 * Get address of physical frame from its frame structure.
 *
 * @param frame Frame structure of the queried frame address.
 *
 * @return Address of frame associated with the argument.
 */
__address frame_get_address(frame_t *frame)
{
	__address v;
	zone_t *z;
	pri_t pri;
	
	z = frame->zone;
	
	pri = cpu_priority_high();
	spinlock_lock(&z->lock);

	v = z->base + (frame - z->frames) * FRAME_SIZE;
	
	if (z->flags & FRAME_KA)
		v = PA2KA(v);

	spinlock_unlock(&z->lock);
	cpu_priority_restore(pri);
	
	return v;
}
