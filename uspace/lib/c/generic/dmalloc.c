/*
 * Copyright (c) 2026 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file Debug memory allocator.
 */

#include <adt/list.h>
#include <dmalloc.h>
#include <io/klog.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>

#include "private/dmalloc.h"

/** List of all heap metadata blocks. */
static LIST_INITIALIZE(dblocks);

/** Get first heap metadata block.
 *
 * @return First metadata block.
 */
static dblock_t *dblock_first(void)
{
	link_t *link;

	link = list_first(&dblocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, dblock_t, lblocks);
}

/** Get next heap metadata block.
 *
 * @param cur Current metadata block.
 * @return First metadata block.
 */
static dblock_t *dblock_next(dblock_t *cur)
{
	link_t *link;

	link = list_next(&cur->lblocks, &dblocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, dblock_t, lblocks);
}

/** Find metadata block by user data pointer.
 *
 * @param ptr User data pointer.
 * @return Metadata block or @c NULL if not found.
 */
static dblock_t *dblock_find(void *ptr)
{
	dblock_t *dblock;

	dblock = dblock_first();
	while (dblock != NULL) {
		if (dblock->ptr == ptr)
			return dblock;
		dblock = dblock_next(dblock);
	}

	return NULL;
}

/** Check heap block consistency.
 *
 * @param dblock Metadata block
 */
static void dblock_check(dblock_t *dblock)
{
	size_t i;
	uint8_t *p = (uint8_t *)dblock->ptr;

	/* Check padding at the beginning. */
	for (i = 0; i < pad_begin; i++) {
		if (p[i - pad_begin] != pad_pattern) {
			KLOG_PRINTF(LVL_NOTE, "dmalloc: block %p begin pad is "
			    "corrupted at index %zu value 0x%x", dblock->ptr,
			    i, p[i - pad_begin]);
			abort();
		}
	}

	/* Check padding at the end. */
	for (i = 0; i < pad_end; i++) {
		if (p[dblock->size + i] != pad_pattern) {
			KLOG_PRINTF(LVL_NOTE, "dmalloc: block %p end pad is "
			    "corrupted at index %zu value 0x%x", dblock->ptr,
			    i, p[dblock->size + i]);
			abort();
		}
	}

	/* Check contents of freed block. */
	if (dblock->freed != 0) {
		for (i = 0; i < dblock->size; i++) {
			if (p[i] != pad_pattern) {
				KLOG_PRINTF(LVL_NOTE, "dmalloc: block %p "
				    "freed pattern is corrupted at index "
				    "%zu value 0x%x", dblock->ptr,
				    i, p[i]);
				abort();
			}
		}
	}
}

/** Allocate memory block.
 *
 * @param size Block size
 * @return Pointer to new block or @c NULL if out of memory
 */
void *malloc(size_t size)
{
	return memalign(16, size);
}

/** Free memory block.
 *
 * @param ptr Memory block
 */
void free(void *ptr)
{
	if (ptr == NULL)
		return;

	dblock_t *dblock = dblock_find(ptr);
	if (dblock == NULL) {
		KLOG_PRINTF(LVL_NOTE, "dmalloc: free() of unknown pointer %p",
		    ptr);
		abort();
	}
	if (dblock->freed != 0) {
		KLOG_PRINTF(LVL_NOTE, "dmalloc: free() of freed pointer %p",
		    ptr);
		abort();
	}

	dblock->freed = true;
	memset(ptr, freed_pattern, dblock->size);
	_free(ptr - pad_begin);
	list_remove(&dblock->lblocks);
	_free(dblock);
}

/** Resize block.
 *
 * @param ptr Existing block or @c NULL to allocate memory.
 * @param size New size
 * @return New block, @c NULL if out of memory or if @a size is zero.
 */
void *realloc(void *ptr, size_t size)
{
	void *newptr;

	if (ptr == NULL)
		return malloc(size);
	if (ptr != NULL && size == 0) {
		free(ptr);
		return NULL;
	}

	dblock_t *dblock = dblock_find(ptr);
	if (dblock == NULL) {
		KLOG_PRINTF(LVL_NOTE, "dmalloc: realloc() of unknown pointer %p",
		    ptr);
		abort();
	}
	if (dblock->freed != 0) {
		KLOG_PRINTF(LVL_NOTE, "dmalloc: realloc() of freed pointer %p",
		    ptr);
		abort();
	}

	size_t overlap;
	if (size < dblock->size)
		overlap = size;
	else
		overlap = dblock->size;

	newptr = malloc(size);
	memcpy(newptr, ptr, overlap);
	free(ptr);
	return newptr;
}

/** Allocate aligned memory.
 *
 * @param align Alignment, must be power of two.
 * @param size Size of allocated block
 * @return Pointer to new block or @c NULL if out of memory
 */
void *memalign(const size_t align, const size_t size)
{
	dblock_t *dblock = _malloc(sizeof(dblock_t));

	if (dblock == NULL)
		return NULL;

	link_initialize(&dblock->lblocks);
	dblock->freed = 0;
	dblock->size = size;
	dblock->ptr = _memalign(align, pad_begin + size + pad_end) + pad_begin;

	memset(dblock->ptr - pad_begin, pad_pattern, pad_begin);
	memset(dblock->ptr + dblock->size, pad_pattern, pad_end);
	memset(dblock->ptr, uninit_pattern, dblock->size);

	if (dblock->ptr == NULL) {
		_free(dblock);
		return NULL;
	}

	list_append(&dblock->lblocks, &dblocks);
	return dblock->ptr;
}

/** Reallocate array.
 *
 * @param ptr Current pointer
 * @param n New number of elements
 * @param size New element size
 * @return Pointer to resized block or @!c NULL if out of memory.
 *         If NULL is returned, the original block is unchanged.
 */
void *reallocarray(void *ptr, size_t n, size_t size)
{
	return realloc(ptr, n * size);
}

/** Check consistency of all heap blocks. */
void dmalloc_check_heap(void)
{
	dblock_t *dblock;

	dblock = dblock_first();
	while (dblock != NULL) {
		dblock_check(dblock);
		dblock = dblock_next(dblock);
	}
}

/** @}
 */
