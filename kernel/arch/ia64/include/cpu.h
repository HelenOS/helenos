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

#include <arch/types.h>
#include <typedefs.h>
#include <arch/register.h>

#define FAMILY_ITANIUM	0x7
#define FAMILY_ITANIUM2	0x1f

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
 */
static inline uint64_t cpuid_read(int n)
{
	uint64_t v;
	
	asm volatile ("mov %0 = cpuid[%1]\n" : "=r" (v) : "r" (n));
	
	return v;
}

#endif

/** @}
 */
