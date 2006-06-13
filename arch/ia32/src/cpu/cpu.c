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

 /** @addtogroup ia32cpu ia32
 * @ingroup cpu
 * @{
 */
/** @file
 */

#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/pm.h>

#include <arch.h>
#include <arch/types.h>
#include <print.h>
#include <typedefs.h>
#include <fpu_context.h>

#include <arch/smp/apic.h>

/*
 * Identification of CPUs.
 * Contains only non-MP-Specification specific SMP code.
 */
#define AMD_CPUID_EBX	0x68747541
#define AMD_CPUID_ECX 	0x444d4163
#define AMD_CPUID_EDX 	0x69746e65

#define INTEL_CPUID_EBX	0x756e6547
#define INTEL_CPUID_ECX 0x6c65746e
#define INTEL_CPUID_EDX 0x49656e69


enum vendor {
	VendorUnknown=0,
	VendorAMD,
	VendorIntel
};

static char *vendor_str[] = {
	"Unknown Vendor",
	"AuthenticAMD",
	"GenuineIntel"
};

void fpu_disable(void)
{
	__asm__ volatile (
		"mov %%cr0,%%eax;"
		"or $8,%%eax;"
		"mov %%eax,%%cr0;"
		:
		:
		:"%eax"
	);
}

void fpu_enable(void)
{
	__asm__ volatile (
		"mov %%cr0,%%eax;"
		"and $0xffFFffF7,%%eax;"
		"mov %%eax,%%cr0;"
		:
		:
		:"%eax"
	);	
}

void cpu_arch_init(void)
{
	cpuid_feature_info fi;
	cpuid_extended_feature_info efi;
	cpu_info_t info;
	__u32 help = 0;
	
	CPU->arch.tss = tss_p;
	CPU->arch.tss->iomap_base = &CPU->arch.tss->iomap[0] - ((__u8 *) CPU->arch.tss);

	CPU->fpu_owner = NULL;

	cpuid(1, &info);

	fi.word = info.cpuid_edx;
	efi.word = info.cpuid_ecx;
	
	if (fi.bits.fxsr)
		fpu_fxsr();
	else
		fpu_fsr();	
	
	if (fi.bits.sse) {
		asm volatile (
			"mov %%cr4,%0\n"
			"or %1,%0\n"
			"mov %0,%%cr4\n"
			: "+r" (help)
			: "i" (CR4_OSFXSR_MASK|(1<<10)) 
		);
	}
}

void cpu_identify(void)
{
	cpu_info_t info;

	CPU->arch.vendor = VendorUnknown;
	if (has_cpuid()) {
		cpuid(0, &info);

		/*
		 * Check for AMD processor.
		 */
		if (info.cpuid_ebx==AMD_CPUID_EBX && info.cpuid_ecx==AMD_CPUID_ECX && info.cpuid_edx==AMD_CPUID_EDX) {
			CPU->arch.vendor = VendorAMD;
		}

		/*
		 * Check for Intel processor.
		 */		
		if (info.cpuid_ebx==INTEL_CPUID_EBX && info.cpuid_ecx==INTEL_CPUID_ECX && info.cpuid_edx==INTEL_CPUID_EDX) {
			CPU->arch.vendor = VendorIntel;
		}
				
		cpuid(1, &info);
		CPU->arch.family = (info.cpuid_eax>>8)&0xf;
		CPU->arch.model = (info.cpuid_eax>>4)&0xf;
		CPU->arch.stepping = (info.cpuid_eax>>0)&0xf;						
	}
}

void cpu_print_report(cpu_t* m)
{
	printf("cpu%d: (%s family=%d model=%d stepping=%d) %dMHz\n",
		m->id, vendor_str[m->arch.vendor], m->arch.family, m->arch.model, m->arch.stepping,
		m->frequency_mhz);
}

 /** @}
 */

