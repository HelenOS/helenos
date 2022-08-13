/*
 * SPDX-FileCopyrightText: 2001-2004 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
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
