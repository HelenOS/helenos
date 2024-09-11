/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2009 Petr Tuma
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
/** @file
 */

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <as.h>
#include <align.h>
#include <macros.h>
#include <assert.h>
#include <errno.h>
#include <bitops.h>
#include <mem.h>
#include <stdlib.h>
#include <adt/gcdlcm.h>
#include <malloc.h>

#include "private/malloc.h"
#include "private/fibril.h"

/** Magic used in heap headers. */
#define HEAP_BLOCK_HEAD_MAGIC  UINT32_C(0xBEEF0101)

/** Magic used in heap footers. */
#define HEAP_BLOCK_FOOT_MAGIC  UINT32_C(0xBEEF0202)

/** Magic used in heap descriptor. */
#define HEAP_AREA_MAGIC  UINT32_C(0xBEEFCAFE)

/** Allocation alignment.
 *
 * This also covers the alignment of fields
 * in the heap header and footer.
 *
 */
#define BASE_ALIGN  16

/** Heap shrink granularity
 *
 * Try not to pump and stress the heap too much
 * by shrinking and enlarging it too often.
 * A heap area won't shrink if the released
 * free block is smaller than this constant.
 *
 */
#define SHRINK_GRANULARITY  (64 * PAGE_SIZE)

/** Overhead of each heap block. */
#define STRUCT_OVERHEAD \
	(sizeof(heap_block_head_t) + sizeof(heap_block_foot_t))

/** Calculate real size of a heap block.
 *
 * Add header and footer size.
 *
 */
#define GROSS_SIZE(size)  ((size) + STRUCT_OVERHEAD)

/** Calculate net size of a heap block.
 *
 * Subtract header and footer size.
 *
 */
#define NET_SIZE(size)  ((size) - STRUCT_OVERHEAD)

/** Overhead of each area. */
#define AREA_OVERHEAD(size) \
	(ALIGN_UP(GROSS_SIZE(size) + sizeof(heap_area_t), BASE_ALIGN))

/** Get first block in heap area.
 *
 */
#define AREA_FIRST_BLOCK_HEAD(area) \
	(ALIGN_UP(((uintptr_t) (area)) + sizeof(heap_area_t), BASE_ALIGN))

/** Get last block in heap area.
 *
 */
#define AREA_LAST_BLOCK_FOOT(area) \
	(((uintptr_t) (area)->end) - sizeof(heap_block_foot_t))

#define AREA_LAST_BLOCK_HEAD(area) \
	((uintptr_t) BLOCK_HEAD(((heap_block_foot_t *) AREA_LAST_BLOCK_FOOT(area))))

/** Get header in heap block.
 *
 */
#define BLOCK_HEAD(foot) \
	((heap_block_head_t *) \
	    (((uintptr_t) (foot)) + sizeof(heap_block_foot_t) - (foot)->size))

/** Get footer in heap block.
 *
 */
#define BLOCK_FOOT(head) \
	((heap_block_foot_t *) \
	    (((uintptr_t) (head)) + (head)->size - sizeof(heap_block_foot_t)))

/** Heap area.
 *
 * The memory managed by the heap allocator is divided into
 * multiple discontinuous heaps. Each heap is represented
 * by a separate address space area which has this structure
 * at its very beginning.
 *
 */
typedef struct heap_area {
	/** Start of the heap area (including this structure)
	 *
	 * Aligned on page boundary.
	 *
	 */
	void *start;

	/** End of the heap area (aligned on page boundary) */
	void *end;

	/** Previous heap area */
	struct heap_area *prev;

	/** Next heap area */
	struct heap_area *next;

	/** A magic value */
	uint32_t magic;
} heap_area_t;

/** Header of a heap block
 *
 */
typedef struct {
	/* Size of the block (including header and footer) */
	size_t size;

	/* Indication of a free block */
	bool free;

	/** Heap area this block belongs to */
	heap_area_t *area;

	/* A magic value to detect overwrite of heap header */
	uint32_t magic;
} heap_block_head_t;

/** Footer of a heap block
 *
 */
typedef struct {
	/* Size of the block (including header and footer) */
	size_t size;

	/* A magic value to detect overwrite of heap footer */
	uint32_t magic;
} heap_block_foot_t;

/** First heap area */
static heap_area_t *first_heap_area = NULL;

/** Last heap area */
static heap_area_t *last_heap_area = NULL;

/** Next heap block to examine (next fit algorithm) */
static heap_block_head_t *next_fit = NULL;

/** Futex for thread-safe heap manipulation */
static fibril_rmutex_t malloc_mutex;

#define malloc_assert(expr) safe_assert(expr)

/*
 * Make sure the base alignment is sufficient.
 */
static_assert(BASE_ALIGN >= alignof(heap_area_t), "");
static_assert(BASE_ALIGN >= alignof(heap_block_head_t), "");
static_assert(BASE_ALIGN >= alignof(heap_block_foot_t), "");
static_assert(BASE_ALIGN >= alignof(max_align_t), "");

/** Serializes access to the heap from multiple threads. */
static inline void heap_lock(void)
{
	fibril_rmutex_lock(&malloc_mutex);
}

/** Serializes access to the heap from multiple threads. */
static inline void heap_unlock(void)
{
	fibril_rmutex_unlock(&malloc_mutex);
}

/** Initialize a heap block
 *
 * Fill in the structures related to a heap block.
 * Should be called only inside the critical section.
 *
 * @param addr Address of the block.
 * @param size Size of the block including the header and the footer.
 * @param free Indication of a free block.
 * @param area Heap area the block belongs to.
 *
 */
static void block_init(void *addr, size_t size, bool free, heap_area_t *area)
{
	/* Calculate the position of the header and the footer */
	heap_block_head_t *head = (heap_block_head_t *) addr;

	head->size = size;
	head->free = free;
	head->area = area;
	head->magic = HEAP_BLOCK_HEAD_MAGIC;

	heap_block_foot_t *foot = BLOCK_FOOT(head);

	foot->size = size;
	foot->magic = HEAP_BLOCK_FOOT_MAGIC;
}

/** Check a heap block
 *
 * Verifies that the structures related to a heap block still contain
 * the magic constants. This helps detect heap corruption early on.
 * Should be called only inside the critical section.
 *
 * @param addr Address of the block.
 *
 */
static void block_check(void *addr)
{
	heap_block_head_t *head = (heap_block_head_t *) addr;

	malloc_assert(head->magic == HEAP_BLOCK_HEAD_MAGIC);

	heap_block_foot_t *foot = BLOCK_FOOT(head);

	malloc_assert(foot->magic == HEAP_BLOCK_FOOT_MAGIC);
	malloc_assert(head->size == foot->size);
}

/** Check a heap area structure
 *
 * Should be called only inside the critical section.
 *
 * @param addr Address of the heap area.
 *
 */
static void area_check(void *addr)
{
	heap_area_t *area = (heap_area_t *) addr;

	malloc_assert(area->magic == HEAP_AREA_MAGIC);
	malloc_assert(addr == area->start);
	malloc_assert(area->start < area->end);
	malloc_assert(((uintptr_t) area->start % PAGE_SIZE) == 0);
	malloc_assert(((uintptr_t) area->end % PAGE_SIZE) == 0);
}

/** Create new heap area
 *
 * Should be called only inside the critical section.
 *
 * @param size Size of the area.
 *
 */
static bool area_create(size_t size)
{
	/* Align the heap area size on page boundary */
	size_t asize = ALIGN_UP(size, PAGE_SIZE);
	void *astart = as_area_create(AS_AREA_ANY, asize,
	    AS_AREA_WRITE | AS_AREA_READ | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (astart == AS_MAP_FAILED)
		return false;

	heap_area_t *area = (heap_area_t *) astart;

	area->start = astart;
	area->end = (void *) ((uintptr_t) astart + asize);
	area->prev = NULL;
	area->next = NULL;
	area->magic = HEAP_AREA_MAGIC;

	void *block = (void *) AREA_FIRST_BLOCK_HEAD(area);
	size_t bsize = (size_t) (area->end - block);

	block_init(block, bsize, true, area);

	if (last_heap_area == NULL) {
		first_heap_area = area;
		last_heap_area = area;
	} else {
		area->prev = last_heap_area;
		last_heap_area->next = area;
		last_heap_area = area;
	}

	return true;
}

/** Try to enlarge a heap area
 *
 * Should be called only inside the critical section.
 *
 * @param area Heap area to grow.
 * @param size Gross size to grow (bytes).
 *
 * @return True if successful.
 *
 */
static bool area_grow(heap_area_t *area, size_t size)
{
	if (size == 0)
		return true;

	area_check(area);

	/* New heap area size */
	size_t gross_size = (size_t) (area->end - area->start) + size;
	size_t asize = ALIGN_UP(gross_size, PAGE_SIZE);
	void *end = (void *) ((uintptr_t) area->start + asize);

	/* Check for overflow */
	if (end < area->start)
		return false;

	/* Resize the address space area */
	errno_t ret = as_area_resize(area->start, asize, 0);
	if (ret != EOK)
		return false;

	heap_block_head_t *last_head =
	    (heap_block_head_t *) AREA_LAST_BLOCK_HEAD(area);

	if (last_head->free) {
		/* Add the new space to the last block. */
		size_t net_size = (size_t) (end - area->end) + last_head->size;
		malloc_assert(net_size > 0);
		block_init(last_head, net_size, true, area);
	} else {
		/* Add new free block */
		size_t net_size = (size_t) (end - area->end);
		if (net_size > 0)
			block_init(area->end, net_size, true, area);
	}

	/* Update heap area parameters */
	area->end = end;

	return true;
}

/** Try to shrink heap
 *
 * Should be called only inside the critical section.
 * In all cases the next pointer is reset.
 *
 * @param area Last modified heap area.
 *
 */
static void heap_shrink(heap_area_t *area)
{
	area_check(area);

	heap_block_foot_t *last_foot =
	    (heap_block_foot_t *) AREA_LAST_BLOCK_FOOT(area);
	heap_block_head_t *last_head = BLOCK_HEAD(last_foot);

	block_check((void *) last_head);
	malloc_assert(last_head->area == area);

	if (last_head->free) {
		/*
		 * The last block of the heap area is
		 * unused. The area might be potentially
		 * shrunk.
		 */

		heap_block_head_t *first_head =
		    (heap_block_head_t *) AREA_FIRST_BLOCK_HEAD(area);

		block_check((void *) first_head);
		malloc_assert(first_head->area == area);

		size_t shrink_size = ALIGN_DOWN(last_head->size, PAGE_SIZE);

		if (first_head == last_head) {
			/*
			 * The entire heap area consists of a single
			 * free heap block. This means we can get rid
			 * of it entirely.
			 */

			heap_area_t *prev = area->prev;
			heap_area_t *next = area->next;

			if (prev != NULL) {
				area_check(prev);
				prev->next = next;
			} else
				first_heap_area = next;

			if (next != NULL) {
				area_check(next);
				next->prev = prev;
			} else
				last_heap_area = prev;

			as_area_destroy(area->start);
		} else if (shrink_size >= SHRINK_GRANULARITY) {
			/*
			 * Make sure that we always shrink the area
			 * by a multiple of page size and update
			 * the block layout accordingly.
			 */

			size_t asize = (size_t) (area->end - area->start) - shrink_size;
			void *end = (void *) ((uintptr_t) area->start + asize);

			/* Resize the address space area */
			errno_t ret = as_area_resize(area->start, asize, 0);
			if (ret != EOK)
				abort();

			/* Update heap area parameters */
			area->end = end;
			size_t excess = ((size_t) area->end) - ((size_t) last_head);

			if (excess > 0) {
				if (excess >= STRUCT_OVERHEAD) {
					/*
					 * The previous block cannot be free and there
					 * is enough free space left in the area to
					 * create a new free block.
					 */
					block_init((void *) last_head, excess, true, area);
				} else {
					/*
					 * The excess is small. Therefore just enlarge
					 * the previous block.
					 */
					heap_block_foot_t *prev_foot = (heap_block_foot_t *)
					    (((uintptr_t) last_head) - sizeof(heap_block_foot_t));
					heap_block_head_t *prev_head = BLOCK_HEAD(prev_foot);

					block_check((void *) prev_head);

					block_init(prev_head, prev_head->size + excess,
					    prev_head->free, area);
				}
			}
		}
	}

	next_fit = NULL;
}

/** Initialize the heap allocator
 *
 * Create initial heap memory area. This routine is
 * only called from libc initialization, thus we do not
 * take any locks.
 *
 */
void __malloc_init(void)
{
	if (fibril_rmutex_initialize(&malloc_mutex) != EOK)
		abort();

	if (!area_create(PAGE_SIZE))
		abort();
}

void __malloc_fini(void)
{
	fibril_rmutex_destroy(&malloc_mutex);
}

/** Split heap block and mark it as used.
 *
 * Should be called only inside the critical section.
 *
 * @param cur  Heap block to split.
 * @param size Number of bytes to split and mark from the beginning
 *             of the block.
 *
 */
static void split_mark(heap_block_head_t *cur, const size_t size)
{
	malloc_assert(cur->size >= size);

	/* See if we should split the block. */
	size_t split_limit = GROSS_SIZE(size);

	if (cur->size > split_limit) {
		/* Block big enough -> split. */
		void *next = ((void *) cur) + size;
		block_init(next, cur->size - size, true, cur->area);
		block_init(cur, size, false, cur->area);
	} else {
		/* Block too small -> use as is. */
		cur->free = false;
	}
}

/** Allocate memory from heap area starting from given block
 *
 * Should be called only inside the critical section.
 * As a side effect this function also sets the current
 * pointer on successful allocation.
 *
 * @param area        Heap area where to allocate from.
 * @param first_block Starting heap block.
 * @param final_block Heap block where to finish the search
 *                    (may be NULL).
 * @param real_size   Gross number of bytes to allocate.
 * @param falign      Physical alignment of the block.
 *
 * @return Address of the allocated block or NULL on not enough memory.
 *
 */
static void *malloc_area(heap_area_t *area, heap_block_head_t *first_block,
    heap_block_head_t *final_block, size_t real_size, size_t falign)
{
	area_check((void *) area);
	malloc_assert((void *) first_block >= (void *) AREA_FIRST_BLOCK_HEAD(area));
	malloc_assert((void *) first_block < area->end);

	for (heap_block_head_t *cur = first_block; (void *) cur < area->end;
	    cur = (heap_block_head_t *) (((void *) cur) + cur->size)) {
		block_check(cur);

		/* Finish searching on the final block */
		if ((final_block != NULL) && (cur == final_block))
			break;

		/* Try to find a block that is free and large enough. */
		if ((cur->free) && (cur->size >= real_size)) {
			/*
			 * We have found a suitable block.
			 * Check for alignment properties.
			 */
			void *addr = (void *)
			    ((uintptr_t) cur + sizeof(heap_block_head_t));
			void *aligned = (void *)
			    ALIGN_UP((uintptr_t) addr, falign);

			if (addr == aligned) {
				/* Exact block start including alignment. */
				split_mark(cur, real_size);

				next_fit = cur;
				return addr;
			} else {
				/* Block start has to be aligned */
				size_t excess = (size_t) (aligned - addr);

				if (cur->size >= real_size + excess) {
					/*
					 * The current block is large enough to fit
					 * data in (including alignment).
					 */
					if ((void *) cur > (void *) AREA_FIRST_BLOCK_HEAD(area)) {
						/*
						 * There is a block before the current block.
						 * This previous block can be enlarged to
						 * compensate for the alignment excess.
						 */
						heap_block_foot_t *prev_foot = (heap_block_foot_t *)
						    ((void *) cur - sizeof(heap_block_foot_t));

						heap_block_head_t *prev_head = (heap_block_head_t *)
						    ((void *) cur - prev_foot->size);

						block_check(prev_head);

						size_t reduced_size = cur->size - excess;
						heap_block_head_t *next_head = ((void *) cur) + excess;

						if ((!prev_head->free) &&
						    (excess >= STRUCT_OVERHEAD)) {
							/*
							 * The previous block is not free and there
							 * is enough free space left to fill in
							 * a new free block between the previous
							 * and current block.
							 */
							block_init(cur, excess, true, area);
						} else {
							/*
							 * The previous block is free (thus there
							 * is no need to induce additional
							 * fragmentation to the heap) or the
							 * excess is small. Therefore just enlarge
							 * the previous block.
							 */
							block_init(prev_head, prev_head->size + excess,
							    prev_head->free, area);
						}

						block_init(next_head, reduced_size, true, area);
						split_mark(next_head, real_size);

						next_fit = next_head;
						return aligned;
					} else {
						/*
						 * The current block is the first block
						 * in the heap area. We have to make sure
						 * that the alignment excess is large enough
						 * to fit a new free block just before the
						 * current block.
						 */
						while (excess < STRUCT_OVERHEAD) {
							aligned += falign;
							excess += falign;
						}

						/* Check for current block size again */
						if (cur->size >= real_size + excess) {
							size_t reduced_size = cur->size - excess;
							cur = (heap_block_head_t *)
							    (AREA_FIRST_BLOCK_HEAD(area) + excess);

							block_init((void *) AREA_FIRST_BLOCK_HEAD(area),
							    excess, true, area);
							block_init(cur, reduced_size, true, area);
							split_mark(cur, real_size);

							next_fit = cur;
							return aligned;
						}
					}
				}
			}
		}
	}

	return NULL;
}

/** Try to enlarge any of the heap areas.
 *
 * If successful, allocate block of the given size in the area.
 * Should be called only inside the critical section.
 *
 * @param size  Gross size of item to allocate (bytes).
 * @param align Memory address alignment.
 *
 * @return Allocated block.
 * @return NULL on failure.
 *
 */
static void *heap_grow_and_alloc(size_t size, size_t align)
{
	if (size == 0)
		return NULL;

	/* First try to enlarge some existing area */
	for (heap_area_t *area = first_heap_area; area != NULL;
	    area = area->next) {

		if (area_grow(area, size + align)) {
			heap_block_head_t *first =
			    (heap_block_head_t *) AREA_LAST_BLOCK_HEAD(area);

			void *addr =
			    malloc_area(area, first, NULL, size, align);
			malloc_assert(addr != NULL);
			return addr;
		}
	}

	/* Eventually try to create a new area */
	if (area_create(AREA_OVERHEAD(size + align))) {
		heap_block_head_t *first =
		    (heap_block_head_t *) AREA_FIRST_BLOCK_HEAD(last_heap_area);

		void *addr =
		    malloc_area(last_heap_area, first, NULL, size, align);
		malloc_assert(addr != NULL);
		return addr;
	}

	return NULL;
}

/** Allocate a memory block
 *
 * Should be called only inside the critical section.
 *
 * @param size  The size of the block to allocate.
 * @param align Memory address alignment.
 *
 * @return Address of the allocated block or NULL on not enough memory.
 *
 */
static void *malloc_internal(size_t size, size_t align)
{
	malloc_assert(first_heap_area != NULL);

	if (size == 0)
		size = 1;

	if (align == 0)
		align = BASE_ALIGN;

	size_t falign = lcm(align, BASE_ALIGN);

	/* Check for integer overflow. */
	if (falign < align)
		return NULL;

	/*
	 * The size of the allocated block needs to be naturally
	 * aligned, because the footer structure also needs to reside
	 * on a naturally aligned address in order to avoid unaligned
	 * memory accesses.
	 */
	size_t gross_size = GROSS_SIZE(ALIGN_UP(size, BASE_ALIGN));

	/* Try the next fit approach */
	heap_block_head_t *split = next_fit;

	if (split != NULL) {
		void *addr = malloc_area(split->area, split, NULL, gross_size,
		    falign);

		if (addr != NULL)
			return addr;
	}

	/* Search the entire heap */
	for (heap_area_t *area = first_heap_area; area != NULL;
	    area = area->next) {
		heap_block_head_t *first = (heap_block_head_t *)
		    AREA_FIRST_BLOCK_HEAD(area);

		void *addr = malloc_area(area, first, split, gross_size,
		    falign);

		if (addr != NULL)
			return addr;
	}

	/* Finally, try to grow heap space and allocate in the new area. */
	return heap_grow_and_alloc(gross_size, falign);
}

/** Allocate memory
 *
 * @param size Number of bytes to allocate.
 *
 * @return Allocated memory or NULL.
 *
 */
void *malloc(const size_t size)
{
	heap_lock();
	void *block = malloc_internal(size, BASE_ALIGN);
	heap_unlock();

	return block;
}

/** Allocate memory with specified alignment
 *
 * @param align Alignment in byes.
 * @param size  Number of bytes to allocate.
 *
 * @return Allocated memory or NULL.
 *
 */
void *memalign(const size_t align, const size_t size)
{
	if (align == 0)
		return NULL;

	size_t palign =
	    1 << (fnzb(max(sizeof(void *), align) - 1) + 1);

	heap_lock();
	void *block = malloc_internal(size, palign);
	heap_unlock();

	return block;
}

/** Reallocate memory block
 *
 * @param addr Already allocated memory or NULL.
 * @param size New size of the memory block.
 *
 * @return Reallocated memory or NULL.
 *
 */
void *realloc(void *const addr, size_t size)
{
	if (size == 0) {
		fprintf(stderr, "realloc() called with size 0\n");
		size = 1;
	}

	if (addr == NULL)
		return malloc(size);

	heap_lock();

	/* Calculate the position of the header. */
	heap_block_head_t *head =
	    (heap_block_head_t *) (addr - sizeof(heap_block_head_t));

	block_check(head);
	malloc_assert(!head->free);

	heap_area_t *area = head->area;

	area_check(area);
	malloc_assert((void *) head >= (void *) AREA_FIRST_BLOCK_HEAD(area));
	malloc_assert((void *) head < area->end);

	void *ptr = NULL;
	bool reloc = false;
	size_t real_size = GROSS_SIZE(ALIGN_UP(size, BASE_ALIGN));
	size_t orig_size = head->size;

	if (orig_size > real_size) {
		/* Shrink */
		if (orig_size - real_size >= STRUCT_OVERHEAD) {
			/*
			 * Split the original block to a full block
			 * and a trailing free block.
			 */
			block_init((void *) head, real_size, false, area);
			block_init((void *) head + real_size,
			    orig_size - real_size, true, area);
			heap_shrink(area);
		}

		ptr = ((void *) head) + sizeof(heap_block_head_t);
	} else {
		heap_block_head_t *next_head =
		    (heap_block_head_t *) (((void *) head) + head->size);
		bool have_next = ((void *) next_head < area->end);

		if (((void *) head) + real_size > area->end) {
			/*
			 * The current area is too small to hold the resized
			 * block. Make sure there are no used blocks standing
			 * in our way and try to grow the area using real_size
			 * as a safe upper bound.
			 */

			bool have_next_next;

			if (have_next) {
				have_next_next = (((void *) next_head) +
				    next_head->size < area->end);
			}
			if (!have_next || (next_head->free && !have_next_next)) {
				/*
				 * There is no next block in this area or
				 * it is a free block and there is no used
				 * block following it. There can't be any
				 * free block following it either as
				 * two free blocks would be merged.
				 */
				(void) area_grow(area, real_size);
			}
		}

		/*
		 * Look at the next block. If it is free and the size is
		 * sufficient then merge the two. Otherwise just allocate a new
		 * block, copy the original data into it and free the original
		 * block.
		 */

		if (have_next && (head->size + next_head->size >= real_size) &&
		    next_head->free) {
			block_check(next_head);
			block_init(head, head->size + next_head->size, false,
			    area);
			split_mark(head, real_size);

			ptr = ((void *) head) + sizeof(heap_block_head_t);
			next_fit = NULL;
		} else {
			reloc = true;
		}
	}

	heap_unlock();

	if (reloc) {
		ptr = malloc(size);
		if (ptr != NULL) {
			memcpy(ptr, addr, NET_SIZE(orig_size));
			free(addr);
		}
	}

	return ptr;
}

/** Reallocate memory for an array
 *
 * Same as realloc(ptr, nelem * elsize), except the multiplication is checked
 * for numerical overflow. Borrowed from POSIX 2024.
 *
 * @param ptr
 * @param nelem
 * @param elsize
 *
 * @return Reallocated memory or NULL.
 */
void *reallocarray(void *ptr, size_t nelem, size_t elsize)
{
	if (nelem > SIZE_MAX / elsize)
		return NULL;

	return realloc(ptr, nelem * elsize);
}

/** Free a memory block
 *
 * @param addr The address of the block.
 *
 */
void free(void *const addr)
{
	if (addr == NULL)
		return;

	heap_lock();

	/* Calculate the position of the header. */
	heap_block_head_t *head =
	    (heap_block_head_t *) (addr - sizeof(heap_block_head_t));

	block_check(head);
	malloc_assert(!head->free);

	heap_area_t *area = head->area;

	area_check(area);
	malloc_assert((void *) head >= (void *) AREA_FIRST_BLOCK_HEAD(area));
	malloc_assert((void *) head < area->end);

	/* Mark the block itself as free. */
	head->free = true;

	/* Look at the next block. If it is free, merge the two. */
	heap_block_head_t *next_head =
	    (heap_block_head_t *) (((void *) head) + head->size);

	if ((void *) next_head < area->end) {
		block_check(next_head);
		if (next_head->free)
			block_init(head, head->size + next_head->size, true, area);
	}

	/* Look at the previous block. If it is free, merge the two. */
	if ((void *) head > (void *) AREA_FIRST_BLOCK_HEAD(area)) {
		heap_block_foot_t *prev_foot =
		    (heap_block_foot_t *) (((void *) head) - sizeof(heap_block_foot_t));

		heap_block_head_t *prev_head =
		    (heap_block_head_t *) (((void *) head) - prev_foot->size);

		block_check(prev_head);

		if (prev_head->free)
			block_init(prev_head, prev_head->size + head->size, true,
			    area);
	}

	heap_shrink(area);

	heap_unlock();
}

void *heap_check(void)
{
	heap_lock();

	if (first_heap_area == NULL) {
		heap_unlock();
		return (void *) -1;
	}

	/* Walk all heap areas */
	for (heap_area_t *area = first_heap_area; area != NULL;
	    area = area->next) {

		/* Check heap area consistency */
		if ((area->magic != HEAP_AREA_MAGIC) ||
		    ((void *) area != area->start) ||
		    (area->start >= area->end) ||
		    (((uintptr_t) area->start % PAGE_SIZE) != 0) ||
		    (((uintptr_t) area->end % PAGE_SIZE) != 0)) {
			heap_unlock();
			return (void *) area;
		}

		/* Walk all heap blocks */
		for (heap_block_head_t *head = (heap_block_head_t *)
		    AREA_FIRST_BLOCK_HEAD(area); (void *) head < area->end;
		    head = (heap_block_head_t *) (((void *) head) + head->size)) {

			/* Check heap block consistency */
			if (head->magic != HEAP_BLOCK_HEAD_MAGIC) {
				heap_unlock();
				return (void *) head;
			}

			heap_block_foot_t *foot = BLOCK_FOOT(head);

			if ((foot->magic != HEAP_BLOCK_FOOT_MAGIC) ||
			    (head->size != foot->size)) {
				heap_unlock();
				return (void *) foot;
			}
		}
	}

	heap_unlock();

	return NULL;
}

/** @}
 */
