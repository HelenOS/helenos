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
 *  @brief Memory management used while booting the kernel.
 */ 


#include "mm.h"


/** Initializes "section" page table entry.
 *
 *  Will be readable/writable by kernel with no access from user mode.
 *  Will belong to domain 0. No cache or buffering is enabled.
 *  
 *  @param pte    Section entry to initialize.
 *  @param frame  First frame in the section (frame number).
 *
 *  @note         If frame is not 1MB aligned, first lower 1MB aligned frame will be used.
 */   
static void init_pte_level0_section(pte_level0_section_t* pte, unsigned int frame)
{
	pte->descriptor_type   = PTE_DESCRIPTOR_SECTION;
	pte->bufferable        = 0;
	pte->cacheable         = 0; 
   	pte->impl_specific     = 0;
	pte->domain            = 0;
	pte->should_be_zero_1  = 0;
	pte->access_permission = PTE_AP_USER_NO_KERNEL_RW;	
	pte->should_be_zero_2  = 0;
	pte->section_base_addr = frame;
}


/** Initializes page table used while booting the kernel. */
static void init_page_table(void) 
{
	int i;
	const unsigned int first_kernel_page = ADDR2PFN(PA2KA(0));

	// create 1:1 virtual-physical mapping (in lower 2GB)
	for (i = 0; i < first_kernel_page; i++) {
		init_pte_level0_section(&page_table[i], i);
	}

	// create 1:1 virtual-physical mapping in kernel space (upper 2GB),
	// physical addresses start from 0
	for (i = first_kernel_page; i < PTL0_ENTRIES; i++) {
		init_pte_level0_section(&page_table[i], i - first_kernel_page);
	}
}


/** Starts the MMU - initializes page table and enables paging. */
void mmu_start() {
	init_page_table();
	set_ptl0_address(page_table);
	enable_paging();
}


/** @}
 */
 
