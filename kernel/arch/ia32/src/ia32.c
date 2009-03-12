/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2009 Martin Decky
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

#include <arch.h>

#include <arch/types.h>

#include <arch/pm.h>

#include <genarch/multiboot/multiboot.h>
#include <genarch/drivers/legacy/ia32/io.h>
#include <genarch/drivers/ega/ega.h>
#include <arch/drivers/vesa.h>
#include <genarch/drivers/i8042/i8042.h>
#include <genarch/kbrd/kbrd.h>
#include <arch/drivers/i8254.h>
#include <arch/drivers/i8259.h>

#include <arch/context.h>

#include <config.h>

#include <arch/interrupt.h>
#include <arch/asm.h>
#include <genarch/acpi/acpi.h>

#include <arch/bios/bios.h>

#include <interrupt.h>
#include <ddi/irq.h>
#include <arch/debugger.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <ddi/device.h>
#include <sysinfo/sysinfo.h>
#include <arch/boot/boot.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#endif

/** Perform ia32-specific initialization before main_bsp() is called.
 *
 * @param signature Should contain the multiboot signature.
 * @param mi        Pointer to the multiboot information structure.
 */
void arch_pre_main(uint32_t signature, const multiboot_info_t *mi)
{
	/* Parse multiboot information obtained from the bootloader. */
	multiboot_info_parse(signature, mi);	
	
#ifdef CONFIG_SMP
	/* Copy AP bootstrap routines below 1 MB. */
	memcpy((void *) AP_BOOT_OFFSET, (void *) BOOT_OFFSET,
	    (size_t) &_hardcoded_unmapped_size);
#endif
}

void arch_pre_mm_init(void)
{
	pm_init();

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
#ifdef CONFIG_EGA
			ega_init(EGA_BASE, EGA_VIDEORAM);  /* video */
#else
			{}
#endif
		
		/* Enable debugger */
		debugger_init();
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
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
#ifdef CONFIG_PC_KBD
	devno_t devno = device_assign_devno();
	
	/*
	 * Initialize the i8042 controller. Then initialize the keyboard
	 * module and connect it to i8042. Enable keyboard interrupts.
	 */
	indev_t *kbrdin = i8042_init((i8042_t *) I8042_BASE, devno, IRQ_KBD);
	if (kbrdin) {
		kbrd_init(kbrdin);
		trap_virtual_enable_irqs(1 << IRQ_KBD);
	}
	
	/*
	 * This is the necessary evil until the userspace driver is entirely
	 * self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, IRQ_KBD);
	sysinfo_set_item_val("kbd.address.physical", NULL,
	    (uintptr_t) I8042_BASE);
	sysinfo_set_item_val("kbd.address.kernel", NULL,
	    (uintptr_t) I8042_BASE);
#endif
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
 * TLS pointer is set in GS register. That means, the GS contains
 * selector, and the descriptor->base is the correct address.
 */
unative_t sys_tls_set(unative_t addr)
{
	THREAD->arch.tls = addr;
	set_tls_desc(addr);

	return 0;
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
#ifdef CONFIG_FB
	if (vesa_present())
		vesa_redraw();
	else
#endif
#ifdef CONFIG_EGA
		ega_redraw();
#else
		{}
#endif
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
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

/** @}
 */
