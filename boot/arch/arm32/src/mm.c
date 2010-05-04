/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32boot
 * @{
 */
/** @file
 * @brief Memory management used while booting the kernel.
 */

#include <typedefs.h>
#include <arch/asm.h>
#include <arch/mm.h>

/** Initialize "section" page table entry.
 *
 * Will be readable/writable by kernel with no access from user mode.
 * Will belong to domain 0. No cache or buffering is enabled.
 *
 * @param pte   Section entry to initialize.
 * @param frame First frame in the section (frame number).
 *
 * @note If frame is not 1 MB aligned, first lower 1 MB aligned frame will be
 *       used.
 *
 */
static void init_ptl0_section(pte_level0_section_t* pte,
    pfn_t frame)
{
	pte->descriptor_type = PTE_DESCRIPTOR_SECTION;
	pte->bufferable = 0;
	pte->cacheable = 0;
	pte->impl_specific = 0;
	pte->domain = 0;
	pte->should_be_zero_1 = 0;
	pte->access_permission = PTE_AP_USER_NO_KERNEL_RW;
	pte->should_be_zero_2 = 0;
	pte->section_base_addr = frame;
}

/** Initialize page table used while booting the kernel. */
static void init_boot_pt(void)
{
	pfn_t split_page = 0x800;
	
	/* Create 1:1 virtual-physical mapping (in lower 2 GB). */
	pfn_t page;
	for (page = 0; page < split_page; page++)
		init_ptl0_section(&boot_pt[page], page);
	
	/*
	 * Create 1:1 virtual-physical mapping in kernel space
	 * (upper 2 GB), physical addresses start from 0.
	 */
	for (page = split_page; page < PTL0_ENTRIES; page++)
		init_ptl0_section(&boot_pt[page], page - split_page);
	
	asm volatile (
		"mcr p15, 0, %[pt], c2, c0, 0\n"
		:: [pt] "r" (boot_pt)
	);
}

static void enable_paging()
{
	/* c3   - each two bits controls access to the one of domains (16)
	 * 0b01 - behave as a client (user) of a domain
	 */
	asm volatile (
		/* Behave as a client of domains */
		"ldr r0, =0x55555555\n"
		"mcr p15, 0, r0, c3, c0, 0\n" 
		
		/* Current settings */
		"mrc p15, 0, r0, c1, c0, 0\n"
		
		/* Mask to enable paging */
		"ldr r1, =0x00000001\n"
		"orr r0, r0, r1\n"
		
		/* Store settings */
		"mcr p15, 0, r0, c1, c0, 0\n"
		::: "r0", "r1"
	);
}

/** Start the MMU - initialize page table and enable paging. */
void mmu_start() {
	init_boot_pt();
	enable_paging();
}

/** @}
 */
