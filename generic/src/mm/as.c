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
#include <genarch/mm/page_ht.h>
#include <mm/asid.h>
#include <arch/mm/asid.h>
#include <synch/spinlock.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <proc/task.h>
#include <arch/asm.h>
#include <panic.h>
#include <debug.h>
#include <print.h>
#include <memstr.h>
#include <macros.h>
#include <arch.h>
#include <errno.h>
#include <config.h>
#include <arch/types.h>
#include <typedefs.h>

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

static int area_flags_to_page_flags(int aflags);
static int get_area_flags(as_area_t *a);
static as_area_t *find_area_and_lock(as_t *as, __address va);
static bool check_area_conflicts(as_t *as, __address va, size_t size, as_area_t *avoid_area);

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
	btree_create(&as->as_area_btree);
	
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
 * @param flags Flags of the area.
 * @param size Size of area.
 * @param base Base address of area.
 *
 * @return Address space area on success or NULL on failure.
 */
as_area_t *as_area_create(as_t *as, int flags, size_t size, __address base)
{
	ipl_t ipl;
	as_area_t *a;
	
	if (base % PAGE_SIZE)
		return NULL;

	if (!size)
		return NULL;

	/* Writeable executable areas are not supported. */
	if ((flags & AS_AREA_EXEC) && (flags & AS_AREA_WRITE))
		return NULL;
	
	ipl = interrupts_disable();
	spinlock_lock(&as->lock);
	
	if (!check_area_conflicts(as, base, size, NULL)) {
		spinlock_unlock(&as->lock);
		interrupts_restore(ipl);
		return NULL;
	}
	
	a = (as_area_t *) malloc(sizeof(as_area_t), 0);

	spinlock_initialize(&a->lock, "as_area_lock");
	
	a->flags = flags;
	a->pages = SIZE2FRAMES(size);
	a->base = base;
	
	btree_insert(&as->as_area_btree, base, (void *) a, NULL);

	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	return a;
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
__address as_area_resize(as_t *as, __address address, size_t size, int flags)
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
		interrupts_restore(ipl);
		return (__address) -1;
	}

	if (area->flags & AS_AREA_DEVICE) {
		/*
		 * Remapping of address space areas associated
		 * with memory mapped devices is not supported.
		 */
		spinlock_unlock(&area->lock);
		spinlock_unlock(&as->lock);
		interrupts_restore(ipl);
		return (__address) -1;
	}

	pages = SIZE2FRAMES((address - area->base) + size);
	if (!pages) {
		/*
		 * Zero size address space areas are not allowed.
		 */
		spinlock_unlock(&area->lock);
		spinlock_unlock(&as->lock);
		interrupts_restore(ipl);
		return (__address) -1;
	}
	
	if (pages < area->pages) {
		int i;

		/*
		 * Shrinking the area.
		 * No need to check for overlaps.
		 */
		for (i = pages; i < area->pages; i++) {
			pte_t *pte;
			
			/*
			 * Releasing physical memory.
			 * This depends on the fact that the memory was allocated using frame_alloc().
			 */
			page_table_lock(as, false);
			pte = page_mapping_find(as, area->base + i*PAGE_SIZE);
			if (pte && PTE_VALID(pte)) {
				__address frame;

				ASSERT(PTE_PRESENT(pte));
				frame = PTE_GET_FRAME(pte);
				page_mapping_remove(as, area->base + i*PAGE_SIZE);
				page_table_unlock(as, false);

				frame_free(ADDR2PFN(frame));
			} else {
				page_table_unlock(as, false);
			}
		}
		/*
		 * Invalidate TLB's.
		 */
		tlb_shootdown_start(TLB_INVL_PAGES, AS->asid, area->base + pages*PAGE_SIZE, area->pages - pages);
		tlb_invalidate_pages(AS->asid, area->base + pages*PAGE_SIZE, area->pages - pages);
		tlb_shootdown_finalize();
	} else {
		/*
		 * Growing the area.
		 * Check for overlaps with other address space areas.
		 */
		if (!check_area_conflicts(as, address, pages * PAGE_SIZE, area)) {
			spinlock_unlock(&area->lock);
			spinlock_unlock(&as->lock);		
			interrupts_restore(ipl);
			return (__address) -1;
		}
	} 

	area->pages = pages;
	
	spinlock_unlock(&area->lock);
	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);

	return address;
}

/** Send address space area to another task.
 *
 * Address space area is sent to the specified task.
 * If the destination task is willing to accept the
 * area, a new area is created according to the
 * source area. Moreover, any existing mapping
 * is copied as well, providing thus a mechanism
 * for sharing group of pages. The source address
 * space area and any associated mapping is preserved.
 *
 * @param id Task ID of the accepting task.
 * @param base Base address of the source address space area.
 *
 * @return 0 on success or ENOENT if there is no such task or
 *	   if there is no such address space area,
 *	   EPERM if there was a problem in accepting the area or
 *	   ENOMEM if there was a problem in allocating destination
 *	   address space area.
 */
int as_area_send(task_id_t id, __address base)
{
	ipl_t ipl;
	task_t *t;
	count_t i;
	as_t *as;
	__address dst_base;
	int flags;
	size_t size;
	as_area_t *area;
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	t = task_find_by_id(id);
	if (!NULL) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	spinlock_lock(&t->lock);
	spinlock_unlock(&tasks_lock);

	as = t->as;
	dst_base = (__address) t->accept_arg.base;
	
	if (as == AS) {
		/*
		 * The two tasks share the entire address space.
		 * Return error since there is no point in continuing.
		 */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		return EPERM;
	}
	
	spinlock_lock(&AS->lock);
	area = find_area_and_lock(AS, base);
	if (!area) {
		/*
		 * Could not find the source address space area.
		 */
		spinlock_unlock(&t->lock);
		spinlock_unlock(&AS->lock);
		interrupts_restore(ipl);
		return ENOENT;
	}
	size = area->pages * PAGE_SIZE;
	flags = area->flags;
	spinlock_unlock(&area->lock);
	spinlock_unlock(&AS->lock);

	if ((t->accept_arg.task_id != TASK->taskid) || (t->accept_arg.size != size) ||
	    (t->accept_arg.flags != flags)) {
		/*
		 * Discrepancy in either task ID, size or flags.
		 */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		return EPERM;
	}
	
	/*
	 * Create copy of the address space area.
	 */
	if (!as_area_create(as, flags, size, dst_base)) {
		/*
		 * Destination address space area could not be created.
		 */
		spinlock_unlock(&t->lock);
		interrupts_restore(ipl);
		return ENOMEM;
	}
	
	/*
	 * NOTE: we have just introduced a race condition.
	 * The destination task can try to attempt the newly
	 * created area before its mapping is copied from
	 * the source address space area. In result, frames
	 * can get lost.
	 *
	 * Currently, this race is not solved, but one of the
	 * possible solutions would be to sleep in as_page_fault()
	 * when this situation is detected.
	 */

	memsetb((__address) &t->accept_arg, sizeof(as_area_acptsnd_arg_t), 0);
	spinlock_unlock(&t->lock);
	
	/*
	 * Avoid deadlock by first locking the address space with lower address.
	 */
	if (as < AS) {
		spinlock_lock(&as->lock);
		spinlock_lock(&AS->lock);
	} else {
		spinlock_lock(&AS->lock);
		spinlock_lock(&as->lock);
	}
	
	for (i = 0; i < SIZE2FRAMES(size); i++) {
		pte_t *pte;
		__address frame;
			
		page_table_lock(AS, false);
		pte = page_mapping_find(AS, base + i*PAGE_SIZE);
		if (pte && PTE_VALID(pte)) {
			ASSERT(PTE_PRESENT(pte));
			frame = PTE_GET_FRAME(pte);
			if (!(flags & AS_AREA_DEVICE))
				frame_reference_add(ADDR2PFN(frame));
			page_table_unlock(AS, false);
		} else {
			page_table_unlock(AS, false);
			continue;
		}
		
		page_table_lock(as, false);
		page_mapping_insert(as, dst_base + i*PAGE_SIZE, frame, area_flags_to_page_flags(flags));
		page_table_unlock(as, false);
	}
	
	spinlock_unlock(&AS->lock);
	spinlock_unlock(&as->lock);
	interrupts_restore(ipl);
	
	return 0;
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
	page_table_lock(as, true);
	
	area = find_area_and_lock(as, page);
	if (!area) {
		panic("page not part of any as_area\n");
	}

	page_mapping_insert(as, page, frame, get_area_flags(area));
	
	spinlock_unlock(&area->lock);
	page_table_unlock(as, true);
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
	pte_t *pte;
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

	ASSERT(!(area->flags & AS_AREA_DEVICE));

	page_table_lock(AS, false);
	
	/*
	 * To avoid race condition between two page faults
	 * on the same address, we need to make sure
	 * the mapping has not been already inserted.
	 */
	if ((pte = page_mapping_find(AS, page))) {
		if (PTE_PRESENT(pte)) {
			page_table_unlock(AS, false);
			spinlock_unlock(&area->lock);
			spinlock_unlock(&AS->lock);
			return 1;
		}
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
	page_table_unlock(AS, false);
	
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

/** Convert address space area flags to page flags.
 *
 * @param aflags Flags of some address space area.
 *
 * @return Flags to be passed to page_mapping_insert().
 */
int area_flags_to_page_flags(int aflags)
{
	int flags;

	flags = PAGE_USER | PAGE_PRESENT;
	
	if (aflags & AS_AREA_READ)
		flags |= PAGE_READ;
		
	if (aflags & AS_AREA_WRITE)
		flags |= PAGE_WRITE;
	
	if (aflags & AS_AREA_EXEC)
		flags |= PAGE_EXEC;
	
	if (!(aflags & AS_AREA_DEVICE))
		flags |= PAGE_CACHEABLE;
		
	return flags;
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
	return area_flags_to_page_flags(a->flags);
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

/** Lock page table.
 *
 * This function should be called before any page_mapping_insert(),
 * page_mapping_remove() and page_mapping_find().
 * 
 * Locking order is such that address space areas must be locked
 * prior to this call. Address space can be locked prior to this
 * call in which case the lock argument is false.
 *
 * @param as Address space.
 * @param as_locked If false, do not attempt to lock as->lock.
 */
void page_table_lock(as_t *as, bool lock)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_lock);

	as_operations->page_table_lock(as, lock);
}

/** Unlock page table.
 *
 * @param as Address space.
 * @param as_locked If false, do not attempt to unlock as->lock.
 */
void page_table_unlock(as_t *as, bool unlock)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_unlock);

	as_operations->page_table_unlock(as, unlock);
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
	as_area_t *a;
	btree_node_t *leaf, *lnode;
	int i;
	
	a = (as_area_t *) btree_search(&as->as_area_btree, va, &leaf);
	if (a) {
		/* va is the base address of an address space area */
		spinlock_lock(&a->lock);
		return a;
	}
	
	/*
	 * Search the leaf node and the righmost record of its left neighbour
	 * to find out whether this is a miss or va belongs to an address
	 * space area found there.
	 */
	
	/* First, search the leaf node itself. */
	for (i = 0; i < leaf->keys; i++) {
		a = (as_area_t *) leaf->value[i];
		spinlock_lock(&a->lock);
		if ((a->base <= va) && (va < a->base + a->pages * PAGE_SIZE)) {
			return a;
		}
		spinlock_unlock(&a->lock);
	}

	/*
	 * Second, locate the left neighbour and test its last record.
	 * Because of its position in the B+tree, it must have base < va.
	 */
	if ((lnode = btree_leaf_node_left_neighbour(&as->as_area_btree, leaf))) {
		a = (as_area_t *) lnode->value[lnode->keys - 1];
		spinlock_lock(&a->lock);
		if (va < a->base + a->pages * PAGE_SIZE) {
			return a;
		}
		spinlock_unlock(&a->lock);
	}

	return NULL;
}

/** Check area conflicts with other areas.
 *
 * The address space must be locked and interrupts must be disabled.
 *
 * @param as Address space.
 * @param va Starting virtual address of the area being tested.
 * @param size Size of the area being tested.
 * @param avoid_area Do not touch this area. 
 *
 * @return True if there is no conflict, false otherwise.
 */
bool check_area_conflicts(as_t *as, __address va, size_t size, as_area_t *avoid_area)
{
	as_area_t *a;
	btree_node_t *leaf, *node;
	int i;
	
	/*
	 * We don't want any area to have conflicts with NULL page.
	 */
	if (overlaps(va, size, NULL, PAGE_SIZE))
		return false;
	
	/*
	 * The leaf node is found in O(log n), where n is proportional to
	 * the number of address space areas belonging to as.
	 * The check for conflicts is then attempted on the rightmost
	 * record in the left neighbour, the leftmost record in the right
	 * neighbour and all records in the leaf node itself.
	 */
	
	if ((a = (as_area_t *) btree_search(&as->as_area_btree, va, &leaf))) {
		if (a != avoid_area)
			return false;
	}
	
	/* First, check the two border cases. */
	if ((node = btree_leaf_node_left_neighbour(&as->as_area_btree, leaf))) {
		a = (as_area_t *) node->value[node->keys - 1];
		spinlock_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			spinlock_unlock(&a->lock);
			return false;
		}
		spinlock_unlock(&a->lock);
	}
	if ((node = btree_leaf_node_right_neighbour(&as->as_area_btree, leaf))) {
		a = (as_area_t *) node->value[0];
		spinlock_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			spinlock_unlock(&a->lock);
			return false;
		}
		spinlock_unlock(&a->lock);
	}
	
	/* Second, check the leaf node. */
	for (i = 0; i < leaf->keys; i++) {
		a = (as_area_t *) leaf->value[i];
	
		if (a == avoid_area)
			continue;
	
		spinlock_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			spinlock_unlock(&a->lock);
			return false;
		}
		spinlock_unlock(&a->lock);
	}

	/*
	 * So far, the area does not conflict with other areas.
	 * Check if it doesn't conflict with kernel address space.
	 */	 
	if (!KERNEL_ADDRESS_SPACE_SHADOWED) {
		return !overlaps(va, size, 
			KERNEL_ADDRESS_SPACE_START, KERNEL_ADDRESS_SPACE_END-KERNEL_ADDRESS_SPACE_START);
	}

	return true;
}

/*
 * Address space related syscalls.
 */

/** Wrapper for as_area_create(). */
__native sys_as_area_create(__address address, size_t size, int flags)
{
	if (as_area_create(AS, flags, size, address))
		return (__native) address;
	else
		return (__native) -1;
}

/** Wrapper for as_area_resize. */
__native sys_as_area_resize(__address address, size_t size, int flags)
{
	return as_area_resize(AS, address, size, 0);
}

/** Prepare task for accepting address space area from another task.
 *
 * @param uspace_accept_arg Accept structure passed from userspace.
 *
 * @return EPERM if the task ID encapsulated in @uspace_accept_arg references
 *	   TASK. Otherwise zero is returned.
 */
__native sys_as_area_accept(as_area_acptsnd_arg_t *uspace_accept_arg)
{
	as_area_acptsnd_arg_t arg;
	
	copy_from_uspace(&arg, uspace_accept_arg, sizeof(as_area_acptsnd_arg_t));
	
	if (!arg.size)
		return (__native) EPERM;
	
	if (arg.task_id == TASK->taskid) {
		/*
		 * Accepting from itself is not allowed.
		 */
		return (__native) EPERM;
	}
	
	memcpy(&TASK->accept_arg, &arg, sizeof(as_area_acptsnd_arg_t));
	
        return 0;
}

/** Wrapper for as_area_send. */
__native sys_as_area_send(as_area_acptsnd_arg_t *uspace_send_arg)
{
	as_area_acptsnd_arg_t arg;
	
	copy_from_uspace(&arg, uspace_send_arg, sizeof(as_area_acptsnd_arg_t));

	if (!arg.size)
		return (__native) EPERM;
	
	if (arg.task_id == TASK->taskid) {
		/*
		 * Sending to itself is not allowed.
		 */
		return (__native) EPERM;
	}

	return (__native) as_area_send(arg.task_id, (__address) arg.base);
}
