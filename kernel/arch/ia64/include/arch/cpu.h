/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_CPU_H_
#define KERN_ia64_CPU_H_

#include <arch/register.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/bootinfo.h>
#include <stdint.h>
#include <trace.h>

#define FAMILY_ITANIUM   0x7
#define FAMILY_ITANIUM2  0x1f

#define CR64_ID_SHIFT   24
#define CR64_ID_MASK    0xff000000
#define CR64_EID_SHIFT  16
#define CR64_EID_MASK   0xff0000

typedef struct {
	uint64_t cpuid0;
	uint64_t cpuid1;
	cpuid3_t cpuid3;
} cpu_arch_t;

/** Read CPUID register.
 *
 * @param n CPUID register number.
 *
 * @return Value of CPUID[n] register.
 *
 */
_NO_TRACE static inline uint64_t cpuid_read(int n)
{
	uint64_t v;

	asm volatile (
	    "mov %[v] = cpuid[%[r]]\n"
	    : [v] "=r" (v)
	    : [r] "r" (n)
	);

	return v;
}

_NO_TRACE static inline int ia64_get_cpu_id(void)
{
	uint64_t cr64 = cr64_read();
	return ((CR64_ID_MASK) &cr64) >> CR64_ID_SHIFT;
}

_NO_TRACE static inline int ia64_get_cpu_eid(void)
{
	uint64_t cr64 = cr64_read();
	return ((CR64_EID_MASK) &cr64) >> CR64_EID_SHIFT;
}

_NO_TRACE static inline void ipi_send_ipi(int id, int eid, int intno)
{
	(bootinfo->sapic)[2 * (id * 256 + eid)] = intno;
	srlz_d();
}

#endif

/** @}
 */
