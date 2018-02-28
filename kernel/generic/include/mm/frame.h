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
#include <trace.h>
#include <adt/bitmap.h>
#include <adt/list.h>
#include <synch/spinlock.h>
#include <arch/mm/page.h>
#include <arch/mm/frame.h>

/** Maximum number of zones in the system. */
#define ZONES_MAX  32

typedef uint8_t frame_flags_t;

#define FRAME_NONE        0x00
/** Do not panic and do not sleep on failure. */
#define FRAME_ATOMIC      0x01
/** Do not start reclaiming when no free memory. */
#define FRAME_NO_RECLAIM  0x02
/** Do not reserve / unreserve memory. */
#define FRAME_NO_RESERVE  0x04
/** Allocate a frame which can be identity-mapped. */
#define FRAME_LOWMEM      0x08
/** Allocate a frame which cannot be identity-mapped. */
#define FRAME_HIGHMEM     0x10

typedef uint8_t zone_flags_t;

#define ZONE_NONE       0x00
/** Available zone (free for allocation) */
#define ZONE_AVAILABLE  0x01
/** Zone is reserved (not available for allocation) */
#define ZONE_RESERVED   0x02
/** Zone is used by firmware (not available for allocation) */
#define ZONE_FIRMWARE   0x04
/** Zone contains memory that can be identity-mapped */
#define ZONE_LOWMEM     0x08
/** Zone contains memory that cannot be identity-mapped */
#define ZONE_HIGHMEM    0x10

/** Mask of zone bits that must be matched exactly. */
#define ZONE_EF_MASK  0x07

#define FRAME_TO_ZONE_FLAGS(ff) \
	((((ff) & FRAME_LOWMEM) ? ZONE_LOWMEM : \
	    (((ff) & FRAME_HIGHMEM) ? ZONE_HIGHMEM : \
	    ZONE_LOWMEM /* | ZONE_HIGHMEM */)) | \
	    ZONE_AVAILABLE)

#define ZONE_FLAGS_MATCH(zf, f) \
	(((((zf) & ZONE_EF_MASK)) == ((f) & ZONE_EF_MASK)) && \
	    (((zf) & ~ZONE_EF_MASK) & (f)))

typedef struct {
	size_t refcount;  /**< Tracking of shared frames */
	void *parent;     /**< If allocated by slab, this points there */
} frame_t;

typedef struct {
	/** Frame_no of the first frame in the frames array */
	pfn_t base;

	/** Size of zone */
	size_t count;

	/** Number of free frame_t structures */
	size_t free_count;

	/** Number of busy frame_t structures */
	size_t busy_count;

	/** Type of the zone */
	zone_flags_t flags;

	/** Frame bitmap */
	bitmap_t bitmap;

	/** Array of frame_t structures in this zone */
	frame_t *frames;
} zone_t;

/*
 * The zoneinfo.lock must be locked when accessing zoneinfo structure.
 * Some of the attributes in zone_t structures are 'read-only'
 */
typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);
	size_t count;
	zone_t info[ZONES_MAX];
} zones_t;

extern zones_t zones;

extern void frame_init(void);
extern bool frame_adjust_zone_bounds(bool, uintptr_t *, size_t *);
extern uintptr_t frame_alloc_generic(size_t, frame_flags_t, uintptr_t,
    size_t *);
extern uintptr_t frame_alloc(size_t, frame_flags_t, uintptr_t);
extern void frame_free_generic(uintptr_t, size_t, frame_flags_t);
extern void frame_free(uintptr_t, size_t);
extern void frame_free_noreserve(uintptr_t, size_t);
extern void frame_reference_add(pfn_t);
extern size_t frame_total_free_get(void);

extern size_t find_zone(pfn_t, size_t, size_t);
extern size_t zone_create(pfn_t, size_t, pfn_t, zone_flags_t);
extern void *frame_get_parent(pfn_t, size_t);
extern void frame_set_parent(pfn_t, void *, size_t);
extern void frame_mark_unavailable(pfn_t, size_t);
extern size_t zone_conf_size(size_t);
extern pfn_t zone_external_conf_alloc(size_t);
extern bool zone_merge(size_t, size_t);
extern void zone_merge_all(void);
extern uint64_t zones_total_size(void);
extern void zones_stats(uint64_t *, uint64_t *, uint64_t *, uint64_t *);

/*
 * Console functions
 */
extern void zones_print_list(void);
extern void zone_print_one(size_t);

#endif

/** @}
 */
