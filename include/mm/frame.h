/*
 * Copyright (C) 2005 Jakub Jermar
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

#define FRAME_KA	1	/* skip frames conflicting with user address space */
#define FRAME_PANIC	2	/* panic on failure */

struct frame_zone {
	link_t fz_link;		/**< link to previous and next frame_zone */

	spinlock_t lock;	/**< this lock protexts everything below */
	link_t free_head;	/**< list of free frames */
	link_t busy_head;	/**< list of busy frames */
	count_t free_count;	/**< frames in free list */
	count_t busy_count;	/**< frames in busy list */
	frame_t *frames;	/**< array of frames in this zone */
	int flags;
};

struct frame {
	count_t refcount;	/**< when > 0, the frame is in busy list, otherwise the frame is in free list */
	link_t link;		/**< link either to frame_zone free or busy list */
};

extern spinlock_t frame_zone_head_lock;		/**< this lock protects frame_zone_head list */
extern link_t frame_zone_head;			/**< list of all frame_zone's in the system */

extern count_t frames;
extern count_t frames_free;

extern count_t kernel_frames;
extern count_t kernel_frames_free;

extern __u8 *frame_bitmap;
extern count_t frame_bitmap_octets;

extern __u8 *frame_kernel_bitmap;

extern void frame_init(void);

__address frame_alloc(int flags);
extern void frame_free(__address addr);
extern void frame_not_free(__address addr);
extern void frame_region_not_free(__address start, __address stop);

#endif
