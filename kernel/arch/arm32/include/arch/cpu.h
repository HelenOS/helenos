/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief CPU identification.
 */

#ifndef KERN_arm32_CPU_H_
#define KERN_arm32_CPU_H_

#include <arch/asm.h>
#include <stdint.h>

enum {
	ARM_MAX_CACHE_LEVELS = 7,
};

/** Struct representing ARM CPU identification. */
typedef struct {
	/** Implementor (vendor) number. */
	uint32_t imp_num;

	/** Variant number. */
	uint32_t variant_num;

	/** Architecture number. */
	uint32_t arch_num;

	/** Primary part number. */
	uint32_t prim_part_num;

	/** Revision number. */
	uint32_t rev_num;

	struct {
		unsigned ways;
		unsigned sets;
		unsigned line_size;
		unsigned way_shift;
		unsigned set_shift;
	} dcache[ARM_MAX_CACHE_LEVELS];
	unsigned dcache_levels;
} cpu_arch_t;

#endif

/** @}
 */
