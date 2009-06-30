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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include "../tester.h"

/*
 * The test consists of several phases which differ in the size of blocks
 * they allocate. The size of blocks is given as a range of minimum and
 * maximum allowed size. Each of the phases is divided into 3 subphases which
 * differ in the probability of free and alloc actions. Second subphase is
 * started when malloc returns 'out of memory' or when MAX_ALLOC is reached.
 * Third subphase is started after a given number of cycles. The third subphase
 * as well as the whole phase ends when all memory blocks are released.
 */

/**
 * sizeof_array
 * @array array to determine the size of
 *
 * Returns the size of @array in array elements.
 */
#define sizeof_array(array) \
	(sizeof(array) / sizeof((array)[0]))

#define MAX_ALLOC (16 * 1024 * 1024)

/*
 * Subphase control structures: subphase termination conditions,
 * probabilities of individual actions, subphase control structure.
 */

typedef struct {
	unsigned int max_cycles;
	unsigned int no_memory;
	unsigned int no_allocated;
} sp_term_cond_s;

typedef struct {
	unsigned int alloc;
	unsigned int free;
} sp_action_prob_s;

typedef struct {
	char *name;
	sp_term_cond_s cond;
	sp_action_prob_s prob;
} subphase_s;


/*
 * Phase control structures: The minimum and maximum block size that
 * can be allocated during the phase execution, phase control structure.
 */

typedef struct {
	size_t min_block_size;
	size_t max_block_size;
} ph_alloc_size_s;

typedef struct {
	char *name;
	ph_alloc_size_s alloc;
	subphase_s *subphases;
} phase_s;


/*
 * Subphases are defined separately here. This is for two reasons:
 * 1) data are not duplicated, 2) we don't have to state beforehand
 * how many subphases a phase contains.
 */
static subphase_s subphases_32B[] = {
	{
		.name = "Allocation",
		.cond = {
			.max_cycles = 200,
			.no_memory = 1,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 90,
			.free = 100
		}
	},
	{
		.name = "Alloc/Dealloc",
		.cond = {
			.max_cycles = 200,
			.no_memory = 0,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 50,
			.free = 100
		}
	},
	{
		.name = "Deallocation",
		.cond = {
			.max_cycles = 0,
			.no_memory = 0,
			.no_allocated = 1,
		},
		.prob = {
			.alloc = 10,
			.free = 100
		}
	}
};

static subphase_s subphases_128K[] = {
	{
		.name = "Allocation",
		.cond = {
			.max_cycles = 0,
			.no_memory = 1,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 70,
			.free = 100
		}
	},
	{
		.name = "Alloc/Dealloc",
		.cond = {
			.max_cycles = 30,
			.no_memory = 0,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 50,
			.free = 100
		}
	},
	{
		.name = "Deallocation",
		.cond = {
			.max_cycles = 0,
			.no_memory = 0,
			.no_allocated = 1,
		},
		.prob = {
			.alloc = 30,
			.free = 100
		}
	}
};

static subphase_s subphases_default[] = {
	{
		.name = "Allocation",
		.cond = {
			.max_cycles = 0,
			.no_memory = 1,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 90,
			.free = 100
		}
	},
	{
		.name = "Alloc/Dealloc",
		.cond = {
			.max_cycles = 200,
			.no_memory = 0,
			.no_allocated = 0,
		},
		.prob = {
			.alloc = 50,
			.free = 100
		}
	},
	{
		.name = "Deallocation",
		.cond = {
			.max_cycles = 0,
			.no_memory = 0,
			.no_allocated = 1,
		},
		.prob = {
			.alloc = 10,
			.free = 100
		}
	}
};


/*
 * Phase definitions.
 */
static phase_s phases[] = {
	{
		.name = "32 B memory blocks",
		.alloc = {
			.min_block_size = 32,
			.max_block_size = 32
		},
		.subphases = subphases_32B
	},
	{
		.name = "128 KB memory blocks",
		.alloc = {
			.min_block_size = 128 * 1024,
			.max_block_size = 128 * 1024
		},
		.subphases = subphases_128K
	},
	{
		.name = "2500 B memory blocks",
		.alloc = {
			.min_block_size = 2500,
			.max_block_size = 2500
		},
		.subphases = subphases_default
	},
	{
		.name = "1 B .. 250000 B memory blocks",
		.alloc = {
			.min_block_size = 1,
			.max_block_size = 250000
		},
		.subphases = subphases_default
	}
};


/*
 * Global error flag. The flag is set if an error
 * is encountered (overlapping blocks, inconsistent
 * block data, etc.)
 */
static bool error_flag = false;

/*
 * Memory accounting: the amount of allocated memory and the
 * number and list of allocated blocks.
 */
static size_t mem_allocated;
static size_t mem_blocks_count;

static LIST_INITIALIZE(mem_blocks);

typedef struct {
	/* Address of the start of the block */
	void *addr;
	
	/* Size of the memory block */
	size_t size;
	
	/* link to other blocks */
	link_t link;
} mem_block_s;

typedef mem_block_s *mem_block_t;


/** init_mem
 *
 * Initializes the memory accounting structures.
 *
 */
static void init_mem(void)
{
	mem_allocated = 0;
	mem_blocks_count = 0;
}


static bool overlap_match(link_t *entry, void *addr, size_t size)
{
	mem_block_t mblk = list_get_instance(entry, mem_block_s, link);
	
	/* Entry block control structure <mbeg, mend) */
	uint8_t *mbeg = (uint8_t *) mblk;
	uint8_t *mend = (uint8_t *) mblk + sizeof(mem_block_s);
	
	/* Entry block memory <bbeg, bend) */
	uint8_t *bbeg = (uint8_t *) mblk->addr;
	uint8_t *bend = (uint8_t *) mblk->addr + mblk->size;
	
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


/** test_overlap
 *
 * Test whether a block starting at @addr overlaps with another, previously
 * allocated memory block or its control structure.
 *
 * @param addr Initial address of the block
 * @param size Size of the block
 *
 * @return false if the block does not overlap.
 *
 */
static int test_overlap(void *addr, size_t size)
{
	link_t *entry;
	bool fnd = false;
	
	for (entry = mem_blocks.next; entry != &mem_blocks; entry = entry->next) {
		if (overlap_match(entry, addr, size)) {
			fnd = true;
			break;
		}
	}
	
	return fnd;
}


/** checked_malloc
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
	if (data == NULL)
		return NULL;
	
	/* Check for overlaps with other chunks */
	if (test_overlap(data, size)) {
		TPRINTF("\nError: Allocated block overlaps with another "
			"previously allocated block.\n");
		error_flag = true;
	}
	
	return data;
}


/** alloc_block
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
static mem_block_t alloc_block(size_t size)
{
	/* Check for allocation limit */
	if (mem_allocated >= MAX_ALLOC)
		return NULL;
	
	/* Allocate the block holder */
	mem_block_t block = (mem_block_t) checked_malloc(sizeof(mem_block_s));
	if (block == NULL)
		return NULL;
	
	link_initialize(&block->link);
	
	/* Allocate the block memory */
	block->addr = checked_malloc(size);
	if (block->addr == NULL) {
		free(block);
		return NULL;
	}
	
	block->size = size;
	
	/* Register the allocated block */
	list_append(&block->link, &mem_blocks);
	mem_allocated += size + sizeof(mem_block_s);
	mem_blocks_count++;
	
	return block;
}


/** free_block
 *
 * Free the block of memory and the block control structure allocated by
 * alloc_block. Set the global error_flag if an error occurs.
 *
 * @param block Block control structure
 *
 */
static void free_block(mem_block_t block)
{
	/* Unregister the block */
	list_remove(&block->link);
	mem_allocated -= block->size + sizeof(mem_block_s);
	mem_blocks_count--;
	
	/* Free the memory */
	free(block->addr);
	free(block);
}


/** expected_value
 *
 * Compute the expected value of a byte located at @pos in memory
 * block described by @blk.
 *
 * @param blk Memory block control structure
 * @param pos Position in the memory block data area
 *
 */
static inline uint8_t expected_value(mem_block_t blk, uint8_t *pos)
{
	return ((unsigned long) blk ^ (unsigned long) pos) & 0xff;
}


/** fill_block
 *
 * Fill the memory block controlled by @blk with data.
 *
 * @param blk Memory block control structure
 *
 */
static void fill_block(mem_block_t blk)
{
	uint8_t *pos;
	uint8_t *end;
	
	for (pos = blk->addr, end = pos + blk->size; pos < end; pos++)
		*pos = expected_value(blk, pos);
}


/** check_block
 *
 * Check whether the block @blk contains the data it was filled with.
 * Set global error_flag if an error occurs.
 *
 * @param blk Memory block control structure
 *
 */
static void check_block(mem_block_t blk)
{
	uint8_t *pos;
	uint8_t *end;
	
	for (pos = blk->addr, end = pos + blk->size; pos < end; pos++) {
		if (*pos != expected_value (blk, pos)) {
			TPRINTF("\nError: Corrupted content of a data block.\n");
			error_flag = true;
			return;
		}
	}
}


static link_t *list_get_nth(link_t *list, unsigned int i)
{
	unsigned int cnt = 0;
	link_t *entry;
	
	for (entry = list->next; entry != list; entry = entry->next) {
		if (cnt == i)
			return entry;
		
		cnt++;
	}
	
	return NULL;
}


/** get_random_block
 *
 * Select a random memory block from the list of allocated blocks.
 *
 * @return Block control structure or NULL if the list is empty.
 *
 */
static mem_block_t get_random_block(void)
{
	if (mem_blocks_count == 0)
		return NULL;
	
	unsigned int blkidx = rand() % mem_blocks_count;
	link_t *entry = list_get_nth(&mem_blocks, blkidx);
	
	if (entry == NULL) {
		TPRINTF("\nError: Corrupted list of allocated memory blocks.\n");
		error_flag = true;
	}
	
	return list_get_instance(entry, mem_block_s, link);
}


#define RETURN_IF_ERROR \
{ \
	if (error_flag) \
		return; \
}


static void do_subphase(phase_s *phase, subphase_s *subphase)
{
	unsigned int cycles;
	for (cycles = 0; /* always */; cycles++) {
		
		if (subphase->cond.max_cycles &&
			cycles >= subphase->cond.max_cycles) {
			/*
			 * We have performed the required number of
			 * cycles. End the current subphase.
			 */
			break;
		}
		
		/*
		 * Decide whether we alloc or free memory in this step.
		 */
		unsigned int rnd = rand() % 100;
		if (rnd < subphase->prob.alloc) {
			/* Compute a random number lying in interval <min_block_size, max_block_size> */
			int alloc = phase->alloc.min_block_size +
			    (rand() % (phase->alloc.max_block_size - phase->alloc.min_block_size + 1));
			
			mem_block_t blk = alloc_block(alloc);
			RETURN_IF_ERROR;
			
			if (blk == NULL) {
				TPRINTF("F(A)");
				if (subphase->cond.no_memory) {
					/* We filled the memory. Proceed to next subphase */
					break;
				}
				
			} else {
				TPRINTF("A");
				fill_block(blk);
			}
			
		} else if (rnd < subphase->prob.free) {
			mem_block_t blk = get_random_block();
			if (blk == NULL) {
				TPRINTF("F(R)");
				if (subphase->cond.no_allocated) {
					/* We free all the memory. Proceed to next subphase. */
					break;
				}
				
			} else {
				TPRINTF("R");
				check_block(blk);
				RETURN_IF_ERROR;
				
				free_block(blk);
				RETURN_IF_ERROR;
			}
		}
	}
	
	TPRINTF("\n..  finished.\n");
}


static void do_phase(phase_s *phase)
{
	unsigned int subno;
	
	for (subno = 0; subno < 3; subno++) {
		subphase_s *subphase = & phase->subphases [subno];
		
		TPRINTF(".. Sub-phase %u (%s)\n", subno + 1, subphase->name);
		do_subphase(phase, subphase);
		RETURN_IF_ERROR;
	}
}

char *test_malloc1(void)
{
	init_mem();
	
	unsigned int phaseno;
	for (phaseno = 0; phaseno < sizeof_array(phases); phaseno++) {
		phase_s *phase = &phases[phaseno];
		
		TPRINTF("Entering phase %u (%s)\n", phaseno + 1, phase->name);
		
		do_phase(phase);
		if (error_flag)
			break;
		
		TPRINTF("Phase finished.\n");
	}
	
	if (error_flag)
		return "Test failed";
	
	return NULL;
}
