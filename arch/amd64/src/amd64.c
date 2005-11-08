/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <arch.h>

#include <arch/types.h>

#include <config.h>

#include <arch/ega.h>
#include <arch/i8042.h>
#include <arch/i8254.h>
#include <arch/i8259.h>

#include <arch/bios/bios.h>
#include <arch/mm/memory_init.h>
#include <arch/cpu.h>
#include <print.h>
#include <arch/cpuid.h>
#include <genarch/acpi/acpi.h>
#include <panic.h>

void arch_pre_mm_init(void)
{
	struct cpu_info cpuid_s;

	cpuid(AMD_CPUID_EXTENDED,&cpuid_s);
	if (! (cpuid_s.cpuid_edx & (1<<AMD_EXT_NOEXECUTE)))
		panic("Processor does not support No-execute pages.\n");

	cpuid(INTEL_CPUID_STANDARD,&cpuid_s);
	if (! (cpuid_s.cpuid_edx & (1<<INTEL_FXSAVE)))
		panic("Processor does not support FXSAVE/FXRESTORE.\n");
	
	if (! (cpuid_s.cpuid_edx & (1<<INTEL_SSE2)))
		panic("Processor does not support SSE2 instructions.\n");

	/* Enable No-execute pages */
	set_efer_flag(AMD_NXE_FLAG);
	/* Enable FPU */
	cpu_setup_fpu();

	pm_init();

	if (config.cpu_active == 1) {
		bios_init();
		i8042_init();	/* a20 bit */
		i8259_init();	/* PIC */
		i8254_init();	/* hard clock */

		trap_register(VECTOR_SYSCALL, syscall);
		
		#ifdef CONFIG_SMP
		trap_register(VECTOR_TLB_SHOOTDOWN_IPI, tlb_shootdown_ipi);
		trap_register(VECTOR_WAKEUP_IPI, wakeup_ipi);
		#endif /* CONFIG_SMP */
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		ega_init();	/* video */
	}
}

void arch_late_init(void)
{
	if (config.cpu_active == 1) {
		memory_print_map();
		
		#ifdef CONFIG_SMP
		acpi_init();
		#endif /* CONFIG_SMP */
	}
}

void calibrate_delay_loop(void)
{
	i8254_calibrate_delay_loop();
	i8254_normal_operation();
}
