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
#include <arch/mm/as.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <mm/tlb.h>
#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/asid.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <config.h>
#include <adt/list.h>
#include <panic.h>
#include <arch/asm.h>
#include <debug.h>
#include <memstr.h>
#include <arch.h>
#include <print.h>

as_operations_t *as_operations = NULL;

/** Address space lock. It protects inactive_as_with_asid_head. */
SPINLOCK_INITIALIZE(as_lock);

/**
 * This list contains address spaces that are not active on any
 * processor and that have valid ASID.
 */
LIST_INITIALIZE(inactive_as_with_asid_head);

/** Kernel address space. */
as_t *AS_KERNEL = NULL;

static int get_area_flags(as_area_t *a);
static as_area_t *find_area_and_lock(as_t *as, __address va);

/** Initialize address space subsystem. */
void as_init(void)
{
	as_arch_init();
	AS_KERNEL = as_create(FLAG_AS_KERNEL);
        if (!AS_KERNEL)
                panic("can't create kernel address space\n");
}

/** Create address space.
 *
 * @param flags Flags that influence way in wich the address space is created.
 */
as_t *as_create(int flags)
{
	as_t *as;

	as = (as_t *) malloc(sizeof(as_t), 0);
	link_initialize(&as->inactive_as_with_asid_link);
	spinlock_initialize(&as->lock, "as_lock");
	list_initialize(&as->as_area_head);
	
	if (flags & FLAG_AS_KERNEL)
		as->asid = ASID_KERNEL;
	else
		as->asid = ASID_INVALID;
	
	as->refcount = 0;
	as->page_table = page_table_create(flags);

	return as;
}

/** Free Adress space */
void as_free(as_t *as)
{
	ASSERT(as->refcount == 0);

	/* TODO: free as_areas and other resources held by as */
	/* TODO: free page table */
	free(as);
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
	
	a = (as_area_t *) malloc(sizeof(as_area_t), 0);

	spinlock_initialize(&a->lock, "as_area_lock");
	
	link_initialize(&a->link);			
	a->type = type;
	a->size = size;
	a->base = base;
	
	list_append(&a->link, &as->as_area_head);

	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	return a;
}

/** Initialize mapping for one page of address space.
 *
 * This functions maps 'page' to 'frame' according
 * to attributes of the address space area to
 * wich 'page' belongs.
 *
 * @param as Target address space.
 * @param page Virtual page within the area.
 * @param frame Physical frame to which page will be mapped.
 */
void as_set_mapping(as_t *as, __address page, __address frame)
{
	as_area_t *area;
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&as->lock);
	
	area = find_area_and_lock(as, page);
	if (!area) {
		panic("page not part of any as_area\n");
	}

	page_mapping_insert(as, page, frame, get_area_flags(area));
	
	spinlock_unlock(&area->lock);
	spinlock_unlock(&as->lock);
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
	as_area_t *area;
	__address frame;
	
	ASSERT(AS);
	spinlock_lock(&AS->lock);
	
	area = find_area_and_lock(AS, page);	
	if (!area) {
		/*
		 * No area contained mapping for 'page'.
		 * Signal page fault to low-level handler.
		 */
		spinlock_unlock(&AS->lock);
		return 0;
	}

	/*
	 * In general, there can be several reasons that
	 * can have caused this fault.
	 *
	 * - non-existent mapping: the area is a scratch
	 *   area (e.g. stack) and so far has not been
	 *   allocated a frame for the faulting page
	 *
	 * - non-present mapping: another possibility,
	 *   currently not implemented, would be frame
	 *   reuse; when this becomes a possibility,
	 *   do not forget to distinguish between
	 *   the different causes
	 */
	frame = PFN2ADDR(frame_alloc(ONE_FRAME, 0));
	memsetb(PA2KA(frame), FRAME_SIZE, 0);
	
	/*
	 * Map 'page' to 'frame'.
	 * Note that TLB shootdown is not attempted as only new information is being
	 * inserted into page tables.
	 */
	page_mapping_insert(AS, page, frame, get_area_flags(area));
	
	spinlock_unlock(&area->lock);
	spinlock_unlock(&AS->lock);

	return 1;
}

/** Switch address spaces.
 *
 * @param old Old address space or NULL.
 * @param new New address space.
 */
void as_switch(as_t *old, as_t *new)
{
	ipl_t ipl;
	bool needs_asid = false;
	
	ipl = interrupts_disable();
	spinlock_lock(&as_lock);

	/*
	 * First, take care of the old address space.
	 */	
	if (old) {
		spinlock_lock(&old->lock);
		ASSERT(old->refcount);
		if((--old->refcount == 0) && (old != AS_KERNEL)) {
			/*
			 * The old address space is no longer active on
			 * any processor. It can be appended to the
			 * list of inactive address spaces with assigned
			 * ASID.
			 */
			 ASSERT(old->asid != ASID_INVALID);
			 list_append(&old->inactive_as_with_asid_link, &inactive_as_with_asid_head);
		}
		spinlock_unlock(&old->lock);
	}

	/*
	 * Second, prepare the new address space.
	 */
	spinlock_lock(&new->lock);
	if ((new->refcount++ == 0) && (new != AS_KERNEL)) {
		if (new->asid != ASID_INVALID)
			list_remove(&new->inactive_as_with_asid_link);
		else
			needs_asid = true;	/* defer call to asid_get() until new->lock is released */
	}
	SET_PTL0_ADDRESS(new->page_table);
	spinlock_unlock(&new->lock);

	if (needs_asid) {
		/*
		 * Allocation of new ASID was deferred
		 * until now in order to avoid deadlock.
		 */
		asid_t asid;
		
		asid = asid_get();
		spinlock_lock(&new->lock);
		new->asid = asid;
		spinlock_unlock(&new->lock);
	}
	spinlock_unlock(&as_lock);
	interrupts_restore(ipl);
	
	/*
	 * Perform architecture-specific steps.
	 * (e.g. write ASID to hardware register etc.)
	 */
	as_install_arch(new);
	
	AS = new;
}

/** Compute flags for virtual address translation subsytem.
 *
 * The address space area must be locked.
 * Interrupts must be disabled.
 *
 * @param a Address space area.
 *
 * @return Flags to be used in page_mapping_insert().
 */
int get_area_flags(as_area_t *a)
{
	int flags;

	switch (a->type) {
		case AS_AREA_TEXT:
			flags = PAGE_EXEC | PAGE_READ | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		case AS_AREA_DATA:
		case AS_AREA_STACK:
			flags = PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_PRESENT | PAGE_CACHEABLE;
			break;
		default:
			panic("unexpected as_area_type_t %d", a->type);
	}
	
	return flags;
}

/** Create page table.
 *
 * Depending on architecture, create either address space
 * private or global page table.
 *
 * @param flags Flags saying whether the page table is for kernel address space.
 *
 * @return First entry of the page table.
 */
pte_t *page_table_create(int flags)
{
        ASSERT(as_operations);
        ASSERT(as_operations->page_table_create);

        return as_operations->page_table_create(flags);
}

/** Find address space area and change it.
 *
 * @param as Address space.
 * @param address Virtual address belonging to the area to be changed. Must be page-aligned.
 * @param size New size of the virtual memory block starting at address. 
 * @param flags Flags influencing the remap operation. Currently unused.
 *
 * @return address on success, (__address) -1 otherwise.
 */ 
__address as_remap(as_t *as, __address address, size_t size, int flags)
{
	as_area_t *area = NULL;
	ipl_t ipl;
	size_t pages;
	
	ipl = interrupts_disable();
	spinlock_lock(&as->lock);
	
	/*
	 * Locate the area.
	 */
	area = find_area_and_lock(as, address);
	if (!area) {
		spinlock_unlock(&as->lock);
		return (__address) -1;
	}

	pages = SIZE2FRAMES((address - area->base) + size);
	if (pages < area->size) {
		int i;

		/*
		 * Shrinking the area.
		 */
		for (i = pages; i < area->size; i++) {
			pte_t *pte;
			
			/*
			 * Releasing physical memory.
			 * This depends on the fact that the memory was allocated using frame_alloc().
			 */ 
			pte = page_mapping_find(as, area->base + i*PAGE_SIZE);
			if (pte && PTE_VALID(pte)) {
				ASSERT(PTE_PRESENT(pte));
				frame_free(ADDR2PFN(PTE_GET_FRAME(pte)));
				page_mapping_remove(as, area->base + i*PAGE_SIZE);
			}
		}
		/*
		 * Invalidate TLB's.
		 */
		tlb_shootdown_start(TLB_INVL_PAGES, AS->asid, area->base + pages*PAGE_SIZE, area->size - pages);
		tlb_invalidate_pages(AS->asid, area->base + pages*PAGE_SIZE, area->size - pages);
		tlb_shootdown_finalize();
	} 

	area->size = pages;
	
	spinlock_unlock(&area->lock);
	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	return address;
}

/** Find address space area and lock it.
 *
 * The address space must be locked and interrupts must be disabled.
 *
 * @param as Address space.
 * @param va Virtual address.
 *
 * @return Locked address space area containing va on success or NULL on failure.
 */
as_area_t *find_area_and_lock(as_t *as, __address va)
{
	link_t *cur;
	as_area_t *a;
	
	for (cur = as->as_area_head.next; cur != &as->as_area_head; cur = cur->next) {
		a = list_get_instance(cur, as_area_t, link);
		spinlock_lock(&a->lock);

		if ((va >= a->base) && (va < a->base + a->size * PAGE_SIZE))
			 return a;
		
		spinlock_unlock(&a->lock);
	}

	return NULL;
}
