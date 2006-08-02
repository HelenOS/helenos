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

/** @addtogroup xen32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <main/main.h>

#include <arch/types.h>
#include <typedefs.h>
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

#include <arch/boot/boot.h>
#include <arch/mm/memory_init.h>
#include <interrupt.h>
#include <arch/debugger.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>

start_info_t start_info;
memzone_t meminfo;

extern void xen_callback(void);
extern void xen_failsafe_callback(void);

void arch_pre_main(void)
{
	xen_vm_assist(VMASST_CMD_ENABLE, VMASST_TYPE_WRITABLE_PAGETABLES);
	
	pte_t pte;
	memsetb((uintptr_t) &pte, sizeof(pte), 0);
	
	pte.present = 1;
	pte.writeable = 1;
	pte.frame_address = ADDR2PFN((uintptr_t) start_info.shared_info);
	xen_update_va_mapping(&shared_info, pte, UVMF_INVLPG);
	
	pte.present = 1;
	pte.writeable = 1;
	pte.frame_address = start_info.console_mfn;
	xen_update_va_mapping(&console_page, pte, UVMF_INVLPG);
	
	xen_set_callbacks(XEN_CS, xen_callback, XEN_CS, xen_failsafe_callback);
	
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
			SET_FRAME_FLAGS(tptl3, PTL3_INDEX(tva), PAGE_PRESENT);
			SET_PTL1_ADDRESS(start_info.ptl0, PTL0_INDEX(va), tpa);
			
			last_ptl0 = PTL0_INDEX(va);
			meminfo.reserved++;
		}
		
		pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(start_info.ptl0, PTL0_INDEX(va)));
		
		SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(va), pa);
		SET_FRAME_FLAGS(ptl3, PTL3_INDEX(va), PAGE_PRESENT | PAGE_WRITE);
	}
}

void arch_pre_mm_init(void)
{
	pm_init();

	if (config.cpu_active == 1) {
//		bios_init();
		
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
		/* video */
		xen_console_init();
		/* Enable debugger */
		debugger_init();
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
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

/** @}
 */
