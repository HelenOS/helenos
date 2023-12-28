/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2009 Tomas Bures
 * Copyright (c) 2009 Lubomir Bulej
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

#include <stdlib.h>
#include <as.h>
#include <adt/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include "../tester.h"
#include "common.h"

/*
 * Global error flag. The flag is set if an error
 * is encountered (overlapping blocks, inconsistent
 * block data, etc.)
 */
bool error_flag = false;

/*
 * Memory accounting: the amount of allocated memory and the
 * number and list of allocated blocks.
 */
size_t mem_allocated;
size_t mem_blocks_count;

static LIST_INITIALIZE(mem_blocks);
static LIST_INITIALIZE(mem_areas);

/** Initializes the memory accounting structures.
 *
 */
void init_mem(void)
{
	mem_allocated = 0;
	mem_blocks_count = 0;
}

/** Cleanup all allocated memory blocks and mapped areas.
 *
 * Set the global error_flag if an error occurs.
 *
 */
void done_mem(void)
{
	link_t *link;

	while ((link = list_first(&mem_blocks)) != NULL) {
		mem_block_t *block = list_get_instance(link, mem_block_t, link);
		free_block(block);
	}

	while ((link = list_first(&mem_areas)) != NULL) {
		mem_area_t *area = list_get_instance(link, mem_area_t, link);
		unmap_area(area);
	}
}

static bool overlap_match(mem_block_t *block, void *addr, size_t size)
{
	/* Entry block control structure <mbeg, mend) */
	uint8_t *mbeg = (uint8_t *) block;
	uint8_t *mend = (uint8_t *) block + sizeof(mem_block_t);

	/* Entry block memory <bbeg, bend) */
	uint8_t *bbeg = (uint8_t *) block->addr;
	uint8_t *bend = (uint8_t *) block->addr + block->size;

	/* Data block <dbeg, dend) */
	uint8_t *dbeg = (uint8_t *) addr;
	uint8_t *dend = (uint8_t *) addr + size;

	/* Check for overlaps */
	if (((mbeg >= dbeg) && (mbeg < dend)) ||
	    ((mend > dbeg) && (mend <= dend)) ||
	    ((bbeg >= dbeg) && (bbeg < dend)) ||
	    ((bend > dbeg) && (bend <= dend)))
		return true;

	return false;
}

/** Test overlap
 *
 * Test whether a block starting at @addr overlaps with another,
 * previously allocated memory block or its control structure.
 *
 * @param addr Initial address of the block
 * @param size Size of the block
 *
 * @return False if the block does not overlap.
 *
 */
static int test_overlap(void *addr, size_t size)
{
	bool fnd = false;

	list_foreach(mem_blocks, link, mem_block_t, block) {
		if (overlap_match(block, addr, size)) {
			fnd = true;
			break;
		}
	}

	return fnd;
}

static void check_consistency(const char *loc)
{
	/* Check heap consistency */
	void *prob = heap_check();
	if (prob != NULL) {
		TPRINTF("\nError: Heap inconsistency at %p in %s.\n",
		    prob, loc);
		TSTACKTRACE();
		error_flag = true;
	}
}

/** Checked malloc
 *
 * Allocate @size bytes of memory and check whether the chunk comes
 * from the non-mapped memory region and whether the chunk overlaps
 * with other, previously allocated, chunks.
 *
 * @param size Amount of memory to allocate
 *
 * @return NULL if the allocation failed. Sets the global error_flag to
 *         true if the allocation succeeded but is illegal.
 *
 */
static void *checked_malloc(size_t size)
{
	void *data;

	/* Allocate the chunk of memory */
	data = malloc(size);
	check_consistency("checked_malloc");
	if (data == NULL)
		return NULL;

	/* Check for overlaps with other chunks */
	if (test_overlap(data, size)) {
		TPRINTF("\nError: Allocated block overlaps with another "
		    "previously allocated block.\n");
		TSTACKTRACE();
		error_flag = true;
	}

	return data;
}

/** Allocate block
 *
 * Allocate a block of memory of @size bytes and add record about it into
 * the mem_blocks list. Return a pointer to the block holder structure or
 * NULL if the allocation failed.
 *
 * If the allocation is illegal (e.g. the memory does not come from the
 * right region or some of the allocated blocks overlap with others),
 * set the global error_flag.
 *
 * @param size Size of the memory block
 *
 */
mem_block_t *alloc_block(size_t size)
{
	/* Check for allocation limit */
	if (mem_allocated >= MAX_ALLOC)
		return NULL;

	/* Allocate the block holder */
	mem_block_t *block =
	    (mem_block_t *) checked_malloc(sizeof(mem_block_t));
	if (block == NULL)
		return NULL;

	link_initialize(&block->link);

	/* Allocate the block memory */
	block->addr = checked_malloc(size);
	if (block->addr == NULL) {
		free(block);
		check_consistency("alloc_block");
		return NULL;
	}

	block->size = size;

	/* Register the allocated block */
	list_append(&block->link, &mem_blocks);
	mem_allocated += size + sizeof(mem_block_t);
	mem_blocks_count++;

	return block;
}

/** Free block
 *
 * Free the block of memory and the block control structure allocated by
 * alloc_block. Set the global error_flag if an error occurs.
 *
 * @param block Block control structure
 *
 */
void free_block(mem_block_t *block)
{
	/* Unregister the block */
	list_remove(&block->link);
	mem_allocated -= block->size + sizeof(mem_block_t);
	mem_blocks_count--;

	/* Free the memory */
	free(block->addr);
	check_consistency("free_block (a)");
	free(block);
	check_consistency("free_block (b)");
}

/** Calculate expected value
 *
 * Compute the expected value of a byte located at @pos in memory
 * block described by @block.
 *
 * @param block Memory block control structure
 * @param pos   Position in the memory block data area
 *
 */
static inline uint8_t block_expected_value(mem_block_t *block, uint8_t *pos)
{
	return ((uintptr_t) block ^ (uintptr_t) pos) & 0xff;
}

/** Fill block
 *
 * Fill the memory block controlled by @block with data.
 *
 * @param block Memory block control structure
 *
 */
void fill_block(mem_block_t *block)
{
	for (uint8_t *pos = block->addr, *end = pos + block->size;
	    pos < end; pos++)
		*pos = block_expected_value(block, pos);

	check_consistency("fill_block");
}

/** Check block
 *
 * Check whether the block @block contains the data it was filled with.
 * Set global error_flag if an error occurs.
 *
 * @param block Memory block control structure
 *
 */
void check_block(mem_block_t *block)
{
	for (uint8_t *pos = block->addr, *end = pos + block->size;
	    pos < end; pos++) {
		if (*pos != block_expected_value(block, pos)) {
			TPRINTF("\nError: Corrupted content of a data block.\n");
			TSTACKTRACE();
			error_flag = true;
			return;
		}
	}
}

/** Get random block
 *
 * Select a random memory block from the list of allocated blocks.
 *
 * @return Block control structure or NULL if the list is empty.
 *
 */
mem_block_t *get_random_block(void)
{
	if (mem_blocks_count == 0)
		return NULL;

	unsigned long idx = rand() % mem_blocks_count;
	link_t *entry = list_nth(&mem_blocks, idx);

	if (entry == NULL) {
		TPRINTF("\nError: Corrupted list of allocated memory blocks.\n");
		TSTACKTRACE();
		error_flag = true;
	}

	return list_get_instance(entry, mem_block_t, link);
}

/** Map memory area
 *
 * Map a memory area of @size bytes and add record about it into
 * the mem_areas list. Return a pointer to the area holder structure or
 * NULL if the mapping failed.
 *
 * @param size Size of the memory area
 *
 */
mem_area_t *map_area(size_t size)
{
	/* Allocate the area holder */
	mem_area_t *area =
	    (mem_area_t *) checked_malloc(sizeof(mem_area_t));
	if (area == NULL)
		return NULL;

	link_initialize(&area->link);

	area->addr = as_area_create(AS_AREA_ANY, size,
	    AS_AREA_WRITE | AS_AREA_READ | AS_AREA_CACHEABLE,
	    AS_AREA_UNPAGED);
	if (area->addr == AS_MAP_FAILED) {
		free(area);
		check_consistency("map_area (a)");
		return NULL;
	}

	area->size = size;

	/* Register the allocated area */
	list_append(&area->link, &mem_areas);

	return area;
}

/** Unmap area
 *
 * Unmap the memory area and free the block control structure.
 * Set the global error_flag if an error occurs.
 *
 * @param area Memory area control structure
 *
 */
void unmap_area(mem_area_t *area)
{
	/* Unregister the area */
	list_remove(&area->link);

	/* Free the memory */
	errno_t ret = as_area_destroy(area->addr);
	if (ret != EOK)
		error_flag = true;

	free(area);
	check_consistency("unmap_area");
}

/** Calculate expected value
 *
 * Compute the expected value of a byte located at @pos in memory
 * area described by @area.
 *
 * @param area Memory area control structure
 * @param pos  Position in the memory area data area
 *
 */
static inline uint8_t area_expected_value(mem_area_t *area, uint8_t *pos)
{
	return ((uintptr_t) area ^ (uintptr_t) pos) & 0xaa;
}

/** Fill area
 *
 * Fill the memory area controlled by @area with data.
 *
 * @param area Memory area control structure
 *
 */
void fill_area(mem_area_t *area)
{
	for (uint8_t *pos = area->addr, *end = pos + area->size;
	    pos < end; pos++)
		*pos = area_expected_value(area, pos);

	check_consistency("fill_area");
}
