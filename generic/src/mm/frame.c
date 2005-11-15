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

#include <typedefs.h>
#include <arch/types.h>
#include <mm/heap.h>
#include <mm/frame.h>
#include <mm/vm.h>
#include <panic.h>
#include <debug.h>
#include <list.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch.h>

spinlock_t zone_head_lock;       /**< this lock protects zone_head list */
link_t zone_head;                /**< list of all zones in the system */

static struct buddy_system_operations  zone_buddy_system_operations = {
	.find_buddy = zone_buddy_find_buddy,
	.bisect = zone_buddy_bisect,
	.coalesce = zone_buddy_coalesce,
	.set_order = zone_buddy_set_order,
	.get_order = zone_buddy_get_order,
};

/** Initialize physical memory management
 *
 * Initialize physical memory managemnt.
 */
void frame_init(void)
{
	if (config.cpu_active == 1) {
		zone_init();
	}

	frame_arch_init();
	
	if (config.cpu_active == 1) {
                frame_region_not_free(config.base, config.base + config.kernel_size + CONFIG_STACK_SIZE);
        }	
}

/** Allocate a frame
 *
 * Allocate a frame of physical memory.
 *
 * @param flags Flags for host zone selection and address processing.
 *
 * @return Allocated frame.
 */
__address frame_alloc(int flags)
{
	ipl_t ipl;
	link_t *cur, *tmp;
	zone_t *z;
	zone_t *zone = NULL;
	frame_t *frame = NULL;
	__address v;
	
loop:
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	/*
	 * First, find suitable frame zone.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		/*
		 * Check if the zone has any free frames.
		 */
		if (z->free_count) {
			zone = z;
			break;
		}
		spinlock_unlock(&z->lock);
	}
	
	if (!zone) {
		if (flags & FRAME_PANIC)
			panic("Can't allocate frame.\n");
		
		/*
		 * TODO: Sleep until frames are available again.
		 */
		spinlock_unlock(&zone_head_lock);
		interrupts_restore(ipl);

		panic("Sleep not implemented.\n");
		goto loop;
	}
		
	tmp = zone->free_head.next;
	frame = list_get_instance(tmp, frame_t, link);

	frame->refcount++;
	list_remove(tmp);			/* remove frame from free_head */
	zone->free_count--;
	zone->busy_count++;
	
	//v = zone->base + (frame - zone->frames) * FRAME_SIZE;
	v = FRAME2ADDR(zone, frame);
	
	if (flags & FRAME_KA)
		v = PA2KA(v);
	
	spinlock_unlock(&zone->lock);
	
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
	
	return v;
}

/** Free a frame.
 *
 * Find respective frame structrue for supplied addr.
 * Decrement frame reference count.
 * If it drops to zero, move the frame structure to free list.
 *
 * @param addr Address of the frame to be freed. It must be a multiple of FRAME_SIZE.
 */
void frame_free(__address addr)
{
	ipl_t ipl;
	link_t *cur;
	zone_t *z;
	zone_t *zone = NULL;
	frame_t *frame;
	
	ASSERT(addr % FRAME_SIZE == 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	/*
	 * First, find host frame zone for addr.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		
		if (IS_KA(addr))
			addr = KA2PA(addr);
		
		/*
		 * Check if addr belongs to z.
		 */
		if ((addr >= z->base) && (addr <= z->base + (z->free_count + z->busy_count) * FRAME_SIZE)) {
			zone = z;
			break;
		}
		spinlock_unlock(&z->lock);
	}
	
	ASSERT(zone != NULL);
	
	frame = ADDR2FRAME(zone, addr);
	// frame = &zone->frames[(addr - zone->base)/FRAME_SIZE];
	ASSERT(frame->refcount);

	if (!--frame->refcount) {
		list_append(&frame->link, &zone->free_head);	/* append frame to free_head */
		zone->free_count++;
		zone->busy_count--;
	}
	
	spinlock_unlock(&zone->lock);	
	
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
}

/** Mark frame not free.
 *
 * Find respective frame structrue for supplied addr.
 * Increment frame reference count and remove the frame structure from free list.
 *
 * @param addr Address of the frame to be marked. It must be a multiple of FRAME_SIZE.
 */
void frame_not_free(__address addr)
{
	ipl_t ipl;
	link_t *cur;
	zone_t *z;
	zone_t *zone = NULL;
	frame_t *frame;
	
	ASSERT(addr % FRAME_SIZE == 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	/*
	 * First, find host frame zone for addr.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		
		if (IS_KA(addr))
			addr = KA2PA(addr);
		
		/*
		 * Check if addr belongs to z.
		 */
		if ((addr >= z->base) && (addr <= z->base + (z->free_count + z->busy_count) * FRAME_SIZE)) {
			zone = z;
			break;
		}
		spinlock_unlock(&z->lock);
	}
	
	ASSERT(zone != NULL);
	
	//frame = &zone->frames[(addr - zone->base)/FRAME_SIZE];
	frame = ADDR2FRAME(zone, addr);

	if (!frame->refcount) {
		frame->refcount++;

		list_remove(&frame->link);			/* remove frame from free_head */
		zone->free_count--;
		zone->busy_count++;
	}
	
	spinlock_unlock(&zone->lock);	
	
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
}

/** Mark frame region not free.
 *
 * Mark frame region not free.
 *
 * @param start First address.
 * @param stop Last address.
 */
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
	__u8 max_order;
	
	ASSERT(start % FRAME_SIZE == 0);
	ASSERT(size % FRAME_SIZE == 0);
	
	cnt = size / FRAME_SIZE;
	
	z = (zone_t *) early_malloc(sizeof(zone_t));
	if (z) {
		link_initialize(&z->link);
		spinlock_initialize(&z->lock);
	
		z->base = start;
		z->flags = flags;

		z->free_count = cnt;
		list_initialize(&z->free_head);

		z->busy_count = 0;
		
		z->frames = (frame_t *) early_malloc(cnt * sizeof(frame_t));
		if (!z->frames) {
			early_free(z);
			return NULL;
		}
		
		for (i = 0; i<cnt; i++) {
			frame_initialize(&z->frames[i], z);
			list_append(&z->frames[i].link, &z->free_head);
		}
		
		/*
		 * Create buddy system for the zone
		 */
		for (max_order = 0; cnt >> max_order; max_order++);
		z->buddy_system = buddy_system_create(max_order, &zone_buddy_system_operations, (void *) z);
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
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	list_append(&zone->link, &zone_head);
	
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
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
}



/*
 * buddy system functions (under construction)
 * 
 */


/** Allocate 2^order frames
 *
 */
__address zone_buddy_frame_alloc(int flags, __u8 order) {
	ipl_t ipl;
	link_t *cur, *tmp;
	zone_t *z;
	zone_t *zone = NULL;
	frame_t *frame = NULL;
	__address v;
	
loop:
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	/*
	 * First, find suitable frame zone.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		/*
		 * Check if the zone has 2^order frames area available
		 * TODO: Must check if buddy system has at least block in order >= given order
		 */
		if (z->free_count == (1 >> order)) { 
			zone = z;
			break;
		}
		
		spinlock_unlock(&z->lock);
	}
	
	if (!zone) {
		if (flags & FRAME_PANIC)
			panic("Can't allocate frame.\n");
		
		/*
		 * TODO: Sleep until frames are available again.
		 */
		spinlock_unlock(&zone_head_lock);
		interrupts_restore(ipl);

		panic("Sleep not implemented.\n");
		goto loop;
	}
		

	/* Allocate frames from zone buddy system */
	cur = buddy_system_alloc(zone->buddy_system, order);
	
	/* frame will be actually a first frame of the block */
	frame = list_get_instance(cur, frame_t, buddy_link);
	
	/* get frame address */
	v = FRAME2ADDR(zone, frame);
	
	if (flags & FRAME_KA)
		v = PA2KA(v);
	
	spinlock_unlock(&zone->lock);
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
	
	return v;
}


/** Free frame(s)
 *
 * @param addr Address of the frame(s) to be freed. It must be a multiple of FRAME_SIZE.
 */
void zone_buddy_frame_free(__address addr)
{
	ipl_t ipl;
	link_t *cur;
	zone_t *z;
	zone_t *zone = NULL;
	frame_t *frame;
	
	ASSERT(addr % FRAME_SIZE == 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&zone_head_lock);
	
	/*
	 * First, find host frame zone for addr.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		
		if (IS_KA(addr))
			addr = KA2PA(addr);
		
		/*
		 * Check if addr belongs to z.
		 */
		if ((addr >= z->base) && (addr <= z->base + (z->free_count + z->busy_count) * FRAME_SIZE)) {
			zone = z;
			break;
		}
		spinlock_unlock(&z->lock);
	}
	
	ASSERT(zone != NULL);
	
	frame = ADDR2FRAME(zone, addr);

	ASSERT(frame->refcount);

	if (!--frame->refcount) {
		buddy_system_free(zone->buddy_system, &frame->buddy_link);
	}
	
	spinlock_unlock(&zone->lock);	
	
	spinlock_unlock(&zone_head_lock);
	interrupts_restore(ipl);
}

/** Guess zone by frame instance address
 *
 * @param frame Frame
 *
 * @return Zone of given frame
 */
zone_t * get_zone_by_frame(frame_t * frame) {
	link_t * cur;
	zone_t * zone, *z;

	ASSERT(frame);
	/*
	 * First, find host frame zone for addr.
	 */
	for (cur = zone_head.next; cur != &zone_head; cur = cur->next) {
		z = list_get_instance(cur, zone_t, link);
		
		spinlock_lock(&z->lock);
		
		/*
		 * Check if frame address belongs to z.
		 */
		if ((frame >= z->frames) && (frame <= z->frames + (z->free_count + z->busy_count))) {
			zone = z;
			break;
		}
		spinlock_unlock(&z->lock);
	}
	ASSERT(zone);
	
	return zone;


}

/** Buddy system find_buddy implementation
 *
 * @param b Buddy system.
 * @param block Block for which buddy should be found
 *
 * @return Buddy for given block if found
 */
link_t * zone_buddy_find_buddy(buddy_system_t *b, link_t * block) {
	frame_t * frame, * f;
	zone_t * zone;
	link_t * cur;
	bool is_left, is_right;

	frame = list_get_instance(block, frame_t, buddy_link);
	zone = get_zone_by_frame(frame);
	
	
	/* 
	 * (FRAME_INDEX % 2^(ORDER+1)) == 0 ===> LEFT BUDDY 
	 * (FRAME_INDEX % 2^(ORDER+1)) == 2^(ORDER) ===> RIGHT BUDDY
	 */
	 
	is_left = IS_BUDDY_LEFT_BLOCK(zone, frame);
	is_right = IS_BUDDY_RIGHT_BLOCK(zone, frame);
	
	ASSERT((is_left || is_right) && (!is_left || !is_right));
	
	for (cur = &zone->buddy_system->order[frame->buddy_order]; cur; cur = cur->next) {
		f = list_get_instance(cur, frame_t, buddy_link);
		
		ASSERT(f->buddy_order == frame->buddy_order);
		
		/* 
		 * if found frame is coherent with our frame from the left 
		 */
		if ((FRAME_INDEX(zone, f) + 1 >> frame->buddy_order == FRAME_INDEX(zone, frame)) && is_right) {
			return cur;
		}
		
		/* 
		 * if found frame is coherent with our frame from the right 
		 */
		if ((FRAME_INDEX(zone,f) - 1 >> frame->buddy_order == FRAME_INDEX(zone, frame)) && is_left) {
			return cur;
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
link_t * zone_buddy_bisect(buddy_system_t *b, link_t * block) {
	frame_t * frame_l, * frame_r;
	
	frame_l = list_get_instance(block, frame_t, buddy_link);

	frame_r = (frame_t *) (&frame_l + (1>>frame_l->buddy_order-1));

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
link_t * zone_buddy_coalesce(buddy_system_t *b, link_t * block_1, link_t * block_2) {
	frame_t * frame1, * frame2;
	
	frame1 = list_get_instance(block_1, frame_t, buddy_link);
	frame2 = list_get_instance(block_2, frame_t, buddy_link);
	
	return &frame1 < &frame2 ? block_1 : block_2;
}

/** Buddy system set_order implementation
 *
 * @param b Buddy system.
 * @param block Buddy system block
 * @param order Order to set
 */
void zone_buddy_set_order(buddy_system_t *b, link_t * block, __u8 order) {
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
__u8 zone_buddy_get_order(buddy_system_t *b, link_t * block) {
	frame_t * frame;
	frame = list_get_instance(block, frame_t, buddy_link);
	return frame->buddy_order;
}
