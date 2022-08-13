/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_CPU_H_
#define KERN_amd64_CPU_H_

#define RFLAGS_CF	(1 << 0)
#define RFLAGS_PF	(1 << 2)
#define RFLAGS_AF	(1 << 4)
#define RFLAGS_ZF	(1 << 6)
#define RFLAGS_SF	(1 << 7)
#define RFLAGS_TF	(1 << 8)
#define RFLAGS_IF	(1 << 9)
#define RFLAGS_DF	(1 << 10)
#define RFLAGS_OF	(1 << 11)
#define RFLAGS_IOPL	(3 << 12)
#define RFLAGS_NT	(1 << 14)
#define RFLAGS_RF 	(1 << 16)
#define RFLAGS_ID	(1 << 21)

#define CR0_PE		(1 << 0)
#define CR0_MP		(1 << 1)
#define CR0_EM		(1 << 2)
#define CR0_TS		(1 << 3)
#define CR0_AM		(1 << 18)
#define CR0_PG		(1 << 31)

#define CR4_PAE		(1 << 5)
#define CR4_OSFXSR	(1 << 9)

/* EFER bits */
#define AMD_SCE		(1 << 0)
#define AMD_LME		(1 << 8)
#define AMD_LMA		(1 << 10)
#define AMD_NXE		(1 << 11)
#define AMD_FFXSR	(1 << 14)

#define AMD_APIC_BASE_GE	(1 << 11)

/* MSR registers */
#define AMD_MSR_APIC_BASE	0x0000001b
#define AMD_MSR_EFER		0xc0000080
#define AMD_MSR_STAR		0xc0000081
#define AMD_MSR_LSTAR		0xc0000082
#define AMD_MSR_SFMASK		0xc0000084
#define AMD_MSR_FS		0xc0000100
#define AMD_MSR_GS		0xc0000101
#define AMD_MSR_GS_KERNEL	0xc0000102

#ifndef __ASSEMBLER__

#include <arch/pm.h>

typedef struct {
	int vendor;
	int family;
	int model;
	int stepping;
	tss_t *tss;

	unsigned int id; /** CPU's local, ie physical, APIC ID. */

	size_t iomapver_copy;  /** Copy of TASK's I/O Permission bitmap generation count. */
} cpu_arch_t;

struct star_msr {
};

struct lstar_msr {
};

void cpu_setup_fpu(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
