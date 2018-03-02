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

#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/pm.h>

#include <arch.h>
#include <stdint.h>
#include <print.h>
#include <fpu_context.h>

#include <arch/smp/apic.h>
#include <arch/syscall.h>

/*
 * Identification of CPUs.
 * Contains only non-MP-Specification specific SMP code.
 */
#define AMD_CPUID_EBX  UINT32_C(0x68747541)
#define AMD_CPUID_ECX  UINT32_C(0x444d4163)
#define AMD_CPUID_EDX  UINT32_C(0x69746e65)

#define INTEL_CPUID_EBX  UINT32_C(0x756e6547)
#define INTEL_CPUID_ECX  UINT32_C(0x6c65746e)
#define INTEL_CPUID_EDX  UINT32_C(0x49656e69)


enum vendor {
	VendorUnknown = 0,
	VendorAMD,
	VendorIntel
};

static const char *vendor_str[] = {
	"Unknown Vendor",
	"AMD",
	"Intel"
};

void fpu_disable(void)
{
	write_cr0(read_cr0() & ~CR0_TS);
}

void fpu_enable(void)
{
	write_cr0(read_cr0() | CR0_TS);
}

void cpu_arch_init(void)
{
	cpu_info_t info;
	uint32_t help = 0;

	CPU->arch.tss = tss_p;
	CPU->arch.tss->iomap_base = &CPU->arch.tss->iomap[0] - ((uint8_t *) CPU->arch.tss);

	CPU->fpu_owner = NULL;

	cpuid(INTEL_CPUID_STANDARD, &info);

	CPU->arch.fi.word = info.cpuid_edx;

	if (CPU->arch.fi.bits.fxsr)
		fpu_fxsr();
	else
		fpu_fsr();

	if (CPU->arch.fi.bits.sse) {
		asm volatile (
			"mov %%cr4, %[help]\n"
			"or %[mask], %[help]\n"
			"mov %[help], %%cr4\n"
			: [help] "+r" (help)
			: [mask] "i" (CR4_OSFXSR | CR4_OSXMMEXCPT)
		);
	}

#ifndef PROCESSOR_i486
	if (CPU->arch.fi.bits.sep) {
		/* Setup fast SYSENTER/SYSEXIT syscalls */
		syscall_setup_cpu();
	}
#endif
}

void cpu_identify(void)
{
	cpu_info_t info;

	CPU->arch.vendor = VendorUnknown;
	if (has_cpuid()) {
		cpuid(INTEL_CPUID_LEVEL, &info);

		/*
		 * Check for AMD processor.
		 */
		if ((info.cpuid_ebx == AMD_CPUID_EBX)
		    && (info.cpuid_ecx == AMD_CPUID_ECX)
		    && (info.cpuid_edx == AMD_CPUID_EDX))
			CPU->arch.vendor = VendorAMD;

		/*
		 * Check for Intel processor.
		 */
		if ((info.cpuid_ebx == INTEL_CPUID_EBX)
		    && (info.cpuid_ecx == INTEL_CPUID_ECX)
		    && (info.cpuid_edx == INTEL_CPUID_EDX))
			CPU->arch.vendor = VendorIntel;

		cpuid(INTEL_CPUID_STANDARD, &info);
		CPU->arch.family = (info.cpuid_eax >> 8) & 0x0fU;
		CPU->arch.model = (info.cpuid_eax >> 4) & 0x0fU;
		CPU->arch.stepping = (info.cpuid_eax >> 0) & 0x0fU;
	}
}

void cpu_print_report(cpu_t* cpu)
{
	printf("cpu%u: (%s family=%u model=%u stepping=%u apicid=%u) %" PRIu16
		" MHz\n", cpu->id, vendor_str[cpu->arch.vendor], cpu->arch.family,
		cpu->arch.model, cpu->arch.stepping, cpu->arch.id, cpu->frequency_mhz);
}

/** @}
 */
