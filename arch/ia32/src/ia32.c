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

#include <arch.h>

#include <arch/types.h>
#include <typedefs.h>

#include <arch/pm.h>

#include <arch/ega.h>
#include <genarch/i8042/i8042.h>
#include <arch/i8254.h>
#include <arch/i8259.h>

#include <arch/context.h>

#include <config.h>

#include <arch/interrupt.h>
#include <arch/asm.h>
#include <genarch/acpi/acpi.h>

#include <arch/bios/bios.h>

#include <arch/mm/memory_init.h>
#include <interrupt.h>
#include <arch/debugger.h>
#include <proc/thread.h>
#include <syscall/syscall.h>

void arch_pre_mm_init(void)
{
	pm_init();

	if (config.cpu_active == 1) {
		bios_init();
		i8259_init();	/* PIC */
		i8254_init();	/* hard clock */
		
		exc_register(VECTOR_SYSCALL, "syscall", (iroutine) syscall);
		
		#ifdef CONFIG_SMP
		exc_register(VECTOR_TLB_SHOOTDOWN_IPI, "tlb_shootdown",
			     (iroutine) tlb_shootdown_ipi);
		#endif /* CONFIG_SMP */
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		ega_init();	/* video */
		/* Enable debugger */
		debugger_init();
	}
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
	i8042_init();	/* keyboard controller */
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
__native sys_tls_set(__native addr)
{
	THREAD->tls = addr;
	set_tls_desc(addr);

	return 0;
}
