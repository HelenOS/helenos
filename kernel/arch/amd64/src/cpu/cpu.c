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

#include <cpu.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/pm.h>

#include <arch.h>
#include <stdio.h>
#include <fpu_context.h>

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
	"AuthenticAMD",
	"GenuineIntel"
};

/** Setup flags on processor so that we can use the FPU
 *
 * cr0.osfxsr = 1 -> we do support fxstor/fxrestor
 * cr0.em = 0 -> we do not emulate coprocessor
 * cr0.mp = 1 -> we do want lazy context switch
 */
void cpu_setup_fpu(void)
{
	write_cr0((read_cr0() & ~CR0_EM) | CR0_MP);
	write_cr4(read_cr4() | CR4_OSFXSR);
}

/** Set the TS flag to 1.
 *
 * If a thread accesses coprocessor, exception is run, which
 * does a lazy fpu context switch.
 *
 */
void fpu_disable(void)
{
	write_cr0(read_cr0() | CR0_TS);
}

void fpu_enable(void)
{
	write_cr0(read_cr0() & ~CR0_TS);
}

void cpu_arch_init(void)
{
	CPU->arch.tss = tss_p;
	CPU->arch.tss->iomap_base = &CPU->arch.tss->iomap[0] -
	    ((uint8_t *) CPU->arch.tss);
	CPU->fpu_owner = NULL;
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
		if ((info.cpuid_ebx == AMD_CPUID_EBX) &&
		    (info.cpuid_ecx == AMD_CPUID_ECX) &&
		    (info.cpuid_edx == AMD_CPUID_EDX)) {
			CPU->arch.vendor = VendorAMD;
		}

		/*
		 * Check for Intel processor.
		 */
		if ((info.cpuid_ebx == INTEL_CPUID_EBX) &&
		    (info.cpuid_ecx == INTEL_CPUID_ECX) &&
		    (info.cpuid_edx == INTEL_CPUID_EDX)) {
			CPU->arch.vendor = VendorIntel;
		}

		cpuid(INTEL_CPUID_STANDARD, &info);
		CPU->arch.family = (info.cpuid_eax >> 8) & 0xf;
		CPU->arch.model = (info.cpuid_eax >> 4) & 0xf;
		CPU->arch.stepping = (info.cpuid_eax >> 0) & 0xf;
	}
}

void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: (%s family=%d model=%d stepping=%d apicid=%u) %dMHz\n",
	    m->id, vendor_str[m->arch.vendor], m->arch.family, m->arch.model,
	    m->arch.stepping, m->arch.id, m->frequency_mhz);
}

/** @}
 */
