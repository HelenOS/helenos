/*
 * Copyright (C) 2001-2006 Jakub Jermar
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

/*
 * This file contains address space manipulation functions.
 * Roughly speaking, this is a higher-level client of
 * Virtual Address Translation (VAT) subsystem.
 */

#include <mm/as.h>
#include <mm/asid.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/tlb.h>
#include <mm/heap.h>
#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/asid.h>
#include <arch/mm/asid.h>
#include <arch/mm/as.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <config.h>
#include <list.h>
#include <panic.h>
#include <arch/asm.h>
#include <debug.h>
#include <memstr.h>
#include <arch.h>
#include <print.h>

#define KAS_START_INDEX		PTL0_INDEX(KERNEL_ADDRESS_SPACE_START)
#define KAS_END_INDEX		PTL0_INDEX(KERNEL_ADDRESS_SPACE_END)
#define KAS_INDICES		(1+(KAS_END_INDEX-KAS_START_INDEX))

/*
 * Here we assume that PFN (Physical Frame Number) space
 * is smaller than the width of index_t. UNALLOCATED_PFN
 * can be then used to mark mappings wich were not
 * yet allocated a physical frame.
 */
#define UNALLOCATED_PFN		((index_t) -1)

/** Create address space. */
/*
 * FIXME: this interface must be meaningful for all possible VAT
 * 	  (Virtual Address Translation) mechanisms.
 */
as_t *as_create(pte_t *ptl0, int flags)
{
	as_t *as;

	as = (as_t *) malloc(sizeof(as_t));
	if (as) {
		list_initialize(&as->as_with_asid_link);
		spinlock_initialize(&as->lock, "as_lock");
		list_initialize(&as->as_area_head);

		if (flags & AS_KERNEL)
			as->asid = ASID_KERNEL;
		else
			as->asid = ASID_INVALID;

		as->ptl0 = ptl0;
		if (!as->ptl0) {
			pte_t *src_ptl0, *dst_ptl0;
		
			src_ptl0 = (pte_t *) PA2KA((__address) GET_PTL0_ADDRESS());
			dst_ptl0 = (pte_t *) frame_alloc(FRAME_KA | FRAME_PANIC, ONE_FRAME, NULL);

//			memsetb((__address) dst_ptl0, PAGE_SIZE, 0);
//			memcpy((void *) &dst_ptl0[KAS_START_INDEX], (void *) &src_ptl0[KAS_START_INDEX], KAS_INDICES);
			
			memcpy((void *) dst_ptl0,(void *) src_ptl0, PAGE_SIZE);

			as->ptl0 = (pte_t *) KA2PA((__address) dst_ptl0);
		}
	}

	return as;
}

/** Create address space area of common attributes.
 *
 * The created address space area is added to the target address space.
 *
 * @param as Target address space.
 * @param type Type of area.
 * @param size Size of area in multiples of PAGE_SIZE.
 * @param base Base address of area.
 *
 * @return Address space area on success or NULL on failure.
 */
as_area_t *as_area_create(as_t *as, as_area_type_t type, size_t size, __address base)
{
	ipl_t ipl;
	as_area_t *a;
	
	if (base % PAGE_SIZE)
		panic("addr not aligned to a page boundary");
	
	ipl = interrupts_disable();
	spinlock_lock(&as->lock);
	
	/*
	 * TODO: test as_area which is to be created doesn't overlap with an existing one.
	 */
	
	a = (as_area_t *) malloc(sizeof(as_area_t));
	if (a) {
		int i;
	
		a->mapping = (index_t *) malloc(size * sizeof(index_t));
		if (!a->mapping) {
			free(a);
			spinlock_unlock(&as->lock);
			interrupts_restore(ipl);
			return NULL;
		}
		
		for (i=0; i<size; i++) {
			/*
			 * Frames will be allocated on-demand by
			 * as_page_fault() or preloaded by
			 * as_area_set_mapping().
			 */
			a->mapping[i] = UNALLOCATED_PFN;
		}
		
		spinlock_initialize(&a->lock, "as_area_lock");
			
		link_initialize(&a->link);			
		a->type = type;
		a->size = size;
		a->base = base;
		
		list_append(&a->link, &as->as_area_head);

	}

	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	return a;
}

/** Load mapping for address space area.
 *
 * Initialize a->mapping.
 *
 * @param a   Target address space area.
 * @param vpn Page number relative to area start.
 * @param pfn Frame number to map.
 */
void as_area_set_mapping(as_area_t *a, index_t vpn, index_t pfn)
{
	ASSERT(vpn < a->size);
	ASSERT(a->mapping[vpn] == UNALLOCATED_PFN);
	ASSERT(pfn != UNALLOCATED_PFN);
	
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&a->lock);
	
	a->mapping[vpn] = pfn;
	
	spinlock_unlock(&a->lock);
	interrupts_restore(ipl);
}

/** Handle page fault within the current address space.
 *
 * This is the high-level page fault handler.
 * Interrupts are assumed disabled.
 *
 * @param page Faulting page.
 *
 * @return 0 on page fault, 1 on success.
 */
int as_page_fault(__address page)
{
	int flags;
	link_t *cur;
	as_area_t *a, *area = NULL;
	index_t vpn;
	__address frame;
	
	ASSERT(AS);
	spinlock_lock(&AS->lock);
	
	/*
	 * Search this areas of this address space for presence of 'page'.
	 */
	for (cur = AS->as_area_head.next; cur != &AS->as_area_head; cur = cur->next) {
		a = list_get_instance(cur, as_area_t, link);
		spinlock_lock(&a->lock);

		if ((page >= a->base) && (page < a->base + a->size * PAGE_SIZE)) {

			/*
			 * We found the area containing 'page'.
			 * TODO: access checking
			 */
			
			vpn = (page - a->base) / PAGE_SIZE;
			area = a;
			break;
		}
		
		spinlock_unlock(&a->lock);
	}
	
	if (!area) {
		/*
		 * No area contained mapping for 'page'.
		 * Signal page fault to low-level handler.
		 */
		spinlock_unlock(&AS->lock);
		return 0;
	}

	/*
	 * Note: area->lock is held.
	 */
	
	/*
	 * Decide if a frame needs to be allocated.
	 * If so, allocate it and adjust area->mapping map.
	 */
	if (area->mapping[vpn] == UNALLOCATED_PFN) {
		frame = frame_alloc(0, ONE_FRAME, NULL);
		memsetb(PA2KA(frame), FRAME_SIZE, 0);
		area->mapping[vpn] = frame / FRAME_SIZE;
		ASSERT(area->mapping[vpn] != UNALLOCATED_PFN);
	} else
		frame = area->mapping[vpn] * FRAME_SIZE;
	
	switch (area->type) {
		case AS_AREA_TEXT:
			flags = PAGE_EXEC | PAGE_READ | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		case AS_AREA_DATA:
		case AS_AREA_STACK:
			flags = PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		default:
			panic("unexpected as_area_type_t %d", area->type);
	}

	/*
	 * Map 'page' to 'frame'.
	 * Note that TLB shootdown is not attempted as only new information is being
	 * inserted into page tables.
	 */
	page_mapping_insert(page, AS->asid, frame, flags, (__address) AS->ptl0);
	
	spinlock_unlock(&area->lock);
	spinlock_unlock(&AS->lock);

	return 1;
}

/** Install address space on CPU.
 *
 * @param as Address space.
 */
void as_install(as_t *as)
{
	ipl_t ipl;
	
	asid_install(as);
	
	ipl = interrupts_disable();
	spinlock_lock(&as->lock);
	ASSERT(as->ptl0);
	SET_PTL0_ADDRESS(as->ptl0);
	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	/*
	 * Perform architecture-specific steps.
	 * (e.g. write ASID to hardware register etc.)
	 */
	as_install_arch(as);
	
	AS = as;
}
