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

#include <malloc.h>
#include <bool.h>
#include <as.h>
#include <align.h>
#include <macros.h>
#include <assert.h>
#include <errno.h>
#include <bitops.h>
#include <mem.h>
#include <futex.h>
#include <adt/gcdlcm.h>
#include "private/malloc.h"

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

/** Get first block in heap area.
 *
 */
#define AREA_FIRST_BLOCK(area) \
	(ALIGN_UP(((uintptr_t) (area)) + sizeof(heap_area_t), BASE_ALIGN))

/** Get footer in heap block.
 *
 */
#define BLOCK_FOOT(head) \
	((heap_block_foot_t *) \
	    (((uintptr_t) head) + head->size - sizeof(heap_block_foot_t)))

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
static heap_block_head_t *next = NULL;

/** Futex for thread-safe heap manipulation */
static futex_t malloc_futex = FUTEX_INITIALIZER;

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
	
	assert(head->magic == HEAP_BLOCK_HEAD_MAGIC);
	
	heap_block_foot_t *foot = BLOCK_FOOT(head);
	
	assert(foot->magic == HEAP_BLOCK_FOOT_MAGIC);
	assert(head->size == foot->size);
}

/** Check a heap area structure
 *
 * @param addr Address of the heap area.
 *
 */
static void area_check(void *addr)
{
	heap_area_t *area = (heap_area_t *) addr;
	
	assert(area->magic == HEAP_AREA_MAGIC);
	assert(area->start < area->end);
	assert(((uintptr_t) area->start % PAGE_SIZE) == 0);
	assert(((uintptr_t) area->end % PAGE_SIZE) == 0);
}

/** Create new heap area
 *
 * @param start Preffered starting address of the new area.
 * @param size  Size of the area.
 *
 */
static bool area_create(size_t size)
{
	void *start = as_get_mappable_page(size);
	if (start == NULL)
		return false;
	
	/* Align the heap area on page boundary */
	void *astart = (void *) ALIGN_UP((uintptr_t) start, PAGE_SIZE);
	size_t asize = ALIGN_UP(size, PAGE_SIZE);
	
	astart = as_area_create(astart, asize, AS_AREA_WRITE | AS_AREA_READ);
	if (astart == (void *) -1)
		return false;
	
	heap_area_t *area = (heap_area_t *) astart;
	
	area->start = astart;
	area->end = (void *)
	    ALIGN_DOWN((uintptr_t) astart + asize, BASE_ALIGN);
	area->next = NULL;
	area->magic = HEAP_AREA_MAGIC;
	
	void *block = (void *) AREA_FIRST_BLOCK(area);
	size_t bsize = (size_t) (area->end - block);
	
	block_init(block, bsize, true, area);
	
	if (last_heap_area == NULL) {
		first_heap_area = area;
		last_heap_area = area;
	} else {
		last_heap_area->next = area;
		last_heap_area = area;
	}
	
	return true;
}

/** Try to enlarge a heap area
 *
 * @param area Heap area to grow.
 * @param size Gross size of item to allocate (bytes).
 *
 */
static bool area_grow(heap_area_t *area, size_t size)
{
	if (size == 0)
		return true;
	
	area_check(area);
	
	size_t asize = ALIGN_UP((size_t) (area->end - area->start) + size,
	    PAGE_SIZE);
	
	/* New heap area size */
	void *end = (void *)
	    ALIGN_DOWN((uintptr_t) area->start + asize, BASE_ALIGN);
	
	/* Check for overflow */
	if (end < area->start)
		return false;
	
	/* Resize the address space area */
	int ret = as_area_resize(area->start, asize, 0);
	if (ret != EOK)
		return false;
	
	/* Add new free block */
	block_init(area->end, (size_t) (end - area->end), true, area);
	
	/* Update heap area parameters */
	area->end = end;
	
	return true;
}

/** Try to enlarge any of the heap areas
 *
 * @param size Gross size of item to allocate (bytes).
 *
 */
static bool heap_grow(size_t size)
{
	if (size == 0)
		return true;
	
	/* First try to enlarge some existing area */
	heap_area_t *area;
	for (area = first_heap_area; area != NULL; area = area->next) {
		if (area_grow(area, size))
			return true;
	}
	
	/* Eventually try to create a new area */
	return area_create(AREA_FIRST_BLOCK(size));
}

/** Try to shrink heap space
 *
 * In all cases the next pointer is reset.
 *
 */
static void heap_shrink(void)
{
	next = NULL;
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
	if (!area_create(PAGE_SIZE))
		abort();
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
	assert(cur->size >= size);
	
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
	assert((void *) first_block >= (void *) AREA_FIRST_BLOCK(area));
	assert((void *) first_block < area->end);
	
	heap_block_head_t *cur;
	for (cur = first_block; (void *) cur < area->end;
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
				
				next = cur;
				return addr;
			} else {
				/* Block start has to be aligned */
				size_t excess = (size_t) (aligned - addr);
				
				if (cur->size >= real_size + excess) {
					/*
					 * The current block is large enough to fit
					 * data in (including alignment).
					 */
					if ((void *) cur > (void *) AREA_FIRST_BLOCK(area)) {
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
						
						next = next_head;
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
							    (AREA_FIRST_BLOCK(area) + excess);
							
							block_init((void *) AREA_FIRST_BLOCK(area), excess,
							    true, area);
							block_init(cur, reduced_size, true, area);
							split_mark(cur, real_size);
							
							next = cur;
							return aligned;
						}
					}
				}
			}
		}
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
static void *malloc_internal(const size_t size, const size_t align)
{
	assert(first_heap_area != NULL);
	
	if (align == 0)
		return NULL;
	
	size_t falign = lcm(align, BASE_ALIGN);
	size_t real_size = GROSS_SIZE(ALIGN_UP(size, falign));
	
	bool retry = false;
	heap_block_head_t *split;
	
loop:
	
	/* Try the next fit approach */
	split = next;
	
	if (split != NULL) {
		void *addr = malloc_area(split->area, split, NULL, real_size,
		    falign);
		
		if (addr != NULL)
			return addr;
	}
	
	/* Search the entire heap */
	heap_area_t *area;
	for (area = first_heap_area; area != NULL; area = area->next) {
		heap_block_head_t *first = (heap_block_head_t *)
		    AREA_FIRST_BLOCK(area);
		
		void *addr = malloc_area(area, first, split, real_size,
		    falign);
		
		if (addr != NULL)
			return addr;
	}
	
	if (!retry) {
		/* Try to grow the heap space */
		if (heap_grow(real_size)) {
			retry = true;
			goto loop;
		}
	}
	
	return NULL;
}

/** Allocate memory by number of elements
 *
 * @param nmemb Number of members to allocate.
 * @param size  Size of one member in bytes.
 *
 * @return Allocated memory or NULL.
 *
 */
void *calloc(const size_t nmemb, const size_t size)
{
	void *block = malloc(nmemb * size);
	if (block == NULL)
		return NULL;
	
	memset(block, 0, nmemb * size);
	return block;
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
	futex_down(&malloc_futex);
	void *block = malloc_internal(size, BASE_ALIGN);
	futex_up(&malloc_futex);
	
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
	
	futex_down(&malloc_futex);
	void *block = malloc_internal(size, palign);
	futex_up(&malloc_futex);
	
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
void *realloc(const void *addr, const size_t size)
{
	if (addr == NULL)
		return malloc(size);
	
	futex_down(&malloc_futex);
	
	/* Calculate the position of the header. */
	heap_block_head_t *head =
	    (heap_block_head_t *) (addr - sizeof(heap_block_head_t));
	
	block_check(head);
	assert(!head->free);
	
	heap_area_t *area = head->area;
	
	area_check(area);
	assert((void *) head >= (void *) AREA_FIRST_BLOCK(area));
	assert((void *) head < area->end);
	
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
			heap_shrink();
		}
		
		ptr = ((void *) head) + sizeof(heap_block_head_t);
	} else {
		/*
		 * Look at the next block. If it is free and the size is
		 * sufficient then merge the two. Otherwise just allocate
		 * a new block, copy the original data into it and
		 * free the original block.
		 */
		heap_block_head_t *next_head =
		    (heap_block_head_t *) (((void *) head) + head->size);
		
		if (((void *) next_head < area->end) &&
		    (head->size + next_head->size >= real_size) &&
		    (next_head->free)) {
			block_check(next_head);
			block_init(head, head->size + next_head->size, false, area);
			split_mark(head, real_size);
			
			ptr = ((void *) head) + sizeof(heap_block_head_t);
			next = NULL;
		} else
			reloc = true;
	}
	
	futex_up(&malloc_futex);
	
	if (reloc) {
		ptr = malloc(size);
		if (ptr != NULL) {
			memcpy(ptr, addr, NET_SIZE(orig_size));
			free(addr);
		}
	}
	
	return ptr;
}

/** Free a memory block
 *
 * @param addr The address of the block.
 *
 */
void free(const void *addr)
{
	futex_down(&malloc_futex);
	
	/* Calculate the position of the header. */
	heap_block_head_t *head
	    = (heap_block_head_t *) (addr - sizeof(heap_block_head_t));
	
	block_check(head);
	assert(!head->free);
	
	heap_area_t *area = head->area;
	
	area_check(area);
	assert((void *) head >= (void *) AREA_FIRST_BLOCK(area));
	assert((void *) head < area->end);
	
	/* Mark the block itself as free. */
	head->free = true;
	
	/* Look at the next block. If it is free, merge the two. */
	heap_block_head_t *next_head
	    = (heap_block_head_t *) (((void *) head) + head->size);
	
	if ((void *) next_head < area->end) {
		block_check(next_head);
		if (next_head->free)
			block_init(head, head->size + next_head->size, true, area);
	}
	
	/* Look at the previous block. If it is free, merge the two. */
	if ((void *) head > (void *) AREA_FIRST_BLOCK(area)) {
		heap_block_foot_t *prev_foot =
		    (heap_block_foot_t *) (((void *) head) - sizeof(heap_block_foot_t));
		
		heap_block_head_t *prev_head =
		    (heap_block_head_t *) (((void *) head) - prev_foot->size);
		
		block_check(prev_head);
		
		if (prev_head->free)
			block_init(prev_head, prev_head->size + head->size, true,
			    area);
	}
	
	heap_shrink();
	
	futex_up(&malloc_futex);
}

/** @}
 */
