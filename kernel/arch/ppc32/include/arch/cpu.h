/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_CPU_H_
#define KERN_ppc32_CPU_H_

#include <stdint.h>
#include <trace.h>

typedef struct {
	uint16_t version;
	uint16_t revision;
} __attribute__((packed)) cpu_arch_t;

_NO_TRACE static inline void cpu_version(cpu_arch_t *info)
{
	asm volatile (
	    "mfpvr %[cpu_info]\n"
	    : [cpu_info] "=r" (*info)
	);
}

#endif

/** @}
 */
