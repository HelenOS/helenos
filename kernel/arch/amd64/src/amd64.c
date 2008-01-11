/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

#include <arch.h>

#include <arch/types.h>

#include <config.h>

#include <proc/thread.h>
#include <arch/drivers/ega.h>
#include <arch/drivers/vesa.h>
#include <genarch/kbd/i8042.h>
#include <arch/drivers/i8254.h>
#include <arch/drivers/i8259.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#endif

#include <arch/bios/bios.h>
#include <arch/mm/memory_init.h>
#include <arch/cpu.h>
#include <print.h>
#include <arch/cpuid.h>
#include <genarch/acpi/acpi.h>
#include <panic.h>
#include <interrupt.h>
#include <arch/syscall.h>
#include <arch/debugger.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <ddi/irq.h>
#include <ddi/device.h>


/** Disable I/O on non-privileged levels
 *
 * Clean IOPL(12,13) and NT(14) flags in EFLAGS register
 */
static void clean_IOPL_NT_flags(void)
{
	asm
	(
		"pushfq;"
		"pop %%rax;"
		"and $~(0x7000),%%rax;"
		"pushq %%rax;"
		"popfq;"
		:
		:
		:"%rax"
	);
}

/** Disable alignment check
 *
 * Clean AM(18) flag in CR0 register 
 */
static void clean_AM_flag(void)
{
	asm
	(
		"mov %%cr0,%%rax;"
		"and $~(0x40000),%%rax;"
		"mov %%rax,%%cr0;"
		:
		:
		:"%rax"
	);
}

void arch_pre_mm_init(void)
{
	/* Enable no-execute pages */
	set_efer_flag(AMD_NXE_FLAG);
	/* Enable FPU */
	cpu_setup_fpu();

	/* Initialize segmentation */
	pm_init();
	
	/* Disable I/O on nonprivileged levels
	 * clear the NT (nested-thread) flag 
	 */
	clean_IOPL_NT_flags();
	/* Disable alignment check */
	clean_AM_flag();

	if (config.cpu_active == 1) {
		interrupt_init();
		bios_init();
		
		/* PIC */
		i8259_init();
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
		
		/* hard clock */
		i8254_init();

#ifdef CONFIG_FB
		if (vesa_present()) 
			vesa_init();
		else
#endif
			ega_init();	/* video */
		
		/* Enable debugger */
		debugger_init();
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
	/* Setup fast SYSCALL/SYSRET */
	syscall_setup_cpu();
	
}

void arch_post_cpu_init()
{
#ifdef CONFIG_SMP
	if (config.cpu_active > 1) {
		l_apic_init();
		l_apic_debug();
	}
#endif
}

void arch_pre_smp_init(void)
{
	if (config.cpu_active == 1) {
		memory_print_map();
		
		#ifdef CONFIG_SMP
		acpi_init();
		#endif /* CONFIG_SMP */
	}
}

void arch_post_smp_init(void)
{
	/* keyboard controller */
	i8042_init(device_assign_devno(), IRQ_KBD, device_assign_devno(), IRQ_MOUSE);
}

void calibrate_delay_loop(void)
{
	i8254_calibrate_delay_loop();
	if (config.cpu_active == 1) {
		/*
		 * This has to be done only on UP.
		 * On SMP, i8254 is not used for time keeping and its interrupt pin remains masked.
		 */
		i8254_normal_operation();
	}
}

/** Set thread-local-storage pointer
 *
 * TLS pointer is set in FS register. Unfortunately the 64-bit
 * part can be set only in CPL0 mode.
 *
 * The specs say, that on %fs:0 there is stored contents of %fs register,
 * we need not to go to CPL0 to read it.
 */
unative_t sys_tls_set(unative_t addr)
{
	THREAD->arch.tls = addr;
	write_msr(AMD_MSR_FS, addr);
	return 0;
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
	i8042_grab();
}
/** Return console to userspace
 *
 */
void arch_release_console(void)
{
	i8042_release();
}

/** @}
 */
