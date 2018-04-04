/*
 * Copyright (c) 2001-2004 Ondrej Palkovsky
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

/** @addtogroup amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_CPUID_H_
#define KERN_amd64_CPUID_H_

#define AMD_CPUID_EXTENDED  0x80000001
#define AMD_EXT_NOEXECUTE   20
#define AMD_EXT_LONG_MODE   29

#define INTEL_CPUID_LEVEL     0x00000000
#define INTEL_CPUID_STANDARD  0x00000001
#define INTEL_CPUID_EXTENDED  0x80000000
#define INTEL_SSE2            26
#define INTEL_FXSAVE          24

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct {
	uint32_t cpuid_eax;
	uint32_t cpuid_ebx;
	uint32_t cpuid_ecx;
	uint32_t cpuid_edx;
} __attribute__((packed)) cpu_info_t;

extern int has_cpuid(void);

extern void cpuid(uint32_t cmd, cpu_info_t *info);

#endif /* !def __ASSEMBLER__ */
#endif

/** @}
 */
