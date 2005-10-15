/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __CPUID_H__
#define __CPUID_H__

#include <arch/types.h>

struct cpu_info {
	__u32 cpuid_eax;
	__u32 cpuid_ebx;
	__u32 cpuid_ecx;
	__u32 cpuid_edx;
} __attribute__ ((packed));

static inline __u32 has_cpuid(void)
{
	__u32 val, ret;
	
	__asm__ volatile (
		"pushf\n"               /* read flags */
		"popl %0\n"
		"movl %0, %1\n"
		
		"btcl $21, %1\n"        /* swap the ID bit */
		
		"pushl %1\n"            /* propagate the change into flags */
		"popf\n"
		"pushf\n"
		"popl %1\n"
		
		"andl $(1 << 21), %0\n" /* interrested only in ID bit */
		"andl $(1 << 21), %1\n"
		"xorl %1, %0\n"
		: "=r" (ret), "=r" (val)
	);
	
	return ret;
}

static inline void cpuid(__u32 cmd, struct cpu_info *info)
{
	__asm__ volatile (
		"movl %4, %%eax\n"
		"cpuid\n"
		
		"movl %%eax, %0\n"
		"movl %%ebx, %1\n"
		"movl %%ecx, %2\n"
		"movl %%edx, %3\n"
		: "=m" (info->cpuid_eax), "=m" (info->cpuid_ebx), "=m" (info->cpuid_ecx), "=m" (info->cpuid_edx)
		: "m" (cmd)
		: "eax", "ebx", "ecx", "edx"
	);
}

#endif
