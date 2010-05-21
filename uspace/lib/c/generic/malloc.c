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

/* Magic used in heap headers. */
#define HEAP_BLOCK_HEAD_MAGIC  0xBEEF0101

/* Magic used in heap footers. */
#define HEAP_BLOCK_FOOT_MAGIC  0xBEEF0202

/** Allocation alignment (this also covers the alignment of fields
    in the heap header and footer) */
#define BASE_ALIGN  16

/**
 * Either 4 * 256M on 32-bit architecures or 16 * 256M on 64-bit architectures
 */
#define MAX_HEAP_SIZE  (sizeof(uintptr_t) << 28)

/**
 *
 */
#define STRUCT_OVERHEAD  (sizeof(heap_block_head_t) + sizeof(heap_block_foot_t))

/**
 * Calculate real size of a heap block (with header and footer)
 */
#define GROSS_SIZE(size)  ((size) + STRUCT_OVERHEAD)

/**
 * Calculate net size of a heap block (without header and footer)
 */
#define NET_SIZE(size)  ((size) - STRUCT_OVERHEAD)

/** Header of a heap block
 *
 */
typedef struct {
	/* Size of the block (including header and footer) */
	size_t size;
	
	/* Indication of a free block */
	bool free;
	
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

/** Linker heap symbol */
extern char _heap;

/** Futex for thread-safe heap manipulation */
static futex_t malloc_futex = FUTEX_INITIALIZER;

/** Address of heap start */
static void *heap_start = 0;

/** Address of heap end */
static void *heap_end = 0;

/** Maximum heap size */
static size_t max_heap_size = (size_t) -1;

/** Current number of pages of heap area */
static size_t heap_pages = 0;

/** Initialize a heap block
 *
 * Fills in the structures related to a heap block.
 * Should be called only inside the critical section.
 *
 * @param addr Address of the block.
 * @param size Size of the block including the header and the footer.
 * @param free Indication of a free block.
 *
 */
static void block_init(void *addr, size_t size, bool free)
{
	/* Calculate the position of the header and the footer */
	heap_block_head_t *head = (heap_block_head_t *) addr;
	heap_block_foot_t *foot =
	    (heap_block_foot_t *) (addr + size - sizeof(heap_block_foot_t));
	
	head->size = size;
	head->free = free;
	head->magic = HEAP_BLOCK_HEAD_MAGIC;
	
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
	
	heap_block_foot_t *foot =
	    (heap_block_foot_t *) (addr + head->size - sizeof(heap_block_foot_t));
	
	assert(foot->magic == HEAP_BLOCK_FOOT_MAGIC);
	assert(head->size == foot->size);
}

/** Increase the heap area size
 *
 * Should be called only inside the critical section.
 *
 * @param size Number of bytes to grow the heap by.
 *
 */
static bool grow_heap(size_t size)
{
	if (size == 0)
		return false;

	if ((heap_start + size < heap_start) || (heap_end + size < heap_end))
		return false;
	
	size_t heap_size = (size_t) (heap_end - heap_start);
	
	if ((max_heap_size != (size_t) -1) && (heap_size + size > max_heap_size))
		return false;
	
	size_t pages = (size - 1) / PAGE_SIZE + 1;
	
	if (as_area_resize((void *) &_heap, (heap_pages + pages) * PAGE_SIZE, 0)
	    == EOK) {
		void *end = (void *) ALIGN_DOWN(((uintptr_t) &_heap) +
		    (heap_pages + pages) * PAGE_SIZE, BASE_ALIGN);
		block_init(heap_end, end - heap_end, true);
		heap_pages += pages;
		heap_end = end;
		return true;
	}
	
	return false;
}

/** Decrease the heap area
 *
 * Should be called only inside the critical section.
 *
 * @param size Number of bytes to shrink the heap by.
 *
 */
static void shrink_heap(void)
{
	// TODO
}

/** Initialize the heap allocator
 *
 * Find how much physical memory we have and create
 * the heap management structures that mark the whole
 * physical memory as a single free block.
 *
 */
void __heap_init(void)
{
	futex_down(&malloc_futex);
	
	if (as_area_create((void *) &_heap, PAGE_SIZE,
	    AS_AREA_WRITE | AS_AREA_READ)) {
		heap_pages = 1;
		heap_start = (void *) ALIGN_UP((uintptr_t) &_heap, BASE_ALIGN);
		heap_end =
		    (void *) ALIGN_DOWN(((uintptr_t) &_heap) + PAGE_SIZE, BASE_ALIGN);
		
		/* Make the entire area one large block. */
		block_init(heap_start, heap_end - heap_start, true);
	}
	
	futex_up(&malloc_futex);
}

/** Get maximum heap address
 *
 */
uintptr_t get_max_heap_addr(void)
{
	futex_down(&malloc_futex);
	
	if (max_heap_size == (size_t) -1)
		max_heap_size =
		    max((size_t) (heap_end - heap_start), MAX_HEAP_SIZE);
	
	uintptr_t max_heap_addr = (uintptr_t) heap_start + max_heap_size;
	
	futex_up(&malloc_futex);
	
	return max_heap_addr;
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
		block_init(next, cur->size - size, true);
		block_init(cur, size, false);
	} else {
		/* Block too small -> use as is. */
		cur->free = false;
	}
}

/** Allocate a memory block
 *
 * Should be called only inside the critical section.
 *
 * @param size  The size of the block to allocate.
 * @param align Memory address alignment.
 *
 * @return the address of the block or NULL when not enough memory.
 *
 */
static void *malloc_internal(const size_t size, const size_t align)
{
	if (align == 0)
		return NULL;
	
	size_t falign = lcm(align, BASE_ALIGN);
	size_t real_size = GROSS_SIZE(ALIGN_UP(size, falign));
	
	bool grown = false;
	void *result;
	
loop:
	result = NULL;
	heap_block_head_t *cur = (heap_block_head_t *) heap_start;
	
	while ((result == NULL) && ((void *) cur < heap_end)) {
		block_check(cur);
		
		/* Try to find a block that is free and large enough. */
		if ((cur->free) && (cur->size >= real_size)) {
			/* We have found a suitable block.
			   Check for alignment properties. */
			void *addr = ((void *) cur) + sizeof(heap_block_head_t);
			void *aligned = (void *) ALIGN_UP(addr, falign);
			
			if (addr == aligned) {
				/* Exact block start including alignment. */
				split_mark(cur, real_size);
				result = addr;
			} else {
				/* Block start has to be aligned */
				size_t excess = (size_t) (aligned - addr);
				
				if (cur->size >= real_size + excess) {
					/* The current block is large enough to fit
					   data in including alignment */
					if ((void *) cur > heap_start) {
						/* There is a block before the current block.
						   This previous block can be enlarged to compensate
						   for the alignment excess */
						heap_block_foot_t *prev_foot =
						    ((void *) cur) - sizeof(heap_block_foot_t);
						
						heap_block_head_t *prev_head =
						    (heap_block_head_t *) (((void *) cur) - prev_foot->size);
						
						block_check(prev_head);
						
						size_t reduced_size = cur->size - excess;
						heap_block_head_t *next_head = ((void *) cur) + excess;
						
						if ((!prev_head->free) && (excess >= STRUCT_OVERHEAD)) {
							/* The previous block is not free and there is enough
							   space to fill in a new free block between the previous
							   and current block */
							block_init(cur, excess, true);
						} else {
							/* The previous block is free (thus there is no need to
							   induce additional fragmentation to the heap) or the
							   excess is small, thus just enlarge the previous block */
							block_init(prev_head, prev_head->size + excess, prev_head->free);
						}
						
						block_init(next_head, reduced_size, true);
						split_mark(next_head, real_size);
						result = aligned;
						cur = next_head;
					} else {
						/* The current block is the first block on the heap.
						   We have to make sure that the alignment excess
						   is large enough to fit a new free block just
						   before the current block */
						while (excess < STRUCT_OVERHEAD) {
							aligned += falign;
							excess += falign;
						}
						
						/* Check for current block size again */
						if (cur->size >= real_size + excess) {
							size_t reduced_size = cur->size - excess;
							cur = (heap_block_head_t *) (heap_start + excess);
							
							block_init(heap_start, excess, true);
							block_init(cur, reduced_size, true);
							split_mark(cur, real_size);
							result = aligned;
						}
					}
				}
			}
		}
		
		/* Advance to the next block. */
		cur = (heap_block_head_t *) (((void *) cur) + cur->size);
	}
	
	if ((result == NULL) && (!grown)) {
		if (grow_heap(real_size)) {
			grown = true;
			goto loop;
		}
	}
	
	return result;
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
	
	assert((void *) head >= heap_start);
	assert((void *) head < heap_end);
	
	block_check(head);
	assert(!head->free);
	
	void *ptr = NULL;
	bool reloc = false;
	size_t real_size = GROSS_SIZE(ALIGN_UP(size, BASE_ALIGN));
	size_t orig_size = head->size;
	
	if (orig_size > real_size) {
		/* Shrink */
		if (orig_size - real_size >= STRUCT_OVERHEAD) {
			/* Split the original block to a full block
			   and a trailing free block */
			block_init((void *) head, real_size, false);
			block_init((void *) head + real_size,
			    orig_size - real_size, true);
			shrink_heap();
		}
		
		ptr = ((void *) head) + sizeof(heap_block_head_t);
	} else {
		/* Look at the next block. If it is free and the size is
		   sufficient then merge the two. Otherwise just allocate
		   a new block, copy the original data into it and
		   free the original block. */
		heap_block_head_t *next_head =
		    (heap_block_head_t *) (((void *) head) + head->size);
		
		if (((void *) next_head < heap_end) &&
		    (head->size + next_head->size >= real_size) &&
		    (next_head->free)) {
			block_check(next_head);
			block_init(head, head->size + next_head->size, false);
			split_mark(head, real_size);
			
			ptr = ((void *) head) + sizeof(heap_block_head_t);
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
	
	assert((void *) head >= heap_start);
	assert((void *) head < heap_end);
	
	block_check(head);
	assert(!head->free);
	
	/* Mark the block itself as free. */
	head->free = true;
	
	/* Look at the next block. If it is free, merge the two. */
	heap_block_head_t *next_head
	    = (heap_block_head_t *) (((void *) head) + head->size);
	
	if ((void *) next_head < heap_end) {
		block_check(next_head);
		if (next_head->free)
			block_init(head, head->size + next_head->size, true);
	}
	
	/* Look at the previous block. If it is free, merge the two. */
	if ((void *) head > heap_start) {
		heap_block_foot_t *prev_foot =
		    (heap_block_foot_t *) (((void *) head) - sizeof(heap_block_foot_t));
		
		heap_block_head_t *prev_head =
		    (heap_block_head_t *) (((void *) head) - prev_foot->size);
		
		block_check(prev_head);
		
		if (prev_head->free)
			block_init(prev_head, prev_head->size + head->size, true);
	}
	
	shrink_heap();
	
	futex_up(&malloc_futex);
}

/** @}
 */
