/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_CPU_H_
#define KERN_ia64_CPU_H_

#include <arch/register.h>
#include <arch/asm.h>
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
NO_TRACE static inline uint64_t cpuid_read(int n)
{
	uint64_t v;

	asm volatile (
		"mov %[v] = cpuid[%[r]]\n"
		: [v] "=r" (v)
		: [r] "r" (n)
	);

	return v;
}

NO_TRACE static inline int ia64_get_cpu_id(void)
{
	uint64_t cr64 = cr64_read();
	return ((CR64_ID_MASK) &cr64) >> CR64_ID_SHIFT;
}

NO_TRACE static inline int ia64_get_cpu_eid(void)
{
	uint64_t cr64 = cr64_read();
	return ((CR64_EID_MASK) &cr64) >> CR64_EID_SHIFT;
}

NO_TRACE static inline void ipi_send_ipi(int id, int eid, int intno)
{
	(bootinfo->sapic)[2 * (id * 256 + eid)] = intno;
	srlz_d();
}

#endif

/** @}
 */
