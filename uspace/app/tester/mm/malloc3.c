/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 * SPDX-FileCopyrightText: 2009 Tomas Bures
 * SPDX-FileCopyrightText: 2009 Lubomir Bulej
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <libarch/config.h>
#include "common.h"
#include "../tester.h"

/*
 * The test is a slight adaptation of malloc1 test. The major difference
 * is that the test forces the heap allocator to create multiple
 * heap areas by creating disturbing address space areas.
 */

static subphase_t subphases_32B[] = {
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

static subphase_t subphases_128K[] = {
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

static subphase_t subphases_default[] = {
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
static phase_t phases[] = {
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

static void do_subphase(phase_t *phase, subphase_t *subphase)
{
	for (unsigned int cycles = 0; /* always */; cycles++) {

		if ((subphase->cond.max_cycles) &&
		    (cycles >= subphase->cond.max_cycles)) {
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
			/*
			 * Compute a random number lying in interval
			 * <min_block_size, max_block_size>
			 */
			int alloc = phase->alloc.min_block_size +
			    (rand() % (phase->alloc.max_block_size - phase->alloc.min_block_size + 1));

			mem_block_t *blk = alloc_block(alloc);
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
				RETURN_IF_ERROR;

				if ((mem_blocks_count % AREA_GRANULARITY) == 0) {
					mem_area_t *area = map_area(AREA_SIZE);
					RETURN_IF_ERROR;

					if (area != NULL) {
						TPRINTF("*");
						fill_area(area);
						RETURN_IF_ERROR;
					} else
						TPRINTF("F(*)");
				}
			}

		} else if (rnd < subphase->prob.free) {
			mem_block_t *blk = get_random_block();
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

static void do_phase(phase_t *phase)
{
	for (unsigned int subno = 0; subno < 3; subno++) {
		subphase_t *subphase = &phase->subphases[subno];

		TPRINTF(".. Sub-phase %u (%s)\n", subno + 1, subphase->name);
		do_subphase(phase, subphase);
		RETURN_IF_ERROR;
	}
}

const char *test_malloc3(void)
{
	init_mem();

	for (unsigned int phaseno = 0; phaseno < sizeof_array(phases);
	    phaseno++) {
		phase_t *phase = &phases[phaseno];

		TPRINTF("Entering phase %u (%s)\n", phaseno + 1, phase->name);

		do_phase(phase);
		if (error_flag)
			break;

		TPRINTF("Phase finished.\n");
	}

	TPRINTF("Cleaning up.\n");
	done_mem();
	if (error_flag)
		return "Test failed";

	return NULL;
}
