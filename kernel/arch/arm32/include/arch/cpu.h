/*
 * Copyright (c) 2007 Michal Kebrt
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
