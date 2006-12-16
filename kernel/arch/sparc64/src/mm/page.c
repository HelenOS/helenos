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

/** @addtogroup sparc64mm	
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <bitops.h>
#include <debug.h>
#include <align.h>
#include <config.h>

#ifdef CONFIG_SMP
/** Entries locked in DTLB of BSP.
 *
 * Application processors need to have the same locked entries in their DTLBs as
 * the bootstrap processor.
 */
static struct {
	uintptr_t virt_page;
	uintptr_t phys_page;
	int pagesize_code;
} bsp_locked_dtlb_entry[DTLB_ENTRY_COUNT];

/** Number of entries in bsp_locked_dtlb_entry array. */
static count_t bsp_locked_dtlb_entries = 0;
#endif /* CONFIG_SMP */

/** Perform sparc64 specific initialization of paging. */
void page_arch_init(void)
{
	if (config.cpu_active == 1) {
		page_mapping_operations = &ht_mapping_operations;
	} else {

#ifdef CONFIG_SMP
		int i;

		/*
		 * Copy locked DTLB entries from the BSP.
		 */		
		for (i = 0; i < bsp_locked_dtlb_entries; i++) {
			dtlb_insert_mapping(bsp_locked_dtlb_entry[i].virt_page,
				bsp_locked_dtlb_entry[i].phys_page,
				bsp_locked_dtlb_entry[i].pagesize_code,	true,
				false);
		}
#endif	

	}
}

/** Map memory-mapped device into virtual memory.
 *
 * So far, only DTLB is used to map devices into memory. Chances are that there
 * will be only a limited amount of devices that the kernel itself needs to
 * lock in DTLB.
 *
 * @param physaddr Physical address of the page where the device is located.
 * 	Must be at least page-aligned.
 * @param size Size of the device's registers. Must not exceed 4M and must
 * 	include extra space caused by the alignment.
 *
 * @return Virtual address of the page where the device is mapped.
 */
uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	unsigned int order;
	int i;

	ASSERT(config.cpu_active == 1);

	struct {
		int pagesize_code;
		size_t increment;
		count_t count;
	} sizemap[] = {
		{ PAGESIZE_8K, 0, 1 },			/* 8K */
		{ PAGESIZE_8K, PAGE_SIZE, 2 },		/* 16K */
		{ PAGESIZE_8K, PAGE_SIZE, 4 },		/* 32K */
		{ PAGESIZE_64K, 0, 1},			/* 64K */
		{ PAGESIZE_64K, 8 * PAGE_SIZE, 2 },	/* 128K */
		{ PAGESIZE_64K, 8 * PAGE_SIZE, 4 },	/* 256K */
		{ PAGESIZE_512K, 0, 1 },		/* 512K */
		{ PAGESIZE_512K, 64 * PAGE_SIZE, 2 },	/* 1M */
		{ PAGESIZE_512K, 64 * PAGE_SIZE, 4 },	/* 2M */
		{ PAGESIZE_4M, 0, 1 },			/* 4M */
		{ PAGESIZE_4M, 512 * PAGE_SIZE, 2 }	/* 8M */
	};
	
	ASSERT(ALIGN_UP(physaddr, PAGE_SIZE) == physaddr);
	ASSERT(size <= 8 * 1024 * 1024);
	
	if (size <= FRAME_SIZE)
		order = 0;
	else
		order = (fnzb64(size - 1) + 1) - FRAME_WIDTH;

	/*
	 * Use virtual addresses that are beyond the limit of physical memory.
	 * Thus, the physical address space will not be wasted by holes created
	 * by frame_alloc().
	 */
	ASSERT(PA2KA(last_frame));
	uintptr_t virtaddr = ALIGN_UP(PA2KA(last_frame), 1 << (order + FRAME_WIDTH));
	last_frame = ALIGN_UP(KA2PA(virtaddr) + size, 1 << (order + FRAME_WIDTH));
	
	for (i = 0; i < sizemap[order].count; i++) {
		/*
		 * First, insert the mapping into DTLB.
		 */
		dtlb_insert_mapping(virtaddr + i * sizemap[order].increment,
			physaddr + i * sizemap[order].increment,
			sizemap[order].pagesize_code, true, false);
	
#ifdef CONFIG_SMP	
		/*
		 * Second, save the information about the mapping for APs.
		 */
		bsp_locked_dtlb_entry[bsp_locked_dtlb_entries].virt_page =
			virtaddr + i * sizemap[order].increment;
		bsp_locked_dtlb_entry[bsp_locked_dtlb_entries].phys_page =
			physaddr + i * sizemap[order].increment;
		bsp_locked_dtlb_entry[bsp_locked_dtlb_entries].pagesize_code =
			sizemap[order].pagesize_code;
		bsp_locked_dtlb_entries++;
#endif
	}
	
	return virtaddr;
}

/** @}
 */
