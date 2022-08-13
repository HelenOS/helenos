/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CPU_H_
#define KERN_ia32_CPU_H_

#define EFLAGS_IF	(1 << 9)
#define EFLAGS_DF	(1 << 10)
#define EFLAGS_IOPL	(3 << 12)
#define EFLAGS_NT	(1 << 14)
#define EFLAGS_RF	(1 << 16)
#define EFLAGS_ID	(1 << 21)

#define CR0_PE		(1 << 0)
#define CR0_TS		(1 << 3)
#define CR0_AM		(1 << 18)
#define CR0_NW		(1 << 29)
#define CR0_CD		(1 << 30)
#define CR0_PG		(1 << 31)

#define CR4_PSE		(1 << 4)
#define CR4_PAE		(1 << 5)
#define CR4_OSFXSR	(1 << 9)
#define CR4_OSXMMEXCPT	(1 << 10)

#define IA32_APIC_BASE_GE	(1 << 11)

#define IA32_MSR_APIC_BASE	0x01b

/* Support for SYSENTER and SYSEXIT */
#define IA32_MSR_SYSENTER_CS	0x174
#define IA32_MSR_SYSENTER_ESP	0x175
#define IA32_MSR_SYSENTER_EIP	0x176

#ifndef __ASSEMBLER__

#include <arch/pm.h>
#include <arch/asm.h>
#include <arch/cpuid.h>

typedef struct {
	unsigned int vendor;
	unsigned int family;
	unsigned int model;
	unsigned int stepping;
	cpuid_feature_info_t fi;

	unsigned int id; /** CPU's local, ie physical, APIC ID. */

	tss_t *tss;

	size_t iomapver_copy;  /** Copy of TASK's I/O Permission bitmap generation count. */
} cpu_arch_t;

#endif

#endif

/** @}
 */
