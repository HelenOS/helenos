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

#include <arch/types.h>
#include <config.h>
#include <func.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <synch/spinlock.h>
#include <debug.h>

/*
 * Note.
 * This is the preliminary code for controlling paging mechanism on ia32. It is
 * needed by other parts of the kernel for its ability to map virtual addresses
 * to physical. SMP code relies on this feature. Other than that, this code is
 * by no means meant to implement virtual memory in terms of swapping pages in
 * and out.
 */

__address bootstrap_dba; 

void page_arch_init(void)
{
	__address dba;
	__u32 i;

	if (config.cpu_active == 1) {
		dba = frame_alloc(FRAME_KA | FRAME_PANIC);
		memsetb(dba, PAGE_SIZE, 0);
	    
		bootstrap_dba = dba;

		/*
		 * Identity mapping for all but 0th page.
		 * PA2KA(identity) mapping for all but 0th page.
		 */
		for (i = 1; i < frames; i++) {
			map_page_to_frame(i * PAGE_SIZE, i * PAGE_SIZE, PAGE_CACHEABLE, KA2PA(dba));
			map_page_to_frame(PA2KA(i * PAGE_SIZE), i * PAGE_SIZE, PAGE_CACHEABLE, KA2PA(dba));
		}

		trap_register(14, page_fault);
		write_cr3(KA2PA(dba));
	}
	else {
		/*
		 * Application processors need to create their own view of the
		 * virtual address space. Because of that, each AP copies
		 * already-initialized paging information from the bootstrap
		 * processor and adjusts it to fulfill its needs.
		 */

		dba = frame_alloc(FRAME_KA | FRAME_PANIC);
		memcopy(bootstrap_dba, dba, PAGE_SIZE);
		write_cr3(KA2PA(dba));
	}

	paging_on();
}

/*
 * Besides mapping pages to frames, this function also sets the present bit of
 * the page's specifier in both page directory and respective page table. If
 * the page table for this page has not been allocated so far, it will take
 * care of it and allocate the necessary frame.
 *
 * PAGE_CACHEABLE flag: when set, it turns caches for that page on
 * PAGE_NOT_PRESENT flag: when set, it marks the page not present
 * PAGE_USER flag: when set, the page is accessible from userspace
 *
 * When the root parameter is non-zero, it is used as the page directory address.
 * Otherwise, the page directory address is read from CPU.
 */
void map_page_to_frame(__address page, __address frame, int flags, __address root)
{
	struct page_specifier *pd, *pt;
	__address dba, newpt;
	int pde, pte;

	if (root) dba = root;
	else dba = read_cr3();

	pde = page >> 22;		/* page directory entry */
	pte = (page >> 12) & 0x3ff;	/* page table entry */
	
	pd = (struct page_specifier *) PA2KA(dba);
	
	if (!pd[pde].present) {
		/*
		 * There is currently no page table for this address. Allocate
		 * frame for the page table and clean it.
		 */
		newpt = frame_alloc(FRAME_KA);
		pd[pde].frame_address = KA2PA(newpt) >> 12;
		memsetb(newpt, PAGE_SIZE, 0);
		pd[pde].present = 1;
		pd[pde].uaccessible = 1;
	}
	
	pt = (struct page_specifier *) PA2KA((pd[pde].frame_address << 12));

	pt[pte].frame_address = frame >> 12;
	pt[pte].present = !(flags & PAGE_NOT_PRESENT);
	pt[pte].page_cache_disable = !(flags & PAGE_CACHEABLE);
	pt[pte].uaccessible = (flags & PAGE_USER) != 0;
	pt[pte].writeable = (flags & PAGE_WRITE) != 0;	
}
