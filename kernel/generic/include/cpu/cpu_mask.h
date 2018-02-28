/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup generic
 * @{
 */
/** @file
 */
#ifndef KERN_CPU_CPU_MASK_H_
#define KERN_CPU_CPU_MASK_H_

#include <cpu.h>
#include <config.h>
#include <lib/memfnc.h>

/** Iterates over all cpu id's whose bit is included in the cpu mask.
 *
 * Example usage:
 * @code
 * DEFINE_CPU_MASK(cpu_mask);
 * cpu_mask_active(&cpu_mask);
 *
 * cpu_mask_for_each(cpu_mask, cpu_id) {
 *     printf("Cpu with logical id %u is active.\n", cpu_id);
 * }
 * @endcode
 */
#define cpu_mask_for_each(mask, cpu_id) \
	for (unsigned int (cpu_id) = 0; (cpu_id) < config.cpu_count; ++(cpu_id)) \
		if (cpu_mask_is_set(&(mask), (cpu_id)))

/** Allocates a cpu_mask_t on stack. */
#define DEFINE_CPU_MASK(cpu_mask) \
	cpu_mask_t *(cpu_mask) = (cpu_mask_t*) alloca(cpu_mask_size())

/** If used with DEFINE_CPU_MASK, the mask is large enough for all detected cpus.*/
typedef struct cpu_mask {
	unsigned int mask[1];
} cpu_mask_t;


extern size_t cpu_mask_size(void);
extern void cpu_mask_active(cpu_mask_t *);
extern void cpu_mask_all(cpu_mask_t *);
extern void cpu_mask_none(cpu_mask_t *);
extern void cpu_mask_set(cpu_mask_t *, unsigned int);
extern void cpu_mask_reset(cpu_mask_t *, unsigned int);
extern bool cpu_mask_is_set(cpu_mask_t *, unsigned int);
extern bool cpu_mask_is_none(cpu_mask_t *);

#endif /* KERN_CPU_CPU_MASK_H_ */

/** @}
 */
