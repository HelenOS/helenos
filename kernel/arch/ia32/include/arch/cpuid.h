/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CPUID_H_
#define KERN_ia32_CPUID_H_

#define INTEL_CPUID_LEVEL     0x00000000
#define INTEL_CPUID_STANDARD  0x00000001
#define INTEL_PSE             3
#define INTEL_SEP             11

#ifndef __ASSEMBLER__

#include <arch/cpu.h>
#include <stdint.h>

typedef struct {
	uint32_t cpuid_eax;
	uint32_t cpuid_ebx;
	uint32_t cpuid_ecx;
	uint32_t cpuid_edx;
} __attribute__((packed)) cpu_info_t;

struct cpuid_extended_feature_info {
	unsigned int sse3 : 1;
	unsigned int : 31;
} __attribute__((packed));

typedef union {
	struct cpuid_extended_feature_info bits;
	uint32_t word;
} cpuid_extended_feature_info_t;

struct cpuid_feature_info {
	unsigned int : 11;
	unsigned int sep  : 1;
	unsigned int : 11;
	unsigned int mmx  : 1;
	unsigned int fxsr : 1;
	unsigned int sse  : 1;
	unsigned int sse2 : 1;
	unsigned int : 5;
} __attribute__((packed));

typedef union {
	struct cpuid_feature_info bits;
	uint32_t word;
} cpuid_feature_info_t;

static inline uint32_t has_cpuid(void)
{
	uint32_t val;
	uint32_t ret;

	asm volatile (
	    "pushf\n"                      /* read flags */
	    "popl %[ret]\n"
	    "movl %[ret], %[val]\n"

	    "xorl %[eflags_id], %[val]\n"  /* swap the ID bit */

	    "pushl %[val]\n"               /* propagate the change into flags */
	    "popf\n"
	    "pushf\n"
	    "popl %[val]\n"

	    "andl %[eflags_id], %[ret]\n"  /* interrested only in ID bit */
	    "andl %[eflags_id], %[val]\n"
	    "xorl %[val], %[ret]\n"
	    : [ret] "=r" (ret), [val] "=r" (val)
	    : [eflags_id] "i" (EFLAGS_ID)
	);

	return ret;
}

static inline void cpuid(uint32_t cmd, cpu_info_t *info)
{
	asm volatile (
	    "cpuid\n"
	    : "=a" (info->cpuid_eax), "=b" (info->cpuid_ebx),
	      "=c" (info->cpuid_ecx), "=d" (info->cpuid_edx)
	    : "a" (cmd)
	);
}

#endif /* !def __ASSEMBLER__ */
#endif

/** @}
 */
