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
