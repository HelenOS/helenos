/*
 * Copyright (C) 2005 Jakub Jermar
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#include <arch.h>
#include <debug.h>
#include <arch/trap/trap.h>
#include <arch/console.h>
#include <arch/drivers/tick.h>
#include <proc/thread.h>
#include <console/console.h>
#include <arch/boot/boot.h>
#include <arch/arch.h>
#include <arch/mm/tlb.h>
#include <mm/asid.h>

bootinfo_t bootinfo;

void arch_pre_mm_init(void)
{
	trap_init();
	tick_init();
}

void arch_post_mm_init(void)
{
	standalone_sparc64_console_init();
}

void arch_pre_smp_init(void)
{
}

void arch_post_smp_init(void)
{
	thread_t *t;

	/*
         * Create thread that polls keyboard.
         */
	t = thread_create(kkbdpoll, NULL, TASK, 0, "kkbdpoll");
	if (!t)
		panic("cannot create kkbdpoll\n");
	thread_ready(t);
}

void calibrate_delay_loop(void)
{
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

/** Take over TLB and trap table.
 *
 * Initialize ITLB and DTLB and switch to kernel
 * trap table.
 *
 * First, demap context 0 and install the
 * global 4M locked kernel mapping.
 *
 * Second, prepare a temporary IMMU mapping in
 * context 1, switch to it, demap context 0,
 * install the global 4M locked kernel mapping
 * in context 0 and switch back to context 0.
 *
 * @param base Base address that will be hardwired in both TLBs.
 */
void take_over_tlb_and_tt(uintptr_t base)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	frame_address_t fr;
	page_address_t pg;

	/*
	 * Switch to the kernel trap table.
	 */
	trap_switch_trap_table();

	fr.address = base;
	pg.address = base;

	/*
	 * We do identity mapping of 4M-page at 4M.
	 */
	tag.value = 0;
	tag.context = 0;
	tag.vpn = pg.vpn;

	data.value = 0;
	data.v = true;
	data.size = PAGESIZE_4M;
	data.pfn = fr.pfn;
	data.l = true;
	data.cp = 1;
	data.cv = 0;
	data.p = true;
	data.w = true;
	data.g = true;

	/*
	 * Straightforwardly demap DMUU context 0,
	 * and replace it with the locked kernel mapping.
	 */
	dtlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_NUCLEUS, 0);
	dtlb_tag_access_write(tag.value);
	dtlb_data_in_write(data.value);

	/*
	 * Install kernel code mapping in context 1
	 * and switch to it.
	 */
	tag.context = 1;
	data.g = false;
	itlb_tag_access_write(tag.value);
	itlb_data_in_write(data.value);
	mmu_primary_context_write(1);
	
	/*
	 * Demap old context 0.
	 */
	itlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_NUCLEUS, 0);
	
	/*
	 * Install the locked kernel mapping in context 0
	 * and switch to it.
	 */
	tag.context = 0;
	data.g = true;
	itlb_tag_access_write(tag.value);
	itlb_data_in_write(data.value);
	mmu_primary_context_write(0);
}

/** @}
 */
