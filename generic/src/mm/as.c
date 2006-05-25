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

/**
 * @file	as.c
 * @brief	Address space related functions.
 *
 * This file contains address space manipulation functions.
 * Roughly speaking, this is a higher-level client of
 * Virtual Address Translation (VAT) subsystem.
 *
 * Functionality provided by this file allows one to
 * create address space and create, resize and share
 * address space areas.
 *
 * @see page.c
 *
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
#include <synch/mutex.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch/asm.h>
#include <panic.h>
#include <debug.h>
#include <print.h>
#include <memstr.h>
#include <macros.h>
#include <arch.h>
#include <errno.h>
#include <config.h>
#include <align.h>
#include <arch/types.h>
#include <typedefs.h>
#include <syscall/copy.h>
#include <arch/interrupt.h>

/** This structure contains information associated with the shared address space area. */
struct share_info {
	mutex_t lock;		/**< This lock must be acquired only when the as_area lock is held. */
	count_t refcount;	/**< This structure can be deallocated if refcount drops to 0. */
	btree_t pagemap;	/**< B+tree containing complete map of anonymous pages of the shared area. */
};

as_operations_t *as_operations = NULL;

/** This lock protects inactive_as_with_asid_head list. It must be acquired before as_t mutex. */
SPINLOCK_INITIALIZE(inactive_as_with_asid_lock);

/**
 * This list contains address spaces that are not active on any
 * processor and that have valid ASID.
 */
LIST_INITIALIZE(inactive_as_with_asid_head);

/** Kernel address space. */
as_t *AS_KERNEL = NULL;

static int area_flags_to_page_flags(int aflags);
static as_area_t *find_area_and_lock(as_t *as, __address va);
static bool check_area_conflicts(as_t *as, __address va, size_t size, as_area_t *avoid_area);
static void sh_info_remove_reference(share_info_t *sh_info);

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
	mutex_initialize(&as->lock);
	btree_create(&as->as_area_btree);
	
	if (flags & FLAG_AS_KERNEL)
		as->asid = ASID_KERNEL;
	else
		as->asid = ASID_INVALID;
	
	as->cpu_refcount = 0;
	as->page_table = page_table_create(flags);

	return as;
}

/** Free Adress space */
void as_free(as_t *as)
{
	ASSERT(as->cpu_refcount == 0);

	/* TODO: free as_areas and other resources held by as */
	/* TODO: free page table */
	free(as);
}

/** Create address space area of common attributes.
 *
 * The created address space area is added to the target address space.
 *
 * @param as Target address space.
 * @param flags Flags of the area memory.
 * @param size Size of area.
 * @param base Base address of area.
 * @param attrs Attributes of the area.
 * @param backend Address space area backend. NULL if no backend is used.
 * @param backend_data NULL or a pointer to an array holding two void *.
 *
 * @return Address space area on success or NULL on failure.
 */
as_area_t *as_area_create(as_t *as, int flags, size_t size, __address base, int attrs,
	       mem_backend_t *backend, void **backend_data)
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
	mutex_lock(&as->lock);
	
	if (!check_area_conflicts(as, base, size, NULL)) {
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return NULL;
	}
	
	a = (as_area_t *) malloc(sizeof(as_area_t), 0);

	mutex_initialize(&a->lock);
	
	a->flags = flags;
	a->attributes = attrs;
	a->pages = SIZE2FRAMES(size);
	a->base = base;
	a->sh_info = NULL;
	a->backend = backend;
	if (backend_data) {
		a->backend_data[0] = backend_data[0];
		a->backend_data[1] = backend_data[1];
	}
	btree_create(&a->used_space);
	
	btree_insert(&as->as_area_btree, base, (void *) a, NULL);

	mutex_unlock(&as->lock);
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
 * @return Zero on success or a value from @ref errno.h otherwise.
 */ 
int as_area_resize(as_t *as, __address address, size_t size, int flags)
{
	as_area_t *area;
	ipl_t ipl;
	size_t pages;
	
	ipl = interrupts_disable();
	mutex_lock(&as->lock);
	
	/*
	 * Locate the area.
	 */
	area = find_area_and_lock(as, address);
	if (!area) {
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	if (area->flags & AS_AREA_DEVICE) {
		/*
		 * Remapping of address space areas associated
		 * with memory mapped devices is not supported.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return ENOTSUP;
	}
	if (area->sh_info) {
		/*
		 * Remapping of shared address space areas 
		 * is not supported.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return ENOTSUP;
	}

	pages = SIZE2FRAMES((address - area->base) + size);
	if (!pages) {
		/*
		 * Zero size address space areas are not allowed.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return EPERM;
	}
	
	if (pages < area->pages) {
		bool cond;
		__address start_free = area->base + pages*PAGE_SIZE;

		/*
		 * Shrinking the area.
		 * No need to check for overlaps.
		 */

		/*
		 * Remove frames belonging to used space starting from
		 * the highest addresses downwards until an overlap with
		 * the resized address space area is found. Note that this
		 * is also the right way to remove part of the used_space
		 * B+tree leaf list.
		 */		
		for (cond = true; cond;) {
			btree_node_t *node;
		
			ASSERT(!list_empty(&area->used_space.leaf_head));
			node = list_get_instance(area->used_space.leaf_head.prev, btree_node_t, leaf_link);
			if ((cond = (bool) node->keys)) {
				__address b = node->key[node->keys - 1];
				count_t c = (count_t) node->value[node->keys - 1];
				int i = 0;
			
				if (overlaps(b, c*PAGE_SIZE, area->base, pages*PAGE_SIZE)) {
					
					if (b + c*PAGE_SIZE <= start_free) {
						/*
						 * The whole interval fits completely
						 * in the resized address space area.
						 */
						break;
					}
		
					/*
					 * Part of the interval corresponding to b and c
					 * overlaps with the resized address space area.
					 */
		
					cond = false;	/* we are almost done */
					i = (start_free - b) >> PAGE_WIDTH;
					if (!used_space_remove(area, start_free, c - i))
						panic("Could not remove used space.");
				} else {
					/*
					 * The interval of used space can be completely removed.
					 */
					if (!used_space_remove(area, b, c))
						panic("Could not remove used space.\n");
				}
			
				for (; i < c; i++) {
					pte_t *pte;
			
					page_table_lock(as, false);
					pte = page_mapping_find(as, b + i*PAGE_SIZE);
					ASSERT(pte && PTE_VALID(pte) && PTE_PRESENT(pte));
					if (area->backend && area->backend->backend_frame_free) {
						area->backend->backend_frame_free(area,
							b + i*PAGE_SIZE, PTE_GET_FRAME(pte));
					}
					page_mapping_remove(as, b + i*PAGE_SIZE);
					page_table_unlock(as, false);
				}
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
			mutex_unlock(&area->lock);
			mutex_unlock(&as->lock);		
			interrupts_restore(ipl);
			return EADDRNOTAVAIL;
		}
	} 

	area->pages = pages;
	
	mutex_unlock(&area->lock);
	mutex_unlock(&as->lock);
	interrupts_restore(ipl);

	return 0;
}

/** Destroy address space area.
 *
 * @param as Address space.
 * @param address Address withing the area to be deleted.
 *
 * @return Zero on success or a value from @ref errno.h on failure. 
 */
int as_area_destroy(as_t *as, __address address)
{
	as_area_t *area;
	__address base;
	ipl_t ipl;
	bool cond;

	ipl = interrupts_disable();
	mutex_lock(&as->lock);

	area = find_area_and_lock(as, address);
	if (!area) {
		mutex_unlock(&as->lock);
		interrupts_restore(ipl);
		return ENOENT;
	}

	base = area->base;

	/*
	 * Visit only the pages mapped by used_space B+tree.
	 * Note that we must be very careful when walking the tree
	 * leaf list and removing used space as the leaf list changes
	 * unpredictibly after each remove. The solution is to actually
	 * not walk the tree at all, but to remove items from the head
	 * of the leaf list until there are some keys left.
	 */
	for (cond = true; cond;) {
		btree_node_t *node;
		
		ASSERT(!list_empty(&area->used_space.leaf_head));
		node = list_get_instance(area->used_space.leaf_head.next, btree_node_t, leaf_link);
		if ((cond = (bool) node->keys)) {
			__address b = node->key[0];
			count_t i;
			pte_t *pte;
			
			for (i = 0; i < (count_t) node->value[0]; i++) {
				page_table_lock(as, false);
				pte = page_mapping_find(as, b + i*PAGE_SIZE);
				ASSERT(pte && PTE_VALID(pte) && PTE_PRESENT(pte));
				if (area->backend && area->backend->backend_frame_free) {
					area->backend->backend_frame_free(area,
						b + i*PAGE_SIZE, PTE_GET_FRAME(pte));
				}
				page_mapping_remove(as, b + i*PAGE_SIZE);
				page_table_unlock(as, false);
			}
			if (!used_space_remove(area, b, i))
				panic("Could not remove used space.\n");
		}
	}
	btree_destroy(&area->used_space);

	/*
	 * Invalidate TLB's.
	 */
	tlb_shootdown_start(TLB_INVL_PAGES, AS->asid, area->base, area->pages);
	tlb_invalidate_pages(AS->asid, area->base, area->pages);
	tlb_shootdown_finalize();

	area->attributes |= AS_AREA_ATTR_PARTIAL;
	
	if (area->sh_info)
		sh_info_remove_reference(area->sh_info);
		
	mutex_unlock(&area->lock);

	/*
	 * Remove the empty area from address space.
	 */
	btree_remove(&AS->as_area_btree, base, NULL);
	
	free(area);
	
	mutex_unlock(&AS->lock);
	interrupts_restore(ipl);
	return 0;
}

/** Share address space area with another or the same address space.
 *
 * Address space area of anonymous memory is shared with a new address
 * space area. If the source address space area has not been shared so
 * far, a new sh_info is created and the original mapping is duplicated
 * in its pagemap B+tree. The new address space are simply gets the
 * sh_info of the source area.
 * 
 * @param src_as Pointer to source address space
 * @param src_base Base address of the source address space area.
 * @param acc_size Expected size of the source area
 * @param dst_base Target base address
 *
 * @return Zero on success or ENOENT if there is no such task or
 *	   if there is no such address space area,
 *	   EPERM if there was a problem in accepting the area or
 *	   ENOMEM if there was a problem in allocating destination
 *	   address space area. ENOTSUP is returned if an attempt
 *	   to share non-anonymous address space area is detected.
 */
int as_area_share(as_t *src_as, __address src_base, size_t acc_size,
		  __address dst_base)
{
	ipl_t ipl;
	int src_flags;
	size_t src_size;
	as_area_t *src_area, *dst_area;
	share_info_t *sh_info;
	link_t *cur;

	ipl = interrupts_disable();
	mutex_lock(&src_as->lock);
	src_area = find_area_and_lock(src_as, src_base);
	if (!src_area) {
		/*
		 * Could not find the source address space area.
		 */
		mutex_unlock(&src_as->lock);
		interrupts_restore(ipl);
		return ENOENT;
	}
	
	if (!src_area->backend || src_area->backend != &anon_backend) {
		/*
		 * As of now, only anonymous address space areas can be shared.
		 */
		mutex_unlock(&src_area->lock);
		mutex_unlock(&src_as->lock);
		interrupts_restore(ipl);
		return ENOTSUP;
	}
	
	src_size = src_area->pages * PAGE_SIZE;
	src_flags = src_area->flags;
	
	if (src_size != acc_size) {
		mutex_unlock(&src_area->lock);
		mutex_unlock(&src_as->lock);
		interrupts_restore(ipl);
		return EPERM;
	}

	/*
	 * Now we are committed to sharing the area.
	 * First prepare the area for sharing.
	 * Then it will be safe to unlock it.
	 */
	sh_info = src_area->sh_info;
	if (!sh_info) {
		sh_info = (share_info_t *) malloc(sizeof(share_info_t), 0);
		mutex_initialize(&sh_info->lock);
		sh_info->refcount = 2;
		btree_create(&sh_info->pagemap);
		src_area->sh_info = sh_info;
	} else {
		mutex_lock(&sh_info->lock);
		sh_info->refcount++;
		mutex_unlock(&sh_info->lock);
	}

	/*
	 * Copy used portions of the area to sh_info's page map.
	 */
	mutex_lock(&sh_info->lock);
	for (cur = src_area->used_space.leaf_head.next; cur != &src_area->used_space.leaf_head; cur = cur->next) {
		btree_node_t *node;
		int i;
		
		node = list_get_instance(cur, btree_node_t, leaf_link);
		for (i = 0; i < node->keys; i++) {
			__address base = node->key[i];
			count_t count = (count_t) node->value[i];
			int j;
			
			for (j = 0; j < count; j++) {
				pte_t *pte;
			
				page_table_lock(src_as, false);
				pte = page_mapping_find(src_as, base + j*PAGE_SIZE);
				ASSERT(pte && PTE_VALID(pte) && PTE_PRESENT(pte));
				btree_insert(&sh_info->pagemap, (base + j*PAGE_SIZE) - src_area->base,
					(void *) PTE_GET_FRAME(pte), NULL);
				page_table_unlock(src_as, false);
			}
				
		}
	}
	mutex_unlock(&sh_info->lock);

	mutex_unlock(&src_area->lock);
	mutex_unlock(&src_as->lock);

	/*
	 * Create copy of the source address space area.
	 * The destination area is created with AS_AREA_ATTR_PARTIAL
	 * attribute set which prevents race condition with
	 * preliminary as_page_fault() calls.
	 */
	dst_area = as_area_create(AS, src_flags, src_size, dst_base, AS_AREA_ATTR_PARTIAL, &anon_backend, NULL);
	if (!dst_area) {
		/*
		 * Destination address space area could not be created.
		 */
		sh_info_remove_reference(sh_info);
		
		interrupts_restore(ipl);
		return ENOMEM;
	}
	
	/*
	 * Now the destination address space area has been
	 * fully initialized. Clear the AS_AREA_ATTR_PARTIAL
	 * attribute and set the sh_info.
	 */	
	mutex_lock(&dst_area->lock);
	dst_area->attributes &= ~AS_AREA_ATTR_PARTIAL;
	dst_area->sh_info = sh_info;
	mutex_unlock(&dst_area->lock);
	
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
		panic("Page not part of any as_area.\n");
	}

	ASSERT(!area->backend);
	
	page_mapping_insert(as, page, frame, as_area_get_flags(area));
	if (!used_space_insert(area, page, 1))
		panic("Could not insert used space.\n");
	
	mutex_unlock(&area->lock);
	page_table_unlock(as, true);
	interrupts_restore(ipl);
}

/** Handle page fault within the current address space.
 *
 * This is the high-level page fault handler. It decides
 * whether the page fault can be resolved by any backend
 * and if so, it invokes the backend to resolve the page
 * fault.
 *
 * Interrupts are assumed disabled.
 *
 * @param page Faulting page.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 * @param istate Pointer to interrupted state.
 *
 * @return AS_PF_FAULT on page fault, AS_PF_OK on success or AS_PF_DEFER if the
 * 	   fault was caused by copy_to_uspace() or copy_from_uspace().
 */
int as_page_fault(__address page, pf_access_t access, istate_t *istate)
{
	pte_t *pte;
	as_area_t *area;
	
	if (!THREAD)
		return AS_PF_FAULT;
		
	ASSERT(AS);

	mutex_lock(&AS->lock);
	area = find_area_and_lock(AS, page);	
	if (!area) {
		/*
		 * No area contained mapping for 'page'.
		 * Signal page fault to low-level handler.
		 */
		mutex_unlock(&AS->lock);
		goto page_fault;
	}

	if (area->attributes & AS_AREA_ATTR_PARTIAL) {
		/*
		 * The address space area is not fully initialized.
		 * Avoid possible race by returning error.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&AS->lock);
		goto page_fault;		
	}

	if (!area->backend || !area->backend->backend_page_fault) {
		/*
		 * The address space area is not backed by any backend
		 * or the backend cannot handle page faults.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&AS->lock);
		goto page_fault;		
	}

	page_table_lock(AS, false);
	
	/*
	 * To avoid race condition between two page faults
	 * on the same address, we need to make sure
	 * the mapping has not been already inserted.
	 */
	if ((pte = page_mapping_find(AS, page))) {
		if (PTE_PRESENT(pte)) {
			page_table_unlock(AS, false);
			mutex_unlock(&area->lock);
			mutex_unlock(&AS->lock);
			return AS_PF_OK;
		}
	}
	
	/*
	 * Resort to the backend page fault handler.
	 */
	if (area->backend->backend_page_fault(area, page, access) != AS_PF_OK) {
		page_table_unlock(AS, false);
		mutex_unlock(&area->lock);
		mutex_unlock(&AS->lock);
		goto page_fault;
	}
	
	page_table_unlock(AS, false);
	mutex_unlock(&area->lock);
	mutex_unlock(&AS->lock);
	return AS_PF_OK;

page_fault:
	if (THREAD->in_copy_from_uspace) {
		THREAD->in_copy_from_uspace = false;
		istate_set_retaddr(istate, (__address) &memcpy_from_uspace_failover_address);
	} else if (THREAD->in_copy_to_uspace) {
		THREAD->in_copy_to_uspace = false;
		istate_set_retaddr(istate, (__address) &memcpy_to_uspace_failover_address);
	} else {
		return AS_PF_FAULT;
	}

	return AS_PF_DEFER;
}

/** Switch address spaces.
 *
 * Note that this function cannot sleep as it is essentially a part of
 * scheduling. Sleeping here would lead to deadlock on wakeup.
 *
 * @param old Old address space or NULL.
 * @param new New address space.
 */
void as_switch(as_t *old, as_t *new)
{
	ipl_t ipl;
	bool needs_asid = false;
	
	ipl = interrupts_disable();
	spinlock_lock(&inactive_as_with_asid_lock);

	/*
	 * First, take care of the old address space.
	 */	
	if (old) {
		mutex_lock_active(&old->lock);
		ASSERT(old->cpu_refcount);
		if((--old->cpu_refcount == 0) && (old != AS_KERNEL)) {
			/*
			 * The old address space is no longer active on
			 * any processor. It can be appended to the
			 * list of inactive address spaces with assigned
			 * ASID.
			 */
			 ASSERT(old->asid != ASID_INVALID);
			 list_append(&old->inactive_as_with_asid_link, &inactive_as_with_asid_head);
		}
		mutex_unlock(&old->lock);
	}

	/*
	 * Second, prepare the new address space.
	 */
	mutex_lock_active(&new->lock);
	if ((new->cpu_refcount++ == 0) && (new != AS_KERNEL)) {
		if (new->asid != ASID_INVALID)
			list_remove(&new->inactive_as_with_asid_link);
		else
			needs_asid = true;	/* defer call to asid_get() until new->lock is released */
	}
	SET_PTL0_ADDRESS(new->page_table);
	mutex_unlock(&new->lock);

	if (needs_asid) {
		/*
		 * Allocation of new ASID was deferred
		 * until now in order to avoid deadlock.
		 */
		asid_t asid;
		
		asid = asid_get();
		mutex_lock_active(&new->lock);
		new->asid = asid;
		mutex_unlock(&new->lock);
	}
	spinlock_unlock(&inactive_as_with_asid_lock);
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
int as_area_get_flags(as_area_t *a)
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
 * @param lock If false, do not attempt to lock as->lock.
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
 * @param unlock If false, do not attempt to unlock as->lock.
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
		mutex_lock(&a->lock);
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
		mutex_lock(&a->lock);
		if ((a->base <= va) && (va < a->base + a->pages * PAGE_SIZE)) {
			return a;
		}
		mutex_unlock(&a->lock);
	}

	/*
	 * Second, locate the left neighbour and test its last record.
	 * Because of its position in the B+tree, it must have base < va.
	 */
	if ((lnode = btree_leaf_node_left_neighbour(&as->as_area_btree, leaf))) {
		a = (as_area_t *) lnode->value[lnode->keys - 1];
		mutex_lock(&a->lock);
		if (va < a->base + a->pages * PAGE_SIZE) {
			return a;
		}
		mutex_unlock(&a->lock);
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
		mutex_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			mutex_unlock(&a->lock);
			return false;
		}
		mutex_unlock(&a->lock);
	}
	if ((node = btree_leaf_node_right_neighbour(&as->as_area_btree, leaf))) {
		a = (as_area_t *) node->value[0];
		mutex_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			mutex_unlock(&a->lock);
			return false;
		}
		mutex_unlock(&a->lock);
	}
	
	/* Second, check the leaf node. */
	for (i = 0; i < leaf->keys; i++) {
		a = (as_area_t *) leaf->value[i];
	
		if (a == avoid_area)
			continue;
	
		mutex_lock(&a->lock);
		if (overlaps(va, size, a->base, a->pages * PAGE_SIZE)) {
			mutex_unlock(&a->lock);
			return false;
		}
		mutex_unlock(&a->lock);
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

/** Return size of the address space area with given base.  */
size_t as_get_size(__address base)
{
	ipl_t ipl;
	as_area_t *src_area;
	size_t size;

	ipl = interrupts_disable();
	src_area = find_area_and_lock(AS, base);
	if (src_area){
		size = src_area->pages * PAGE_SIZE;
		mutex_unlock(&src_area->lock);
	} else {
		size = 0;
	}
	interrupts_restore(ipl);
	return size;
}

/** Mark portion of address space area as used.
 *
 * The address space area must be already locked.
 *
 * @param a Address space area.
 * @param page First page to be marked.
 * @param count Number of page to be marked.
 *
 * @return 0 on failure and 1 on success.
 */
int used_space_insert(as_area_t *a, __address page, count_t count)
{
	btree_node_t *leaf, *node;
	count_t pages;
	int i;

	ASSERT(page == ALIGN_DOWN(page, PAGE_SIZE));
	ASSERT(count);

	pages = (count_t) btree_search(&a->used_space, page, &leaf);
	if (pages) {
		/*
		 * We hit the beginning of some used space.
		 */
		return 0;
	}

	node = btree_leaf_node_left_neighbour(&a->used_space, leaf);
	if (node) {
		__address left_pg = node->key[node->keys - 1], right_pg = leaf->key[0];
		count_t left_cnt = (count_t) node->value[node->keys - 1], right_cnt = (count_t) leaf->value[0];
		
		/*
		 * Examine the possibility that the interval fits
		 * somewhere between the rightmost interval of
		 * the left neigbour and the first interval of the leaf.
		 */
		 
		if (page >= right_pg) {
			/* Do nothing. */
		} else if (overlaps(page, count*PAGE_SIZE, left_pg, left_cnt*PAGE_SIZE)) {
			/* The interval intersects with the left interval. */
			return 0;
		} else if (overlaps(page, count*PAGE_SIZE, right_pg, right_cnt*PAGE_SIZE)) {
			/* The interval intersects with the right interval. */
			return 0;			
		} else if ((page == left_pg + left_cnt*PAGE_SIZE) && (page + count*PAGE_SIZE == right_pg)) {
			/* The interval can be added by merging the two already present intervals. */
			node->value[node->keys - 1] += count + right_cnt;
			btree_remove(&a->used_space, right_pg, leaf);
			return 1; 
		} else if (page == left_pg + left_cnt*PAGE_SIZE) {
			/* The interval can be added by simply growing the left interval. */
			node->value[node->keys - 1] += count;
			return 1;
		} else if (page + count*PAGE_SIZE == right_pg) {
			/*
			 * The interval can be addded by simply moving base of the right
			 * interval down and increasing its size accordingly.
			 */
			leaf->value[0] += count;
			leaf->key[0] = page;
			return 1;
		} else {
			/*
			 * The interval is between both neigbouring intervals,
			 * but cannot be merged with any of them.
			 */
			btree_insert(&a->used_space, page, (void *) count, leaf);
			return 1;
		}
	} else if (page < leaf->key[0]) {
		__address right_pg = leaf->key[0];
		count_t right_cnt = (count_t) leaf->value[0];
	
		/*
		 * Investigate the border case in which the left neighbour does not
		 * exist but the interval fits from the left.
		 */
		 
		if (overlaps(page, count*PAGE_SIZE, right_pg, right_cnt*PAGE_SIZE)) {
			/* The interval intersects with the right interval. */
			return 0;
		} else if (page + count*PAGE_SIZE == right_pg) {
			/*
			 * The interval can be added by moving the base of the right interval down
			 * and increasing its size accordingly.
			 */
			leaf->key[0] = page;
			leaf->value[0] += count;
			return 1;
		} else {
			/*
			 * The interval doesn't adjoin with the right interval.
			 * It must be added individually.
			 */
			btree_insert(&a->used_space, page, (void *) count, leaf);
			return 1;
		}
	}

	node = btree_leaf_node_right_neighbour(&a->used_space, leaf);
	if (node) {
		__address left_pg = leaf->key[leaf->keys - 1], right_pg = node->key[0];
		count_t left_cnt = (count_t) leaf->value[leaf->keys - 1], right_cnt = (count_t) node->value[0];
		
		/*
		 * Examine the possibility that the interval fits
		 * somewhere between the leftmost interval of
		 * the right neigbour and the last interval of the leaf.
		 */

		if (page < left_pg) {
			/* Do nothing. */
		} else if (overlaps(page, count*PAGE_SIZE, left_pg, left_cnt*PAGE_SIZE)) {
			/* The interval intersects with the left interval. */
			return 0;
		} else if (overlaps(page, count*PAGE_SIZE, right_pg, right_cnt*PAGE_SIZE)) {
			/* The interval intersects with the right interval. */
			return 0;			
		} else if ((page == left_pg + left_cnt*PAGE_SIZE) && (page + count*PAGE_SIZE == right_pg)) {
			/* The interval can be added by merging the two already present intervals. */
			leaf->value[leaf->keys - 1] += count + right_cnt;
			btree_remove(&a->used_space, right_pg, node);
			return 1; 
		} else if (page == left_pg + left_cnt*PAGE_SIZE) {
			/* The interval can be added by simply growing the left interval. */
			leaf->value[leaf->keys - 1] +=  count;
			return 1;
		} else if (page + count*PAGE_SIZE == right_pg) {
			/*
			 * The interval can be addded by simply moving base of the right
			 * interval down and increasing its size accordingly.
			 */
			node->value[0] += count;
			node->key[0] = page;
			return 1;
		} else {
			/*
			 * The interval is between both neigbouring intervals,
			 * but cannot be merged with any of them.
			 */
			btree_insert(&a->used_space, page, (void *) count, leaf);
			return 1;
		}
	} else if (page >= leaf->key[leaf->keys - 1]) {
		__address left_pg = leaf->key[leaf->keys - 1];
		count_t left_cnt = (count_t) leaf->value[leaf->keys - 1];
	
		/*
		 * Investigate the border case in which the right neighbour does not
		 * exist but the interval fits from the right.
		 */
		 
		if (overlaps(page, count*PAGE_SIZE, left_pg, left_cnt*PAGE_SIZE)) {
			/* The interval intersects with the left interval. */
			return 0;
		} else if (left_pg + left_cnt*PAGE_SIZE == page) {
			/* The interval can be added by growing the left interval. */
			leaf->value[leaf->keys - 1] += count;
			return 1;
		} else {
			/*
			 * The interval doesn't adjoin with the left interval.
			 * It must be added individually.
			 */
			btree_insert(&a->used_space, page, (void *) count, leaf);
			return 1;
		}
	}
	
	/*
	 * Note that if the algorithm made it thus far, the interval can fit only
	 * between two other intervals of the leaf. The two border cases were already
	 * resolved.
	 */
	for (i = 1; i < leaf->keys; i++) {
		if (page < leaf->key[i]) {
			__address left_pg = leaf->key[i - 1], right_pg = leaf->key[i];
			count_t left_cnt = (count_t) leaf->value[i - 1], right_cnt = (count_t) leaf->value[i];

			/*
			 * The interval fits between left_pg and right_pg.
			 */

			if (overlaps(page, count*PAGE_SIZE, left_pg, left_cnt*PAGE_SIZE)) {
				/* The interval intersects with the left interval. */
				return 0;
			} else if (overlaps(page, count*PAGE_SIZE, right_pg, right_cnt*PAGE_SIZE)) {
				/* The interval intersects with the right interval. */
				return 0;			
			} else if ((page == left_pg + left_cnt*PAGE_SIZE) && (page + count*PAGE_SIZE == right_pg)) {
				/* The interval can be added by merging the two already present intervals. */
				leaf->value[i - 1] += count + right_cnt;
				btree_remove(&a->used_space, right_pg, leaf);
				return 1; 
			} else if (page == left_pg + left_cnt*PAGE_SIZE) {
				/* The interval can be added by simply growing the left interval. */
				leaf->value[i - 1] += count;
				return 1;
			} else if (page + count*PAGE_SIZE == right_pg) {
				/*
			         * The interval can be addded by simply moving base of the right
			 	 * interval down and increasing its size accordingly.
			 	 */
				leaf->value[i] += count;
				leaf->key[i] = page;
				return 1;
			} else {
				/*
				 * The interval is between both neigbouring intervals,
				 * but cannot be merged with any of them.
				 */
				btree_insert(&a->used_space, page, (void *) count, leaf);
				return 1;
			}
		}
	}

	panic("Inconsistency detected while adding %d pages of used space at %P.\n", count, page);
}

/** Mark portion of address space area as unused.
 *
 * The address space area must be already locked.
 *
 * @param a Address space area.
 * @param page First page to be marked.
 * @param count Number of page to be marked.
 *
 * @return 0 on failure and 1 on success.
 */
int used_space_remove(as_area_t *a, __address page, count_t count)
{
	btree_node_t *leaf, *node;
	count_t pages;
	int i;

	ASSERT(page == ALIGN_DOWN(page, PAGE_SIZE));
	ASSERT(count);

	pages = (count_t) btree_search(&a->used_space, page, &leaf);
	if (pages) {
		/*
		 * We are lucky, page is the beginning of some interval.
		 */
		if (count > pages) {
			return 0;
		} else if (count == pages) {
			btree_remove(&a->used_space, page, leaf);
			return 1;
		} else {
			/*
			 * Find the respective interval.
			 * Decrease its size and relocate its start address.
			 */
			for (i = 0; i < leaf->keys; i++) {
				if (leaf->key[i] == page) {
					leaf->key[i] += count*PAGE_SIZE;
					leaf->value[i] -= count;
					return 1;
				}
			}
			goto error;
		}
	}

	node = btree_leaf_node_left_neighbour(&a->used_space, leaf);
	if (node && page < leaf->key[0]) {
		__address left_pg = node->key[node->keys - 1];
		count_t left_cnt = (count_t) node->value[node->keys - 1];

		if (overlaps(left_pg, left_cnt*PAGE_SIZE, page, count*PAGE_SIZE)) {
			if (page + count*PAGE_SIZE == left_pg + left_cnt*PAGE_SIZE) {
				/*
				 * The interval is contained in the rightmost interval
				 * of the left neighbour and can be removed by
				 * updating the size of the bigger interval.
				 */
				node->value[node->keys - 1] -= count;
				return 1;
			} else if (page + count*PAGE_SIZE < left_pg + left_cnt*PAGE_SIZE) {
				count_t new_cnt;
				
				/*
				 * The interval is contained in the rightmost interval
				 * of the left neighbour but its removal requires
				 * both updating the size of the original interval and
				 * also inserting a new interval.
				 */
				new_cnt = ((left_pg + left_cnt*PAGE_SIZE) - (page + count*PAGE_SIZE)) >> PAGE_WIDTH;
				node->value[node->keys - 1] -= count + new_cnt;
				btree_insert(&a->used_space, page + count*PAGE_SIZE, (void *) new_cnt, leaf);
				return 1;
			}
		}
		return 0;
	} else if (page < leaf->key[0]) {
		return 0;
	}
	
	if (page > leaf->key[leaf->keys - 1]) {
		__address left_pg = leaf->key[leaf->keys - 1];
		count_t left_cnt = (count_t) leaf->value[leaf->keys - 1];

		if (overlaps(left_pg, left_cnt*PAGE_SIZE, page, count*PAGE_SIZE)) {
			if (page + count*PAGE_SIZE == left_pg + left_cnt*PAGE_SIZE) {
				/*
				 * The interval is contained in the rightmost interval
				 * of the leaf and can be removed by updating the size
				 * of the bigger interval.
				 */
				leaf->value[leaf->keys - 1] -= count;
				return 1;
			} else if (page + count*PAGE_SIZE < left_pg + left_cnt*PAGE_SIZE) {
				count_t new_cnt;
				
				/*
				 * The interval is contained in the rightmost interval
				 * of the leaf but its removal requires both updating
				 * the size of the original interval and
				 * also inserting a new interval.
				 */
				new_cnt = ((left_pg + left_cnt*PAGE_SIZE) - (page + count*PAGE_SIZE)) >> PAGE_WIDTH;
				leaf->value[leaf->keys - 1] -= count + new_cnt;
				btree_insert(&a->used_space, page + count*PAGE_SIZE, (void *) new_cnt, leaf);
				return 1;
			}
		}
		return 0;
	}	
	
	/*
	 * The border cases have been already resolved.
	 * Now the interval can be only between intervals of the leaf. 
	 */
	for (i = 1; i < leaf->keys - 1; i++) {
		if (page < leaf->key[i]) {
			__address left_pg = leaf->key[i - 1];
			count_t left_cnt = (count_t) leaf->value[i - 1];

			/*
			 * Now the interval is between intervals corresponding to (i - 1) and i.
			 */
			if (overlaps(left_pg, left_cnt*PAGE_SIZE, page, count*PAGE_SIZE)) {
				if (page + count*PAGE_SIZE == left_pg + left_cnt*PAGE_SIZE) {
					/*
				 	* The interval is contained in the interval (i - 1)
					 * of the leaf and can be removed by updating the size
					 * of the bigger interval.
					 */
					leaf->value[i - 1] -= count;
					return 1;
				} else if (page + count*PAGE_SIZE < left_pg + left_cnt*PAGE_SIZE) {
					count_t new_cnt;
				
					/*
					 * The interval is contained in the interval (i - 1)
					 * of the leaf but its removal requires both updating
					 * the size of the original interval and
					 * also inserting a new interval.
					 */
					new_cnt = ((left_pg + left_cnt*PAGE_SIZE) - (page + count*PAGE_SIZE)) >> PAGE_WIDTH;
					leaf->value[i - 1] -= count + new_cnt;
					btree_insert(&a->used_space, page + count*PAGE_SIZE, (void *) new_cnt, leaf);
					return 1;
				}
			}
			return 0;
		}
	}

error:
	panic("Inconsistency detected while removing %d pages of used space from %P.\n", count, page);
}

/** Remove reference to address space area share info.
 *
 * If the reference count drops to 0, the sh_info is deallocated.
 *
 * @param sh_info Pointer to address space area share info.
 */
void sh_info_remove_reference(share_info_t *sh_info)
{
	bool dealloc = false;

	mutex_lock(&sh_info->lock);
	ASSERT(sh_info->refcount);
	if (--sh_info->refcount == 0) {
		dealloc = true;
		bool cond;
		
		/*
		 * Now walk carefully the pagemap B+tree and free/remove
		 * reference from all frames found there.
		 */
		for (cond = true; cond;) {
			btree_node_t *node;
			
			ASSERT(!list_empty(&sh_info->pagemap.leaf_head));
			node = list_get_instance(sh_info->pagemap.leaf_head.next, btree_node_t, leaf_link);
			if ((cond = node->keys)) {
				frame_free(ADDR2PFN((__address) node->value[0]));
				btree_remove(&sh_info->pagemap, node->key[0], node);
			}
		}
		
	}
	mutex_unlock(&sh_info->lock);
	
	if (dealloc) {
		btree_destroy(&sh_info->pagemap);
		free(sh_info);
	}
}

static int anon_page_fault(as_area_t *area, __address addr, pf_access_t access);
static void anon_frame_free(as_area_t *area, __address page, __address frame);

/*
 * Anonymous memory backend.
 */
mem_backend_t anon_backend = {
	.backend_page_fault = anon_page_fault,
	.backend_frame_free = anon_frame_free
};

/** Service a page fault in the anonymous memory address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param addr Faulting virtual address.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 *
 * @return AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK on success (i.e. serviced).
 */
int anon_page_fault(as_area_t *area, __address addr, pf_access_t access)
{
	__address frame;

	if (area->sh_info) {
		btree_node_t *leaf;
		
		/*
		 * The area is shared, chances are that the mapping can be found
		 * in the pagemap of the address space area share info structure.
		 * In the case that the pagemap does not contain the respective
		 * mapping, a new frame is allocated and the mapping is created.
		 */
		mutex_lock(&area->sh_info->lock);
		frame = (__address) btree_search(&area->sh_info->pagemap,
			ALIGN_DOWN(addr, PAGE_SIZE) - area->base, &leaf);
		if (!frame) {
			bool allocate = true;
			int i;
			
			/*
			 * Zero can be returned as a valid frame address.
			 * Just a small workaround.
			 */
			for (i = 0; i < leaf->keys; i++) {
				if (leaf->key[i] == ALIGN_DOWN(addr, PAGE_SIZE)) {
					allocate = false;
					break;
				}
			}
			if (allocate) {
				frame = PFN2ADDR(frame_alloc(ONE_FRAME, 0));
				memsetb(PA2KA(frame), FRAME_SIZE, 0);
				
				/*
				 * Insert the address of the newly allocated frame to the pagemap.
				 */
				btree_insert(&area->sh_info->pagemap, ALIGN_DOWN(addr, PAGE_SIZE) - area->base, (void *) frame, leaf);
			}
		}
		mutex_unlock(&area->sh_info->lock);
	} else {

		/*
		 * In general, there can be several reasons that
		 * can have caused this fault.
		 *
		 * - non-existent mapping: the area is an anonymous
		 *   area (e.g. heap or stack) and so far has not been
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
	}
	
	/*
	 * Map 'page' to 'frame'.
	 * Note that TLB shootdown is not attempted as only new information is being
	 * inserted into page tables.
	 */
	page_mapping_insert(AS, addr, frame, as_area_get_flags(area));
	if (!used_space_insert(area, ALIGN_DOWN(addr, PAGE_SIZE), 1))
		panic("Could not insert used space.\n");
		
	return AS_PF_OK;
}

/** Free a frame that is backed by the anonymous memory backend.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Ignored.
 * @param page Ignored.
 * @param frame Frame to be released.
 */
void anon_frame_free(as_area_t *area, __address page, __address frame)
{
	frame_free(ADDR2PFN(frame));
}

/*
 * Address space related syscalls.
 */

/** Wrapper for as_area_create(). */
__native sys_as_area_create(__address address, size_t size, int flags)
{
	if (as_area_create(AS, flags, size, address, AS_AREA_ATTR_NONE, &anon_backend, NULL))
		return (__native) address;
	else
		return (__native) -1;
}

/** Wrapper for as_area_resize. */
__native sys_as_area_resize(__address address, size_t size, int flags)
{
	return (__native) as_area_resize(AS, address, size, 0);
}

/** Wrapper for as_area_destroy. */
__native sys_as_area_destroy(__address address)
{
	return (__native) as_area_destroy(AS, address);
}
