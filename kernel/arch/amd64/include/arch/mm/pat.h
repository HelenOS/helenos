/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2024 Jiří Zárevúcky
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

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_MM_PAT_H_
#define KERN_amd64_MM_PAT_H_

#include <arch/asm.h>
#include <arch/cpuid.h>

#define MSR_IA32_PAT  0x00000277

typedef enum {
	PAT_TYPE_UNCACHEABLE = 0,
	PAT_TYPE_WRITE_COMBINING = 1,
	PAT_TYPE_WRITE_THROUGH = 4,
	PAT_TYPE_WRITE_PROTECTED = 5,
	PAT_TYPE_WRITE_BACK = 6,
	PAT_TYPE_UNCACHED  = 7,
} pat_type_t;

#ifndef PROCESSOR_i486
/**
 * Assign caching type for a particular combination of PAT,
 * PCD and PWT bits in PTE.
 */
static inline void pat_set_mapping(bool pat, bool pcd, bool pwt,
    pat_type_t type)
{
	int index = pat << 2 | pcd << 1 | pwt;
	int shift = index * 8;

	uint64_t r = read_msr(MSR_IA32_PAT);
	r &= ~(0xffull << shift);
	r |= ((uint64_t) type) << shift;
	write_msr(MSR_IA32_PAT, r);
}
#endif

static inline bool pat_supported(void)
{
	if (!has_cpuid())
		return false;

	cpu_info_t info;
	cpuid(INTEL_CPUID_STANDARD, &info);

	return (info.cpuid_edx & (1 << 16)) != 0;
}

#endif

/** @}
 */
