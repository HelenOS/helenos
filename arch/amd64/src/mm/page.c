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

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <config.h>
#include <memstr.h>
#include <interrupt.h>
#include <print.h>
#include <panic.h>
#include <align.h>

/* Definitions for identity page mapper */
pte_t helper_ptl1[512] __attribute__((aligned (PAGE_SIZE)));
pte_t helper_ptl2[512] __attribute__((aligned (PAGE_SIZE)));
pte_t helper_ptl3[512] __attribute__((aligned (PAGE_SIZE)));
extern pte_t ptl_0; /* From boot.S */

#define PTL1_PRESENT(ptl0, page) (!(GET_PTL1_FLAGS_ARCH(ptl0, PTL0_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))
#define PTL2_PRESENT(ptl1, page) (!(GET_PTL2_FLAGS_ARCH(ptl1, PTL1_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))
#define PTL3_PRESENT(ptl2, page) (!(GET_PTL3_FLAGS_ARCH(ptl2, PTL2_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))

#define PTL1_ADDR(ptl0, page) ((pte_t *)PA2KA(GET_PTL1_ADDRESS_ARCH(ptl0, PTL0_INDEX_ARCH(page))))
#define PTL2_ADDR(ptl1, page) ((pte_t *)PA2KA(GET_PTL2_ADDRESS_ARCH(ptl1, PTL1_INDEX_ARCH(page))))
#define PTL3_ADDR(ptl2, page) ((pte_t *)PA2KA(GET_PTL3_ADDRESS_ARCH(ptl2, PTL2_INDEX_ARCH(page))))

#define SETUP_PTL1(ptl0, page, tgt)  {	\
	SET_PTL1_ADDRESS_ARCH(ptl0, PTL0_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL1_FLAGS_ARCH(ptl0, PTL0_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_PTL2(ptl1, page, tgt)  {	\
	SET_PTL2_ADDRESS_ARCH(ptl1, PTL1_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL2_FLAGS_ARCH(ptl1, PTL1_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_PTL3(ptl2, page, tgt)  {	\
	SET_PTL3_ADDRESS_ARCH(ptl2, PTL2_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL3_FLAGS_ARCH(ptl2, PTL2_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_FRAME(ptl3, page, tgt)  {	\
	SET_FRAME_ADDRESS_ARCH(ptl3, PTL3_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_FRAME_FLAGS_ARCH(ptl3, PTL3_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }


void page_arch_init(void)
{
	__address cur;
	int i;
	int identity_flags = PAGE_CACHEABLE | PAGE_EXEC | PAGE_GLOBAL;

	if (config.cpu_active == 1) {
		page_mapping_operations = &pt_mapping_operations;
		
		/*
		 * PA2KA(identity) mapping for all frames.
		 */
		for (cur = 0; cur < last_frame; cur += FRAME_SIZE) {
			/* Standard identity mapping */
			page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, identity_flags);
		}
		/* Upper kernel mapping
		 * - from zero to top of kernel (include bottom addresses
		 *   because some are needed for init )
		 */
		for (cur = PA2KA_CODE(0); cur < config.base+config.kernel_size; cur += FRAME_SIZE) {
			page_mapping_insert(AS_KERNEL, cur, KA2PA(cur), identity_flags);
		}
		for (i=0; i < init.cnt; i++) {
			for (cur=init.tasks[i].addr;cur < init.tasks[i].size; cur += FRAME_SIZE) {
				page_mapping_insert(AS_KERNEL, PA2KA_CODE(KA2PA(cur)), KA2PA(cur), identity_flags);
			}
		}

		exc_register(14, "page_fault", (iroutine)page_fault);
		write_cr3((__address) AS_KERNEL->page_table);
	}
	else {
		write_cr3((__address) AS_KERNEL->page_table);
	}
}


/** Identity page mapper
 *
 * We need to map whole physical memory identically before the page subsystem
 * is initializaed. This thing clears page table and fills in the specific
 * items.
 */
void ident_page_fault(int n, istate_t *istate)
{
	__address page;
	static __address oldpage = 0;
	pte_t *aptl_1, *aptl_2, *aptl_3;

	page = read_cr2();
	if (oldpage) {
		/* Unmap old address */
		aptl_1 = PTL1_ADDR(&ptl_0, oldpage);
		aptl_2 = PTL2_ADDR(aptl_1, oldpage);
		aptl_3 = PTL3_ADDR(aptl_2, oldpage);

		SET_FRAME_FLAGS_ARCH(aptl_3, PTL3_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (KA2PA(aptl_3) == KA2PA(helper_ptl3))
			SET_PTL3_FLAGS_ARCH(aptl_2, PTL2_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (KA2PA(aptl_2) == KA2PA(helper_ptl2))
			SET_PTL2_FLAGS_ARCH(aptl_1, PTL1_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (KA2PA(aptl_1) == KA2PA(helper_ptl1))
			SET_PTL1_FLAGS_ARCH(&ptl_0, PTL0_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
	}
	if (PTL1_PRESENT(&ptl_0, page))
		aptl_1 = PTL1_ADDR(&ptl_0, page);
	else {
		SETUP_PTL1(&ptl_0, page, helper_ptl1);
		aptl_1 = helper_ptl1;
	}
	    
	if (PTL2_PRESENT(aptl_1, page)) 
		aptl_2 = PTL2_ADDR(aptl_1, page);
	else {
		SETUP_PTL2(aptl_1, page, helper_ptl2);
		aptl_2 = helper_ptl2;
	}

	if (PTL3_PRESENT(aptl_2, page))
		aptl_3 = PTL3_ADDR(aptl_2, page);
	else {
		SETUP_PTL3(aptl_2, page, helper_ptl3);
		aptl_3 = helper_ptl3;
	}
	
	SETUP_FRAME(aptl_3, page, page);

	oldpage = page;
}


void page_fault(int n, istate_t *istate)
{
	__address page;
	pf_access_t access;
	
	page = read_cr2();
	
	if (istate->error_word & PFERR_CODE_RSVD)
		panic("Reserved bit set in page table entry.\n");
	
	if (istate->error_word & PFERR_CODE_RW)
		access = PF_ACCESS_WRITE;
	else if (istate->error_word & PFERR_CODE_ID)
		access = PF_ACCESS_EXEC;
	else
		access = PF_ACCESS_READ;
	
	if (as_page_fault(page, access, istate) == AS_PF_FAULT) {
		print_info_errcode(n, istate);
		printf("Page fault address: %llX\n", page);
		panic("page fault\n");
	}
}


__address hw_map(__address physaddr, size_t size)
{
	if (last_frame + ALIGN_UP(size, PAGE_SIZE) > KA2PA(KERNEL_ADDRESS_SPACE_END_ARCH))
		panic("Unable to map physical memory %p (%d bytes)", physaddr, size)
	
	__address virtaddr = PA2KA(last_frame);
	pfn_t i;
	for (i = 0; i < ADDR2PFN(ALIGN_UP(size, PAGE_SIZE)); i++)
		page_mapping_insert(AS_KERNEL, virtaddr + PFN2ADDR(i), physaddr + PFN2ADDR(i), PAGE_NOT_CACHEABLE);
	
	last_frame = ALIGN_UP(last_frame + size, FRAME_SIZE);
	
	return virtaddr;
}
