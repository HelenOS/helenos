/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_CPU_H_
#define KERN_sparc64_sun4v_CPU_H_

/** Maximum number of virtual processors. */
#define MAX_NUM_STRANDS  64

/** Maximum number of logical processors in a processor core */
#define MAX_CORE_STRANDS  8

#ifndef __ASSEMBLER__

#include <atomic.h>
#include <synch/spinlock.h>

struct cpu;

typedef struct {
	uint64_t exec_unit_id;
	uint8_t strand_count;
	uint64_t cpuids[MAX_CORE_STRANDS];
	struct cpu *cpus[MAX_CORE_STRANDS];
	atomic_size_t nrdy;
	SPINLOCK_DECLARE(proposed_nrdy_lock);
} exec_unit_t;

typedef struct cpu_arch {
	/** Virtual processor ID */
	uint64_t id;
	/** Processor frequency in Hz */
	uint32_t clock_frequency;
	/** Next clock interrupt should be generated when the TICK register
	 * matches this value.
	 */
	uint64_t next_tick_cmpr;
	/** Physical core. */
	exec_unit_t *exec_unit;
	/** Proposed No. of ready threads so that cores are equally balanced. */
	unsigned long proposed_nrdy;
} cpu_arch_t;

#endif

#endif

/** @}
 */
