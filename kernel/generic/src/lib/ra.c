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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief Resource allocator.
 *
 * This is a generic resource allocator, loosely based on the ideas presented
 * in chapter 4 of the following paper and further simplified:
 *
 *   Bonwick J., Adams J.: Magazines and Vmem: Extending the Slab Allocator to
 *   Many CPUs and Arbitrary Resources, USENIX 2001
 *
 */

#include <lib/ra.h>
#include <typedefs.h>
#include <mm/slab.h>
#include <bitops.h>
#include <debug.h>

static hash_table_operations_t used_ops = {
	.hash = NULL,
	.compare = NULL,
	.remove_callback = NULL,
};

static ra_segment_t *ra_segment_create(uintptr_t base, size_t size)
{
	ra_segment_t *seg;

	seg = (ra_segment_t *) malloc(sizeof(ra_segment_t), FRAME_ATOMIC);
	if (!seg)
		return NULL;

	link_initialize(&seg->segment_link);
	link_initialize(&seg->free_link);
	link_initialize(&seg->used_link);

	seg->base = base;
	seg->size = size;

	return seg;
}

/*
static void ra_segment_destroy(ra_segment_t *seg)
{
	free(seg);
}
*/

static ra_span_t *ra_span_create(uintptr_t base, size_t size)
{
	ra_span_t *span;
	ra_segment_t *seg;
	unsigned int i;

	span = (ra_span_t *) malloc(sizeof(ra_span_t), FRAME_ATOMIC);
	if (!span)
		return NULL;

	span->max_order = fnzb(size);

	span->free = (list_t *) malloc((span->max_order + 1) * sizeof(list_t),
	    FRAME_ATOMIC);
	if (!span->free) {
		free(span);
		return NULL;
	}
	seg = ra_segment_create(base, size);
	if (!seg) {
		free(span->free);
		free(span);
		return NULL;
	}

	link_initialize(&span->span_link);
	list_initialize(&span->segments);

	hash_table_create(&span->used, span->max_order + 1, 1, &used_ops);

	for (i = 0; i < span->max_order; i++)
		list_initialize(&span->free[i]);

	list_append(&seg->segment_link, &span->segments);
	list_append(&seg->free_link, &span->free[span->max_order]);

	return span;
}

ra_arena_t *ra_arena_create(uintptr_t base, size_t size)
{
	ra_arena_t *arena;
	ra_span_t *span;

	arena = (ra_arena_t *) malloc(sizeof(ra_arena_t), FRAME_ATOMIC);
	if (!arena)
		return NULL;

	span = ra_span_create(base, size);
	if (!span) {
		free(arena);
		return NULL;
	}

	list_initialize(&arena->spans);
	list_append(&span->span_link, &arena->spans);

	return arena;
}

void ra_span_add(ra_arena_t *arena, uintptr_t base, size_t size)
{
	ra_span_t *span;

	span = ra_span_create(base, size);
	ASSERT(span);

	/* TODO: check for overlaps */
	list_append(&span->span_link, &arena->spans);
}

uintptr_t ra_alloc(ra_arena_t *arena, size_t size, size_t alignment)
{
	return 0;
}

void ra_free(ra_arena_t *arena, uintptr_t base, size_t size)
{
}


/** @}
 */
