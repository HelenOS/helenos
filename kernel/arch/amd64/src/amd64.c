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
#include <typedefs.h>
#include <errno.h>
#include <memstr.h>
#include <interrupt.h>
#include <console/console.h>
#include <syscall/syscall.h>
#include <sysinfo/sysinfo.h>
#include <arch/bios/bios.h>
#include <arch/boot/boot.h>
#include <arch/drivers/i8254.h>
#include <arch/drivers/i8259.h>
#include <arch/syscall.h>
#include <genarch/acpi/acpi.h>
#include <genarch/drivers/ega/ega.h>
#include <genarch/drivers/i8042/i8042.h>
#include <genarch/drivers/ns16550/ns16550.h>
#include <genarch/drivers/legacy/ia32/io.h>
#include <genarch/fb/bfb.h>
#include <genarch/kbrd/kbrd.h>
#include <genarch/srln/srln.h>
#include <genarch/multiboot/multiboot.h>
#include <genarch/multiboot/multiboot2.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#endif

/** Disable I/O on non-privileged levels
 *
 * Clean IOPL(12,13) and NT(14) flags in EFLAGS register
 */
static void clean_IOPL_NT_flags(void)
{
	asm volatile (
		"pushfq\n"
		"pop %%rax\n"
		"and $~(0x7000), %%rax\n"
		"pushq %%rax\n"
		"popfq\n"
		::: "%rax"
	);
}

/** Disable alignment check
 *
 * Clean AM(18) flag in CR0 register 
 */
static void clean_AM_flag(void)
{
	asm volatile (
		"mov %%cr0, %%rax\n"
		"and $~(0x40000), %%rax\n"
		"mov %%rax, %%cr0\n"
		::: "%rax"
	);
}

/** Perform amd64-specific initialization before main_bsp() is called.
 *
 * @param signature Multiboot signature.
 * @param info      Multiboot information structure.
 *
 */
void arch_pre_main(uint32_t signature, void *info)
{
	/* Parse multiboot information obtained from the bootloader. */
	multiboot_info_parse(signature, (multiboot_info_t *) info);
	multiboot2_info_parse(signature, (multiboot2_info_t *) info);
	
#ifdef CONFIG_SMP
	/* Copy AP bootstrap routines below 1 MB. */
	memcpy((void *) AP_BOOT_OFFSET, (void *) BOOT_OFFSET,
	    (size_t) &_hardcoded_unmapped_size);
#endif
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
		
#if (defined(CONFIG_FB) || defined(CONFIG_EGA))
		bool bfb = false;
#endif
		
#ifdef CONFIG_FB
		bfb = bfb_init();
#endif
		
#ifdef CONFIG_EGA
		if (!bfb) {
			outdev_t *egadev = ega_init(EGA_BASE, EGA_VIDEORAM);
			if (egadev)
				stdout_wire(egadev);
		}
#endif
		
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
#ifdef CONFIG_SMP
		acpi_init();
#endif /* CONFIG_SMP */
	}
}

void arch_post_smp_init(void)
{
	/* Currently the only supported platform for amd64 is 'pc'. */
	static const char *platform = "pc";

	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));

#ifdef CONFIG_PC_KBD
	/*
	 * Initialize the i8042 controller. Then initialize the keyboard
	 * module and connect it to i8042. Enable keyboard interrupts.
	 */
	i8042_instance_t *i8042_instance = i8042_init((i8042_t *) I8042_BASE, IRQ_KBD);
	if (i8042_instance) {
		kbrd_instance_t *kbrd_instance = kbrd_init();
		if (kbrd_instance) {
			indev_t *sink = stdin_wire();
			indev_t *kbrd = kbrd_wire(kbrd_instance, sink);
			i8042_wire(i8042_instance, kbrd);
			trap_virtual_enable_irqs(1 << IRQ_KBD);
			trap_virtual_enable_irqs(1 << IRQ_MOUSE);
		}
	}
#endif

#if (defined(CONFIG_NS16550) || defined(CONFIG_NS16550_OUT))
	/*
	 * Initialize the ns16550 controller.
	 */
#ifdef CONFIG_NS16550_OUT
	outdev_t *ns16550_out;
	outdev_t **ns16550_out_ptr = &ns16550_out;
#else
	outdev_t **ns16550_out_ptr = NULL;
#endif
	ns16550_instance_t *ns16550_instance
	    = ns16550_init((ns16550_t *) NS16550_BASE, IRQ_NS16550, NULL, NULL,
	    ns16550_out_ptr);
	if (ns16550_instance) {
#ifdef CONFIG_NS16550
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			ns16550_wire(ns16550_instance, srln);
			trap_virtual_enable_irqs(1 << IRQ_NS16550);
		}
#endif
#ifdef CONFIG_NS16550_OUT
		if (ns16550_out) {
			stdout_wire(ns16550_out);
		}
#endif
	}
#endif
	
	if (irqs_info != NULL)
		sysinfo_set_item_val(irqs_info, NULL, true);
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
sysarg_t sys_tls_set(uintptr_t addr)
{
	THREAD->arch.tls = addr;
	write_msr(AMD_MSR_FS, addr);
	
	return EOK;
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void arch_reboot(void)
{
#ifdef CONFIG_PC_KBD
	i8042_cpu_reset((i8042_t *) I8042_BASE);
#endif
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

/** @}
 */
