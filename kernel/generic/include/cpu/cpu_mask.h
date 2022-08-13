/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
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

/** If used with DEFINE_CPU_MASK, the mask is large enough for all detected cpus. */
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
