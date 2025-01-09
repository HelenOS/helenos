/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2018 Jiří Zárevúcky
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

#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <align.h>
#include <bitops.h>
#include <mm/slab.h>
#include <memw.h>
#include <main/main.h> // malloc_init()
#include <macros.h>

/** Minimum size to be allocated by malloc */
#define SLAB_MIN_MALLOC_W  4

/** Maximum size to be allocated by malloc */
#define SLAB_MAX_MALLOC_W  22

/** Caches for malloc */
static slab_cache_t *malloc_caches[SLAB_MAX_MALLOC_W - SLAB_MIN_MALLOC_W + 1];

static const char *malloc_names[] =  {
	"malloc-16",
	"malloc-32",
	"malloc-64",
	"malloc-128",
	"malloc-256",
	"malloc-512",
	"malloc-1K",
	"malloc-2K",
	"malloc-4K",
	"malloc-8K",
	"malloc-16K",
	"malloc-32K",
	"malloc-64K",
	"malloc-128K",
	"malloc-256K",
	"malloc-512K",
	"malloc-1M",
	"malloc-2M",
	"malloc-4M"
};

void malloc_init(void)
{
	/* Initialize structures for malloc */
	size_t i;
	size_t size;

	for (i = 0, size = (1 << SLAB_MIN_MALLOC_W);
	    i < (SLAB_MAX_MALLOC_W - SLAB_MIN_MALLOC_W + 1);
	    i++, size <<= 1) {
		malloc_caches[i] = slab_cache_create(malloc_names[i], size, 0,
		    NULL, NULL, SLAB_CACHE_MAGDEFERRED);
	}
}

static void _check_sizes(size_t *alignment, size_t *size)
{
	assert(size);
	assert(alignment);

	/* Force size to be nonzero. */
	if (*size == 0)
		*size = 1;

	/* Alignment must be a power of 2. */
	assert(ispwr2(*alignment));
	assert(*alignment <= PAGE_SIZE);

	if (*alignment < alignof(max_align_t))
		*alignment = alignof(max_align_t);

	*size = ALIGN_UP(*size, *alignment);

	if (*size < (1 << SLAB_MIN_MALLOC_W))
		*size = (1 << SLAB_MIN_MALLOC_W);
}

static slab_cache_t *cache_for_size(size_t size)
{
	assert(size > 0);
	assert(size <= (1 << SLAB_MAX_MALLOC_W));

	size_t idx = fnzb(size - 1) - SLAB_MIN_MALLOC_W + 1;

	assert(idx < sizeof(malloc_caches) / sizeof(malloc_caches[0]));

	slab_cache_t *cache = malloc_caches[idx];

	assert(cache != NULL);
	return cache;
}

// TODO: Expose publicly and use mem_alloc() and mem_free() instead of malloc()

static void *mem_alloc(size_t, size_t) __attribute__((malloc));

static void *mem_alloc(size_t alignment, size_t size)
{
	_check_sizes(&alignment, &size);

	if (size > (1 << SLAB_MAX_MALLOC_W)) {
		// TODO: Allocate big objects directly from coarse allocator.
		assert(size <= (1 << SLAB_MAX_MALLOC_W));
	}

	/* We assume that slab objects are aligned naturally */
	return slab_alloc(cache_for_size(size), FRAME_ATOMIC);
}

static void *mem_realloc(void *old_ptr, size_t alignment, size_t old_size,
    size_t new_size)
{
	assert(old_ptr);
	_check_sizes(&alignment, &old_size);
	_check_sizes(&alignment, &new_size);

	// TODO: handle big objects
	assert(new_size <= (1 << SLAB_MAX_MALLOC_W));

	slab_cache_t *old_cache = cache_for_size(old_size);
	slab_cache_t *new_cache = cache_for_size(new_size);
	if (old_cache == new_cache)
		return old_ptr;

	void *new_ptr = slab_alloc(new_cache, FRAME_ATOMIC);
	if (!new_ptr)
		return NULL;

	memcpy(new_ptr, old_ptr, min(old_size, new_size));
	slab_free(old_cache, old_ptr);
	return new_ptr;
}

/**
 * Free memory allocated using mem_alloc().
 *
 * @param ptr        Pointer returned by mem_alloc().
 * @param size       Size used to call mem_alloc().
 * @param alignment  Alignment used to call mem_alloc().
 */
static void mem_free(void *ptr, size_t alignment, size_t size)
{
	if (!ptr)
		return;

	_check_sizes(&alignment, &size);

	if (size > (1 << SLAB_MAX_MALLOC_W)) {
		// TODO: Allocate big objects directly from coarse allocator.
		assert(size <= (1 << SLAB_MAX_MALLOC_W));
	}

	return slab_free(cache_for_size(size), ptr);
}

static const size_t _offset = ALIGN_UP(sizeof(size_t), alignof(max_align_t));

void *malloc(size_t size)
{
	if (size + _offset < size)
		return NULL;

	void *obj = mem_alloc(alignof(max_align_t), size + _offset) + _offset;

	/* Remember the allocation size just before the object. */
	((size_t *) obj)[-1] = size;
	return obj;
}

void free(void *obj)
{
	/*
	 * We don't check integrity of size, so buffer over/underruns can
	 * corrupt it. That's ok, it ultimately only serves as a hint to
	 * select the correct slab cache. If the selected cache is not correct,
	 * slab_free() will detect it and panic.
	 */
	size_t size = ((size_t *) obj)[-1];
	mem_free(obj - _offset, alignof(max_align_t), size + _offset);
}

void *realloc(void *old_obj, size_t new_size)
{
	if (new_size == 0)
		new_size = 1;

	if (!old_obj)
		return malloc(new_size);

	size_t old_size = ((size_t *) old_obj)[-1];

	void *new_obj = mem_realloc(old_obj - _offset, alignof(max_align_t),
	    old_size + _offset, new_size + _offset) + _offset;
	if (!new_obj)
		return NULL;

	((size_t *) new_obj)[-1] = new_size;
	return new_obj;
}
