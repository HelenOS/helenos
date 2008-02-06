/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ia32xen
 * @{
 */
/** @file
 */

#include <arch.h>
#include <main/main.h>

#include <arch/types.h>
#include <align.h>

#include <arch/pm.h>

#include <arch/drivers/xconsole.h>
#include <arch/mm/page.h>

#include <arch/context.h>

#include <config.h>

#include <arch/interrupt.h>
#include <arch/asm.h>
#include <genarch/acpi/acpi.h>

#include <arch/bios/bios.h>

#include <interrupt.h>
#include <arch/debugger.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <ddi/irq.h>

start_info_t start_info;
memzone_t meminfo;

extern void xen_callback(void);
extern void xen_failsafe_callback(void);

void arch_pre_main(void)
{
	pte_t pte;
	memsetb((uintptr_t) &pte, sizeof(pte), 0);
	
	pte.present = 1;
	pte.writeable = 1;
	pte.frame_address = ADDR2PFN((uintptr_t) start_info.shared_info);
	ASSERT(xen_update_va_mapping(&shared_info, pte, UVMF_INVLPG) == 0);
	
	if (!(start_info.flags & SIF_INITDOMAIN)) {
		/* Map console frame */
		pte.present = 1;
		pte.writeable = 1;
		pte.frame_address = start_info.console.domU.mfn;
		ASSERT(xen_update_va_mapping(&console_page, pte, UVMF_INVLPG) == 0);
	} else
		start_info.console.domU.evtchn = 0;
	
	ASSERT(xen_set_callbacks(XEN_CS, xen_callback, XEN_CS, xen_failsafe_callback) == 0);
	
	/* Create identity mapping */
	
	meminfo.start = ADDR2PFN(ALIGN_UP(KA2PA(start_info.ptl0), PAGE_SIZE)) + start_info.pt_frames;
	meminfo.size = start_info.frames - meminfo.start;
	meminfo.reserved = 0;

	uintptr_t pa;
	index_t last_ptl0 = 0;
	for (pa = PFN2ADDR(meminfo.start); pa < PFN2ADDR(meminfo.start + meminfo.size); pa += FRAME_SIZE) {
		uintptr_t va = PA2KA(pa);
		
		if ((PTL0_INDEX(va) != last_ptl0) && (GET_PTL1_FLAGS(start_info.ptl0, PTL0_INDEX(va)) & PAGE_NOT_PRESENT)) {
			/* New page directory entry needed */
			uintptr_t tpa = PFN2ADDR(meminfo.start + meminfo.reserved);
			uintptr_t tva = PA2KA(tpa);
			
			memsetb(tva, PAGE_SIZE, 0);
			
			pte_t *tptl3 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(start_info.ptl0, PTL0_INDEX(tva)));
			SET_FRAME_ADDRESS(tptl3, PTL3_INDEX(tva), 0);
			SET_PTL1_ADDRESS(start_info.ptl0, PTL0_INDEX(va), tpa);
			SET_FRAME_ADDRESS(tptl3, PTL3_INDEX(tva), tpa);
			
			last_ptl0 = PTL0_INDEX(va);
			meminfo.reserved++;
		}
		
		pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(start_info.ptl0, PTL0_INDEX(va)));
		
		SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(va), pa);
		SET_FRAME_FLAGS(ptl3, PTL3_INDEX(va), PAGE_PRESENT | PAGE_WRITE);
	}
	
	/* Put initial stack safely in the mapped area */
	stack_safe = PA2KA(PFN2ADDR(meminfo.start + meminfo.reserved));
}

void arch_pre_mm_init(void)
{
	pm_init();

	if (config.cpu_active == 1) {
		interrupt_init();
//		bios_init();
		
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
		
		/* Video */
		xen_console_init();
		
		/* Enable debugger */
		debugger_init();
		
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
}

void arch_post_cpu_init(void)
{
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
}

void calibrate_delay_loop(void)
{
//	i8254_calibrate_delay_loop();
	if (config.cpu_active == 1) {
		/*
		 * This has to be done only on UP.
		 * On SMP, i8254 is not used for time keeping and its interrupt pin remains masked.
		 */
//		i8254_normal_operation();
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
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
}

void arch_reboot(void)
{
	// TODO
	while (1);
}

/** @}
 */
