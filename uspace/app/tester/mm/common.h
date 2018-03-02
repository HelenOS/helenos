/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup tester
 * @{
 */
/** @file
 */

#ifndef MM_COMMON_H_
#define MM_COMMON_H_

#include <stddef.h>
#include <stdbool.h>
#include <adt/list.h>

#define MAX_ALLOC  (16 * 1024 * 1024)

#define AREA_GRANULARITY  16
#define AREA_SIZE         (4 * PAGE_SIZE)

typedef struct {
	/* Address of the start of the block */
	void *addr;

	/* Size of the memory block */
	size_t size;

	/* Link to other blocks */
	link_t link;
} mem_block_t;

typedef struct {
	/* Address of the start of the area */
	void *addr;

	/* Size of the memory area */
	size_t size;
	/* Link to other areas */
	link_t link;
} mem_area_t;

/*
 * Subphase control structures: subphase termination conditions,
 * probabilities of individual actions, subphase control structure.
 */

typedef struct {
	unsigned int max_cycles;
	unsigned int no_memory;
	unsigned int no_allocated;
} sp_term_cond_t;

typedef struct {
	unsigned int alloc;
	unsigned int free;
} sp_action_prob_t;

typedef struct {
	const char *name;
	sp_term_cond_t cond;
	sp_action_prob_t prob;
} subphase_t;

/*
 * Phase control structures: The minimum and maximum block size that
 * can be allocated during the phase execution, phase control structure.
 */

typedef struct {
	size_t min_block_size;
	size_t max_block_size;
} ph_alloc_size_t;

typedef struct {
	const char *name;
	ph_alloc_size_t alloc;
	subphase_t *subphases;
} phase_t;

extern bool error_flag;
extern size_t mem_allocated;
extern size_t mem_blocks_count;

#define RETURN_IF_ERROR \
	do { \
		if (error_flag) \
			return; \
	} while (0)

extern void init_mem(void);
extern void done_mem(void);

extern mem_block_t *alloc_block(size_t);
extern void free_block(mem_block_t *);
extern void fill_block(mem_block_t *);
extern void check_block(mem_block_t *);
extern mem_block_t *get_random_block(void);

extern mem_area_t *map_area(size_t);
extern void fill_area(mem_area_t *);
extern void unmap_area(mem_area_t *);

#endif

/** @}
 */
