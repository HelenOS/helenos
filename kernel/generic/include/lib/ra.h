/*
 * Copyright (c) 2011 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_RA_H_
#define KERN_RA_H_

#include <typedefs.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <synch/spinlock.h>

typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);
	list_t spans;		/**< List of arena's spans. */
} ra_arena_t;

typedef struct {
	link_t span_link;	/**< Arena's list of spans link. */

	list_t segments;	/**< List of span's segments. */

	size_t max_order;	/**< Base 2 logarithm of span's size. */
	list_t *free;		/**< max_order segment free lists. */

	hash_table_t used;

	uintptr_t base;		/**< Span base. */
	size_t size;		/**< Span size. */
} ra_span_t;

#define RA_SEGMENT_FREE		1

/*
 * We would like to achieve a good ratio of the size of one unit of the
 * represented resource (e.g. a page) and sizeof(ra_segment_t).  We therefore
 * attempt to have as few redundant information in the segment as possible. For
 * example, the size of the segment needs to be calculated from the segment
 * base and the base of the following segment.
 */
typedef struct {
	link_t segment_link;	/**< Span's segment list link. */

	/*
	 * A segment cannot be both on the free list and in the used hash.
	 * Their respective links can therefore occupy the same space.
	 */
	union {
		link_t fl_link;		/**< Span's free list link. */
		ht_link_t uh_link;	/**< Span's used hash link. */
	};

	uintptr_t base;		/**< Segment base. */
	uint8_t flags;		/**< Segment flags. */
} ra_segment_t;

extern void ra_init(void);
extern ra_arena_t *ra_arena_create(void);
extern void ra_arena_destroy(ra_arena_t *);
extern bool ra_span_add(ra_arena_t *, uintptr_t, size_t);
extern bool ra_alloc(ra_arena_t *, size_t, size_t, uintptr_t *);
extern void ra_free(ra_arena_t *, uintptr_t, size_t);

#endif

/** @}
 */
