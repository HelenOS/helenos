/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief Address space related functions.
 *
 * This file contains address space manipulation functions.
 * Roughly speaking, this is a higher-level client of
 * Virtual Address Translation (VAT) subsystem.
 *
 * Functionality provided by this file allows one to
 * create address spaces and create, resize and share
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
#include <preemption.h>
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
#include <bitops.h>
#include <arch.h>
#include <errno.h>
#include <config.h>
#include <align.h>
#include <typedefs.h>
#include <syscall/copy.h>
#include <arch/interrupt.h>

/**
 * Each architecture decides what functions will be used to carry out
 * address space operations such as creating or locking page tables.
 */
as_operations_t *as_operations = NULL;

/** Slab for as_t objects.
 *
 */
static slab_cache_t *as_slab;

/** ASID subsystem lock.
 *
 * This lock protects:
 * - inactive_as_with_asid_list
 * - as->asid for each as of the as_t type
 * - asids_allocated counter
 *
 */
SPINLOCK_INITIALIZE(asidlock);

/**
 * Inactive address spaces (on all processors)
 * that have valid ASID.
 */
LIST_INITIALIZE(inactive_as_with_asid_list);

/** Kernel address space. */
as_t *AS_KERNEL = NULL;

NO_TRACE static int as_constructor(void *obj, unsigned int flags)
{
	as_t *as = (as_t *) obj;
	
	link_initialize(&as->inactive_as_with_asid_link);
	mutex_initialize(&as->lock, MUTEX_PASSIVE);
	
	return as_constructor_arch(as, flags);
}

NO_TRACE static size_t as_destructor(void *obj)
{
	return as_destructor_arch((as_t *) obj);
}

/** Initialize address space subsystem. */
void as_init(void)
{
	as_arch_init();
	
	as_slab = slab_cache_create("as_t", sizeof(as_t), 0,
	    as_constructor, as_destructor, SLAB_CACHE_MAGDEFERRED);
	
	AS_KERNEL = as_create(FLAG_AS_KERNEL);
	if (!AS_KERNEL)
		panic("Cannot create kernel address space.");
	
	/*
	 * Make sure the kernel address space
	 * reference count never drops to zero.
	 */
	as_hold(AS_KERNEL);
}

/** Create address space.
 *
 * @param flags Flags that influence the way in wich the address
 *              space is created.
 *
 */
as_t *as_create(unsigned int flags)
{
	as_t *as = (as_t *) slab_alloc(as_slab, 0);
	(void) as_create_arch(as, 0);
	
	btree_create(&as->as_area_btree);
	
	if (flags & FLAG_AS_KERNEL)
		as->asid = ASID_KERNEL;
	else
		as->asid = ASID_INVALID;
	
	atomic_set(&as->refcount, 0);
	as->cpu_refcount = 0;
	
#ifdef AS_PAGE_TABLE
	as->genarch.page_table = page_table_create(flags);
#else
	page_table_create(flags);
#endif
	
	return as;
}

/** Destroy adress space.
 *
 * When there are no tasks referencing this address space (i.e. its refcount is
 * zero), the address space can be destroyed.
 *
 * We know that we don't hold any spinlock.
 *
 * @param as Address space to be destroyed.
 *
 */
void as_destroy(as_t *as)
{
	DEADLOCK_PROBE_INIT(p_asidlock);
	
	ASSERT(as != AS);
	ASSERT(atomic_get(&as->refcount) == 0);
	
	/*
	 * Since there is no reference to this address space, it is safe not to
	 * lock its mutex.
	 */
	
	/*
	 * We need to avoid deadlock between TLB shootdown and asidlock.
	 * We therefore try to take asid conditionally and if we don't succeed,
	 * we enable interrupts and try again. This is done while preemption is
	 * disabled to prevent nested context switches. We also depend on the
	 * fact that so far no spinlocks are held.
	 */
	preemption_disable();
	ipl_t ipl = interrupts_read();
	
retry:
	interrupts_disable();
	if (!spinlock_trylock(&asidlock)) {
		interrupts_enable();
		DEADLOCK_PROBE(p_asidlock, DEADLOCK_THRESHOLD);
		goto retry;
	}
	
	/* Interrupts disabled, enable preemption */
	preemption_enable();
	
	if ((as->asid != ASID_INVALID) && (as != AS_KERNEL)) {
		if (as->cpu_refcount == 0)
			list_remove(&as->inactive_as_with_asid_link);
		
		asid_put(as->asid);
	}
	
	spinlock_unlock(&asidlock);
	interrupts_restore(ipl);
	
	
	/*
	 * Destroy address space areas of the address space.
	 * The B+tree must be walked carefully because it is
	 * also being destroyed.
	 */
	bool cond = true;
	while (cond) {
		ASSERT(!list_empty(&as->as_area_btree.leaf_list));
		
		btree_node_t *node =
		    list_get_instance(list_first(&as->as_area_btree.leaf_list),
		    btree_node_t, leaf_link);
		
		if ((cond = node->keys))
			as_area_destroy(as, node->key[0]);
	}
	
	btree_destroy(&as->as_area_btree);
	
#ifdef AS_PAGE_TABLE
	page_table_destroy(as->genarch.page_table);
#else
	page_table_destroy(NULL);
#endif
	
	slab_free(as_slab, as);
}

/** Hold a reference to an address space.
 *
 * Holding a reference to an address space prevents destruction
 * of that address space.
 *
 * @param as Address space to be held.
 *
 */
NO_TRACE void as_hold(as_t *as)
{
	atomic_inc(&as->refcount);
}

/** Release a reference to an address space.
 *
 * The last one to release a reference to an address space
 * destroys the address space.
 *
 * @param asAddress space to be released.
 *
 */
NO_TRACE void as_release(as_t *as)
{
	if (atomic_predec(&as->refcount) == 0)
		as_destroy(as);
}

/** Check area conflicts with other areas.
 *
 * @param as    Address space.
 * @param addr  Starting virtual address of the area being tested.
 * @param count Number of pages in the area being tested.
 * @param avoid Do not touch this area.
 *
 * @return True if there is no conflict, false otherwise.
 *
 */
NO_TRACE static bool check_area_conflicts(as_t *as, uintptr_t addr,
    size_t count, as_area_t *avoid)
{
	ASSERT((addr % PAGE_SIZE) == 0);
	ASSERT(mutex_locked(&as->lock));
	
	/*
	 * We don't want any area to have conflicts with NULL page.
	 */
	if (overlaps(addr, P2SZ(count), (uintptr_t) NULL, PAGE_SIZE))
		return false;
	
	/*
	 * The leaf node is found in O(log n), where n is proportional to
	 * the number of address space areas belonging to as.
	 * The check for conflicts is then attempted on the rightmost
	 * record in the left neighbour, the leftmost record in the right
	 * neighbour and all records in the leaf node itself.
	 */
	btree_node_t *leaf;
	as_area_t *area =
	    (as_area_t *) btree_search(&as->as_area_btree, addr, &leaf);
	if (area) {
		if (area != avoid)
			return false;
	}
	
	/* First, check the two border cases. */
	btree_node_t *node =
	    btree_leaf_node_left_neighbour(&as->as_area_btree, leaf);
	if (node) {
		area = (as_area_t *) node->value[node->keys - 1];
		
		if (area != avoid) {
			mutex_lock(&area->lock);
			
			if (overlaps(addr, P2SZ(count), area->base,
			    P2SZ(area->pages))) {
				mutex_unlock(&area->lock);
				return false;
			}
			
			mutex_unlock(&area->lock);
		}
	}
	
	node = btree_leaf_node_right_neighbour(&as->as_area_btree, leaf);
	if (node) {
		area = (as_area_t *) node->value[0];
		
		if (area != avoid) {
			mutex_lock(&area->lock);
			
			if (overlaps(addr, P2SZ(count), area->base,
			    P2SZ(area->pages))) {
				mutex_unlock(&area->lock);
				return false;
			}
			
			mutex_unlock(&area->lock);
		}
	}
	
	/* Second, check the leaf node. */
	btree_key_t i;
	for (i = 0; i < leaf->keys; i++) {
		area = (as_area_t *) leaf->value[i];
		
		if (area == avoid)
			continue;
		
		mutex_lock(&area->lock);
		
		if (overlaps(addr, P2SZ(count), area->base,
		    P2SZ(area->pages))) {
			mutex_unlock(&area->lock);
			return false;
		}
		
		mutex_unlock(&area->lock);
	}
	
	/*
	 * So far, the area does not conflict with other areas.
	 * Check if it doesn't conflict with kernel address space.
	 */
	if (!KERNEL_ADDRESS_SPACE_SHADOWED) {
		return !overlaps(addr, P2SZ(count), KERNEL_ADDRESS_SPACE_START,
		    KERNEL_ADDRESS_SPACE_END - KERNEL_ADDRESS_SPACE_START);
	}
	
	return true;
}

/** Return pointer to unmapped address space area
 *
 * The address space must be already locked when calling
 * this function.
 *
 * @param as    Address space.
 * @param bound Lowest address bound.
 * @param size  Requested size of the allocation.
 *
 * @return Address of the beginning of unmapped address space area.
 * @return -1 if no suitable address space area was found.
 *
 */
NO_TRACE static uintptr_t as_get_unmapped_area(as_t *as, uintptr_t bound,
    size_t size)
{
	ASSERT(mutex_locked(&as->lock));
	
	if (size == 0)
		return (uintptr_t) -1;
	
	/*
	 * Make sure we allocate from page-aligned
	 * address. Check for possible overflow in
	 * each step.
	 */
	
	size_t pages = SIZE2FRAMES(size);
	
	/*
	 * Find the lowest unmapped address aligned on the size
	 * boundary, not smaller than bound and of the required size.
	 */
	
	/* First check the bound address itself */
	uintptr_t addr = ALIGN_UP(bound, PAGE_SIZE);
	if ((addr >= bound) &&
	    (check_area_conflicts(as, addr, pages, NULL)))
		return addr;
	
	/* Eventually check the addresses behind each area */
	list_foreach(as->as_area_btree.leaf_list, cur) {
		btree_node_t *node =
		    list_get_instance(cur, btree_node_t, leaf_link);
		
		for (btree_key_t i = 0; i < node->keys; i++) {
			as_area_t *area = (as_area_t *) node->value[i];
			
			mutex_lock(&area->lock);
			
			addr =
			    ALIGN_UP(area->base + P2SZ(area->pages), PAGE_SIZE);
			bool avail =
			    ((addr >= bound) && (addr >= area->base) &&
			    (check_area_conflicts(as, addr, pages, area)));
			
			mutex_unlock(&area->lock);
			
			if (avail)
				return addr;
		}
	}
	
	/* No suitable address space area found */
	return (uintptr_t) -1;
}

/** Create address space area of common attributes.
 *
 * The created address space area is added to the target address space.
 *
 * @param as           Target address space.
 * @param flags        Flags of the area memory.
 * @param size         Size of area.
 * @param attrs        Attributes of the area.
 * @param backend      Address space area backend. NULL if no backend is used.
 * @param backend_data NULL or a pointer to an array holding two void *.
 * @param base         Starting virtual address of the area.
 *                     If set to -1, a suitable mappable area is found.
 * @param bound        Lowest address bound if base is set to -1.
 *                     Otherwise ignored.
 *
 * @return Address space area on success or NULL on failure.
 *
 */
as_area_t *as_area_create(as_t *as, unsigned int flags, size_t size,
    unsigned int attrs, mem_backend_t *backend,
    mem_backend_data_t *backend_data, uintptr_t *base, uintptr_t bound)
{
	if ((*base != (uintptr_t) -1) && ((*base % PAGE_SIZE) != 0))
		return NULL;
	
	if (size == 0)
		return NULL;
	
	size_t pages = SIZE2FRAMES(size);
	
	/* Writeable executable areas are not supported. */
	if ((flags & AS_AREA_EXEC) && (flags & AS_AREA_WRITE))
		return NULL;
	
	mutex_lock(&as->lock);
	
	if (*base == (uintptr_t) -1) {
		*base = as_get_unmapped_area(as, bound, size);
		if (*base == (uintptr_t) -1) {
			mutex_unlock(&as->lock);
			return NULL;
		}
	}
	
	if (!check_area_conflicts(as, *base, pages, NULL)) {
		mutex_unlock(&as->lock);
		return NULL;
	}
	
	as_area_t *area = (as_area_t *) malloc(sizeof(as_area_t), 0);
	
	mutex_initialize(&area->lock, MUTEX_PASSIVE);
	
	area->as = as;
	area->flags = flags;
	area->attributes = attrs;
	area->pages = pages;
	area->resident = 0;
	area->base = *base;
	area->sh_info = NULL;
	area->backend = backend;
	
	if (backend_data)
		area->backend_data = *backend_data;
	else
		memsetb(&area->backend_data, sizeof(area->backend_data), 0);
	
	if (area->backend && area->backend->create) {
		if (!area->backend->create(area)) {
			free(area);
			mutex_unlock(&as->lock);
			return NULL;
		}
	}
	
	btree_create(&area->used_space);
	btree_insert(&as->as_area_btree, *base, (void *) area,
	    NULL);
	
	mutex_unlock(&as->lock);
	
	return area;
}

/** Find address space area and lock it.
 *
 * @param as Address space.
 * @param va Virtual address.
 *
 * @return Locked address space area containing va on success or
 *         NULL on failure.
 *
 */
NO_TRACE static as_area_t *find_area_and_lock(as_t *as, uintptr_t va)
{
	ASSERT(mutex_locked(&as->lock));
	
	btree_node_t *leaf;
	as_area_t *area = (as_area_t *) btree_search(&as->as_area_btree, va,
	    &leaf);
	if (area) {
		/* va is the base address of an address space area */
		mutex_lock(&area->lock);
		return area;
	}
	
	/*
	 * Search the leaf node and the rightmost record of its left neighbour
	 * to find out whether this is a miss or va belongs to an address
	 * space area found there.
	 */
	
	/* First, search the leaf node itself. */
	btree_key_t i;
	
	for (i = 0; i < leaf->keys; i++) {
		area = (as_area_t *) leaf->value[i];
		
		mutex_lock(&area->lock);

		if ((area->base <= va) &&
		    (va <= area->base + (P2SZ(area->pages) - 1)))
			return area;
		
		mutex_unlock(&area->lock);
	}
	
	/*
	 * Second, locate the left neighbour and test its last record.
	 * Because of its position in the B+tree, it must have base < va.
	 */
	btree_node_t *lnode = btree_leaf_node_left_neighbour(&as->as_area_btree,
	    leaf);
	if (lnode) {
		area = (as_area_t *) lnode->value[lnode->keys - 1];
		
		mutex_lock(&area->lock);
		
		if (va <= area->base + (P2SZ(area->pages) - 1))
			return area;
		
		mutex_unlock(&area->lock);
	}
	
	return NULL;
}

/** Find address space area and change it.
 *
 * @param as      Address space.
 * @param address Virtual address belonging to the area to be changed.
 *                Must be page-aligned.
 * @param size    New size of the virtual memory block starting at
 *                address.
 * @param flags   Flags influencing the remap operation. Currently unused.
 *
 * @return Zero on success or a value from @ref errno.h otherwise.
 *
 */
int as_area_resize(as_t *as, uintptr_t address, size_t size, unsigned int flags)
{
	mutex_lock(&as->lock);
	
	/*
	 * Locate the area.
	 */
	as_area_t *area = find_area_and_lock(as, address);
	if (!area) {
		mutex_unlock(&as->lock);
		return ENOENT;
	}
	
	if (area->backend == &phys_backend) {
		/*
		 * Remapping of address space areas associated
		 * with memory mapped devices is not supported.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		return ENOTSUP;
	}
	
	if (area->sh_info) {
		/*
		 * Remapping of shared address space areas
		 * is not supported.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		return ENOTSUP;
	}
	
	size_t pages = SIZE2FRAMES((address - area->base) + size);
	if (!pages) {
		/*
		 * Zero size address space areas are not allowed.
		 */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		return EPERM;
	}
	
	if (pages < area->pages) {
		uintptr_t start_free = area->base + P2SZ(pages);
		
		/*
		 * Shrinking the area.
		 * No need to check for overlaps.
		 */
		
		page_table_lock(as, false);
		
		/*
		 * Remove frames belonging to used space starting from
		 * the highest addresses downwards until an overlap with
		 * the resized address space area is found. Note that this
		 * is also the right way to remove part of the used_space
		 * B+tree leaf list.
		 */
		bool cond = true;
		while (cond) {
			ASSERT(!list_empty(&area->used_space.leaf_list));
			
			btree_node_t *node =
			    list_get_instance(list_last(&area->used_space.leaf_list),
			    btree_node_t, leaf_link);
			
			if ((cond = (bool) node->keys)) {
				uintptr_t ptr = node->key[node->keys - 1];
				size_t size =
				    (size_t) node->value[node->keys - 1];
				size_t i = 0;
				
				if (overlaps(ptr, P2SZ(size), area->base,
				    P2SZ(pages))) {
					
					if (ptr + P2SZ(size) <= start_free) {
						/*
						 * The whole interval fits
						 * completely in the resized
						 * address space area.
						 */
						break;
					}
					
					/*
					 * Part of the interval corresponding
					 * to b and c overlaps with the resized
					 * address space area.
					 */
					
					/* We are almost done */
					cond = false;
					i = (start_free - ptr) >> PAGE_WIDTH;
					if (!used_space_remove(area, start_free,
					    size - i))
						panic("Cannot remove used space.");
				} else {
					/*
					 * The interval of used space can be
					 * completely removed.
					 */
					if (!used_space_remove(area, ptr, size))
						panic("Cannot remove used space.");
				}
				
				/*
				 * Start TLB shootdown sequence.
				 *
				 * The sequence is rather short and can be
				 * repeated multiple times. The reason is that
				 * we don't want to have used_space_remove()
				 * inside the sequence as it may use a blocking
				 * memory allocation for its B+tree. Blocking
				 * while holding the tlblock spinlock is
				 * forbidden and would hit a kernel assertion.
				 */

				ipl_t ipl = tlb_shootdown_start(TLB_INVL_PAGES,
				    as->asid, area->base + P2SZ(pages),
				    area->pages - pages);
		
				for (; i < size; i++) {
					pte_t *pte = page_mapping_find(as,
					    ptr + P2SZ(i), false);
					
					ASSERT(pte);
					ASSERT(PTE_VALID(pte));
					ASSERT(PTE_PRESENT(pte));
					
					if ((area->backend) &&
					    (area->backend->frame_free)) {
						area->backend->frame_free(area,
						    ptr + P2SZ(i),
						    PTE_GET_FRAME(pte));
					}
					
					page_mapping_remove(as, ptr + P2SZ(i));
				}
		
				/*
				 * Finish TLB shootdown sequence.
				 */
		
				tlb_invalidate_pages(as->asid,
				    area->base + P2SZ(pages),
				    area->pages - pages);
		
				/*
				 * Invalidate software translation caches
				 * (e.g. TSB on sparc64, PHT on ppc32).
				 */
				as_invalidate_translation_cache(as,
				    area->base + P2SZ(pages),
				    area->pages - pages);
				tlb_shootdown_finalize(ipl);
			}
		}
		page_table_unlock(as, false);
	} else {
		/*
		 * Growing the area.
		 * Check for overlaps with other address space areas.
		 */
		if (!check_area_conflicts(as, address, pages, area)) {
			mutex_unlock(&area->lock);
			mutex_unlock(&as->lock);
			return EADDRNOTAVAIL;
		}
	}
	
	if (area->backend && area->backend->resize) {
		if (!area->backend->resize(area, pages)) {
			mutex_unlock(&area->lock);
			mutex_unlock(&as->lock);
			return ENOMEM;
		}
	}
	
	area->pages = pages;
	
	mutex_unlock(&area->lock);
	mutex_unlock(&as->lock);
	
	return 0;
}

/** Remove reference to address space area share info.
 *
 * If the reference count drops to 0, the sh_info is deallocated.
 *
 * @param sh_info Pointer to address space area share info.
 *
 */
NO_TRACE static void sh_info_remove_reference(share_info_t *sh_info)
{
	bool dealloc = false;
	
	mutex_lock(&sh_info->lock);
	ASSERT(sh_info->refcount);
	
	if (--sh_info->refcount == 0) {
		dealloc = true;
		
		/*
		 * Now walk carefully the pagemap B+tree and free/remove
		 * reference from all frames found there.
		 */
		list_foreach(sh_info->pagemap.leaf_list, cur) {
			btree_node_t *node
			    = list_get_instance(cur, btree_node_t, leaf_link);
			btree_key_t i;
			
			for (i = 0; i < node->keys; i++)
				frame_free((uintptr_t) node->value[i]);
		}
		
	}
	mutex_unlock(&sh_info->lock);
	
	if (dealloc) {
		btree_destroy(&sh_info->pagemap);
		free(sh_info);
	}
}

/** Destroy address space area.
 *
 * @param as      Address space.
 * @param address Address within the area to be deleted.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int as_area_destroy(as_t *as, uintptr_t address)
{
	mutex_lock(&as->lock);
	
	as_area_t *area = find_area_and_lock(as, address);
	if (!area) {
		mutex_unlock(&as->lock);
		return ENOENT;
	}

	if (area->backend && area->backend->destroy)
		area->backend->destroy(area);
	
	uintptr_t base = area->base;
	
	page_table_lock(as, false);
	
	/*
	 * Start TLB shootdown sequence.
	 */
	ipl_t ipl = tlb_shootdown_start(TLB_INVL_PAGES, as->asid, area->base,
	    area->pages);
	
	/*
	 * Visit only the pages mapped by used_space B+tree.
	 */
	list_foreach(area->used_space.leaf_list, cur) {
		btree_node_t *node;
		btree_key_t i;
		
		node = list_get_instance(cur, btree_node_t, leaf_link);
		for (i = 0; i < node->keys; i++) {
			uintptr_t ptr = node->key[i];
			size_t size;
			
			for (size = 0; size < (size_t) node->value[i]; size++) {
				pte_t *pte = page_mapping_find(as,
				     ptr + P2SZ(size), false);
				
				ASSERT(pte);
				ASSERT(PTE_VALID(pte));
				ASSERT(PTE_PRESENT(pte));
				
				if ((area->backend) &&
				    (area->backend->frame_free)) {
					area->backend->frame_free(area,
					    ptr + P2SZ(size),
					    PTE_GET_FRAME(pte));
				}
				
				page_mapping_remove(as, ptr + P2SZ(size));
			}
		}
	}
	
	/*
	 * Finish TLB shootdown sequence.
	 */
	
	tlb_invalidate_pages(as->asid, area->base, area->pages);
	
	/*
	 * Invalidate potential software translation caches
	 * (e.g. TSB on sparc64, PHT on ppc32).
	 */
	as_invalidate_translation_cache(as, area->base, area->pages);
	tlb_shootdown_finalize(ipl);
	
	page_table_unlock(as, false);
	
	btree_destroy(&area->used_space);
	
	area->attributes |= AS_AREA_ATTR_PARTIAL;
	
	if (area->sh_info)
		sh_info_remove_reference(area->sh_info);
	
	mutex_unlock(&area->lock);
	
	/*
	 * Remove the empty area from address space.
	 */
	btree_remove(&as->as_area_btree, base, NULL);
	
	free(area);
	
	mutex_unlock(&as->lock);
	return 0;
}

/** Share address space area with another or the same address space.
 *
 * Address space area mapping is shared with a new address space area.
 * If the source address space area has not been shared so far,
 * a new sh_info is created. The new address space area simply gets the
 * sh_info of the source area. The process of duplicating the
 * mapping is done through the backend share function.
 *
 * @param src_as         Pointer to source address space.
 * @param src_base       Base address of the source address space area.
 * @param acc_size       Expected size of the source area.
 * @param dst_as         Pointer to destination address space.
 * @param dst_flags_mask Destination address space area flags mask.
 * @param dst_base       Target base address. If set to -1,
 *                       a suitable mappable area is found.
 * @param bound          Lowest address bound if dst_base is set to -1.
 *                       Otherwise ignored.
 *
 * @return Zero on success.
 * @return ENOENT if there is no such task or such address space.
 * @return EPERM if there was a problem in accepting the area.
 * @return ENOMEM if there was a problem in allocating destination
 *         address space area.
 * @return ENOTSUP if the address space area backend does not support
 *         sharing.
 *
 */
int as_area_share(as_t *src_as, uintptr_t src_base, size_t acc_size,
    as_t *dst_as, unsigned int dst_flags_mask, uintptr_t *dst_base,
    uintptr_t bound)
{
	mutex_lock(&src_as->lock);
	as_area_t *src_area = find_area_and_lock(src_as, src_base);
	if (!src_area) {
		/*
		 * Could not find the source address space area.
		 */
		mutex_unlock(&src_as->lock);
		return ENOENT;
	}
	
	if ((!src_area->backend) || (!src_area->backend->share)) {
		/*
		 * There is no backend or the backend does not
		 * know how to share the area.
		 */
		mutex_unlock(&src_area->lock);
		mutex_unlock(&src_as->lock);
		return ENOTSUP;
	}
	
	size_t src_size = P2SZ(src_area->pages);
	unsigned int src_flags = src_area->flags;
	mem_backend_t *src_backend = src_area->backend;
	mem_backend_data_t src_backend_data = src_area->backend_data;
	
	/* Share the cacheable flag from the original mapping */
	if (src_flags & AS_AREA_CACHEABLE)
		dst_flags_mask |= AS_AREA_CACHEABLE;
	
	if ((src_size != acc_size) ||
	    ((src_flags & dst_flags_mask) != dst_flags_mask)) {
		mutex_unlock(&src_area->lock);
		mutex_unlock(&src_as->lock);
		return EPERM;
	}
	
	/*
	 * Now we are committed to sharing the area.
	 * First, prepare the area for sharing.
	 * Then it will be safe to unlock it.
	 */
	share_info_t *sh_info = src_area->sh_info;
	if (!sh_info) {
		sh_info = (share_info_t *) malloc(sizeof(share_info_t), 0);
		mutex_initialize(&sh_info->lock, MUTEX_PASSIVE);
		sh_info->refcount = 2;
		btree_create(&sh_info->pagemap);
		src_area->sh_info = sh_info;
		
		/*
		 * Call the backend to setup sharing.
		 */
		src_area->backend->share(src_area);
	} else {
		mutex_lock(&sh_info->lock);
		sh_info->refcount++;
		mutex_unlock(&sh_info->lock);
	}
	
	mutex_unlock(&src_area->lock);
	mutex_unlock(&src_as->lock);
	
	/*
	 * Create copy of the source address space area.
	 * The destination area is created with AS_AREA_ATTR_PARTIAL
	 * attribute set which prevents race condition with
	 * preliminary as_page_fault() calls.
	 * The flags of the source area are masked against dst_flags_mask
	 * to support sharing in less privileged mode.
	 */
	as_area_t *dst_area = as_area_create(dst_as, dst_flags_mask,
	    src_size, AS_AREA_ATTR_PARTIAL, src_backend,
	    &src_backend_data, dst_base, bound);
	if (!dst_area) {
		/*
		 * Destination address space area could not be created.
		 */
		sh_info_remove_reference(sh_info);
		
		return ENOMEM;
	}
	
	/*
	 * Now the destination address space area has been
	 * fully initialized. Clear the AS_AREA_ATTR_PARTIAL
	 * attribute and set the sh_info.
	 */
	mutex_lock(&dst_as->lock);
	mutex_lock(&dst_area->lock);
	dst_area->attributes &= ~AS_AREA_ATTR_PARTIAL;
	dst_area->sh_info = sh_info;
	mutex_unlock(&dst_area->lock);
	mutex_unlock(&dst_as->lock);
	
	return 0;
}

/** Check access mode for address space area.
 *
 * @param area   Address space area.
 * @param access Access mode.
 *
 * @return False if access violates area's permissions, true
 *         otherwise.
 *
 */
NO_TRACE bool as_area_check_access(as_area_t *area, pf_access_t access)
{
	ASSERT(mutex_locked(&area->lock));
	
	int flagmap[] = {
		[PF_ACCESS_READ] = AS_AREA_READ,
		[PF_ACCESS_WRITE] = AS_AREA_WRITE,
		[PF_ACCESS_EXEC] = AS_AREA_EXEC
	};
	
	if (!(area->flags & flagmap[access]))
		return false;
	
	return true;
}

/** Convert address space area flags to page flags.
 *
 * @param aflags Flags of some address space area.
 *
 * @return Flags to be passed to page_mapping_insert().
 *
 */
NO_TRACE static unsigned int area_flags_to_page_flags(unsigned int aflags)
{
	unsigned int flags = PAGE_USER | PAGE_PRESENT;
	
	if (aflags & AS_AREA_READ)
		flags |= PAGE_READ;
		
	if (aflags & AS_AREA_WRITE)
		flags |= PAGE_WRITE;
	
	if (aflags & AS_AREA_EXEC)
		flags |= PAGE_EXEC;
	
	if (aflags & AS_AREA_CACHEABLE)
		flags |= PAGE_CACHEABLE;
	
	return flags;
}

/** Change adress space area flags.
 *
 * The idea is to have the same data, but with a different access mode.
 * This is needed e.g. for writing code into memory and then executing it.
 * In order for this to work properly, this may copy the data
 * into private anonymous memory (unless it's already there).
 *
 * @param as      Address space.
 * @param flags   Flags of the area memory.
 * @param address Address within the area to be changed.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int as_area_change_flags(as_t *as, unsigned int flags, uintptr_t address)
{
	/* Flags for the new memory mapping */
	unsigned int page_flags = area_flags_to_page_flags(flags);
	
	mutex_lock(&as->lock);
	
	as_area_t *area = find_area_and_lock(as, address);
	if (!area) {
		mutex_unlock(&as->lock);
		return ENOENT;
	}
	
	if ((area->sh_info) || (area->backend != &anon_backend)) {
		/* Copying shared areas not supported yet */
		/* Copying non-anonymous memory not supported yet */
		mutex_unlock(&area->lock);
		mutex_unlock(&as->lock);
		return ENOTSUP;
	}
	
	/*
	 * Compute total number of used pages in the used_space B+tree
	 */
	size_t used_pages = 0;
	
	list_foreach(area->used_space.leaf_list, cur) {
		btree_node_t *node
		    = list_get_instance(cur, btree_node_t, leaf_link);
		btree_key_t i;
		
		for (i = 0; i < node->keys; i++)
			used_pages += (size_t) node->value[i];
	}
	
	/* An array for storing frame numbers */
	uintptr_t *old_frame = malloc(used_pages * sizeof(uintptr_t), 0);
	
	page_table_lock(as, false);
	
	/*
	 * Start TLB shootdown sequence.
	 */
	ipl_t ipl = tlb_shootdown_start(TLB_INVL_PAGES, as->asid, area->base,
	    area->pages);
	
	/*
	 * Remove used pages from page tables and remember their frame
	 * numbers.
	 */
	size_t frame_idx = 0;
	
	list_foreach(area->used_space.leaf_list, cur) {
		btree_node_t *node = list_get_instance(cur, btree_node_t,
		    leaf_link);
		btree_key_t i;
		
		for (i = 0; i < node->keys; i++) {
			uintptr_t ptr = node->key[i];
			size_t size;
			
			for (size = 0; size < (size_t) node->value[i]; size++) {
				pte_t *pte = page_mapping_find(as,
				    ptr + P2SZ(size), false);
				
				ASSERT(pte);
				ASSERT(PTE_VALID(pte));
				ASSERT(PTE_PRESENT(pte));
				
				old_frame[frame_idx++] = PTE_GET_FRAME(pte);
				
				/* Remove old mapping */
				page_mapping_remove(as, ptr + P2SZ(size));
			}
		}
	}
	
	/*
	 * Finish TLB shootdown sequence.
	 */
	
	tlb_invalidate_pages(as->asid, area->base, area->pages);
	
	/*
	 * Invalidate potential software translation caches
	 * (e.g. TSB on sparc64, PHT on ppc32).
	 */
	as_invalidate_translation_cache(as, area->base, area->pages);
	tlb_shootdown_finalize(ipl);
	
	page_table_unlock(as, false);
	
	/*
	 * Set the new flags.
	 */
	area->flags = flags;
	
	/*
	 * Map pages back in with new flags. This step is kept separate
	 * so that the memory area could not be accesed with both the old and
	 * the new flags at once.
	 */
	frame_idx = 0;
	
	list_foreach(area->used_space.leaf_list, cur) {
		btree_node_t *node
		    = list_get_instance(cur, btree_node_t, leaf_link);
		btree_key_t i;
		
		for (i = 0; i < node->keys; i++) {
			uintptr_t ptr = node->key[i];
			size_t size;
			
			for (size = 0; size < (size_t) node->value[i]; size++) {
				page_table_lock(as, false);
				
				/* Insert the new mapping */
				page_mapping_insert(as, ptr + P2SZ(size),
				    old_frame[frame_idx++], page_flags);
				
				page_table_unlock(as, false);
			}
		}
	}
	
	free(old_frame);
	
	mutex_unlock(&area->lock);
	mutex_unlock(&as->lock);
	
	return 0;
}

/** Handle page fault within the current address space.
 *
 * This is the high-level page fault handler. It decides whether the page fault
 * can be resolved by any backend and if so, it invokes the backend to resolve
 * the page fault.
 *
 * Interrupts are assumed disabled.
 *
 * @param page   Faulting page.
 * @param access Access mode that caused the page fault (i.e.
 *               read/write/exec).
 * @param istate Pointer to the interrupted state.
 *
 * @return AS_PF_FAULT on page fault.
 * @return AS_PF_OK on success.
 * @return AS_PF_DEFER if the fault was caused by copy_to_uspace()
 *         or copy_from_uspace().
 *
 */
int as_page_fault(uintptr_t page, pf_access_t access, istate_t *istate)
{
	if (!THREAD)
		return AS_PF_FAULT;
	
	if (!AS)
		return AS_PF_FAULT;
	
	mutex_lock(&AS->lock);
	as_area_t *area = find_area_and_lock(AS, page);
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
	
	if ((!area->backend) || (!area->backend->page_fault)) {
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
	 * To avoid race condition between two page faults on the same address,
	 * we need to make sure the mapping has not been already inserted.
	 */
	pte_t *pte;
	if ((pte = page_mapping_find(AS, page, false))) {
		if (PTE_PRESENT(pte)) {
			if (((access == PF_ACCESS_READ) && PTE_READABLE(pte)) ||
			    (access == PF_ACCESS_WRITE && PTE_WRITABLE(pte)) ||
			    (access == PF_ACCESS_EXEC && PTE_EXECUTABLE(pte))) {
				page_table_unlock(AS, false);
				mutex_unlock(&area->lock);
				mutex_unlock(&AS->lock);
				return AS_PF_OK;
			}
		}
	}
	
	/*
	 * Resort to the backend page fault handler.
	 */
	if (area->backend->page_fault(area, page, access) != AS_PF_OK) {
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
		istate_set_retaddr(istate,
		    (uintptr_t) &memcpy_from_uspace_failover_address);
	} else if (THREAD->in_copy_to_uspace) {
		THREAD->in_copy_to_uspace = false;
		istate_set_retaddr(istate,
		    (uintptr_t) &memcpy_to_uspace_failover_address);
	} else {
		return AS_PF_FAULT;
	}
	
	return AS_PF_DEFER;
}

/** Switch address spaces.
 *
 * Note that this function cannot sleep as it is essentially a part of
 * scheduling. Sleeping here would lead to deadlock on wakeup. Another
 * thing which is forbidden in this context is locking the address space.
 *
 * When this function is entered, no spinlocks may be held.
 *
 * @param old Old address space or NULL.
 * @param new New address space.
 *
 */
void as_switch(as_t *old_as, as_t *new_as)
{
	DEADLOCK_PROBE_INIT(p_asidlock);
	preemption_disable();
	
retry:
	(void) interrupts_disable();
	if (!spinlock_trylock(&asidlock)) {
		/*
		 * Avoid deadlock with TLB shootdown.
		 * We can enable interrupts here because
		 * preemption is disabled. We should not be
		 * holding any other lock.
		 */
		(void) interrupts_enable();
		DEADLOCK_PROBE(p_asidlock, DEADLOCK_THRESHOLD);
		goto retry;
	}
	preemption_enable();
	
	/*
	 * First, take care of the old address space.
	 */
	if (old_as) {
		ASSERT(old_as->cpu_refcount);
		
		if ((--old_as->cpu_refcount == 0) && (old_as != AS_KERNEL)) {
			/*
			 * The old address space is no longer active on
			 * any processor. It can be appended to the
			 * list of inactive address spaces with assigned
			 * ASID.
			 */
			ASSERT(old_as->asid != ASID_INVALID);
			
			list_append(&old_as->inactive_as_with_asid_link,
			    &inactive_as_with_asid_list);
		}
		
		/*
		 * Perform architecture-specific tasks when the address space
		 * is being removed from the CPU.
		 */
		as_deinstall_arch(old_as);
	}
	
	/*
	 * Second, prepare the new address space.
	 */
	if ((new_as->cpu_refcount++ == 0) && (new_as != AS_KERNEL)) {
		if (new_as->asid != ASID_INVALID)
			list_remove(&new_as->inactive_as_with_asid_link);
		else
			new_as->asid = asid_get();
	}
	
#ifdef AS_PAGE_TABLE
	SET_PTL0_ADDRESS(new_as->genarch.page_table);
#endif
	
	/*
	 * Perform architecture-specific steps.
	 * (e.g. write ASID to hardware register etc.)
	 */
	as_install_arch(new_as);
	
	spinlock_unlock(&asidlock);
	
	AS = new_as;
}

/** Compute flags for virtual address translation subsytem.
 *
 * @param area Address space area.
 *
 * @return Flags to be used in page_mapping_insert().
 *
 */
NO_TRACE unsigned int as_area_get_flags(as_area_t *area)
{
	ASSERT(mutex_locked(&area->lock));
	
	return area_flags_to_page_flags(area->flags);
}

/** Create page table.
 *
 * Depending on architecture, create either address space private or global page
 * table.
 *
 * @param flags Flags saying whether the page table is for the kernel
 *              address space.
 *
 * @return First entry of the page table.
 *
 */
NO_TRACE pte_t *page_table_create(unsigned int flags)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_create);
	
	return as_operations->page_table_create(flags);
}

/** Destroy page table.
 *
 * Destroy page table in architecture specific way.
 *
 * @param page_table Physical address of PTL0.
 *
 */
NO_TRACE void page_table_destroy(pte_t *page_table)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_destroy);
	
	as_operations->page_table_destroy(page_table);
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
 * @param as   Address space.
 * @param lock If false, do not attempt to lock as->lock.
 *
 */
NO_TRACE void page_table_lock(as_t *as, bool lock)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_lock);
	
	as_operations->page_table_lock(as, lock);
}

/** Unlock page table.
 *
 * @param as     Address space.
 * @param unlock If false, do not attempt to unlock as->lock.
 *
 */
NO_TRACE void page_table_unlock(as_t *as, bool unlock)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_unlock);
	
	as_operations->page_table_unlock(as, unlock);
}

/** Test whether page tables are locked.
 *
 * @param as Address space where the page tables belong.
 *
 * @return True if the page tables belonging to the address soace
 *         are locked, otherwise false.
 */
NO_TRACE bool page_table_locked(as_t *as)
{
	ASSERT(as_operations);
	ASSERT(as_operations->page_table_locked);

	return as_operations->page_table_locked(as);
}

/** Return size of the address space area with given base.
 *
 * @param base Arbitrary address inside the address space area.
 *
 * @return Size of the address space area in bytes or zero if it
 *         does not exist.
 *
 */
size_t as_area_get_size(uintptr_t base)
{
	size_t size;
	
	page_table_lock(AS, true);
	as_area_t *src_area = find_area_and_lock(AS, base);
	
	if (src_area) {
		size = P2SZ(src_area->pages);
		mutex_unlock(&src_area->lock);
	} else
		size = 0;
	
	page_table_unlock(AS, true);
	return size;
}

/** Mark portion of address space area as used.
 *
 * The address space area must be already locked.
 *
 * @param area  Address space area.
 * @param page  First page to be marked.
 * @param count Number of page to be marked.
 *
 * @return False on failure or true on success.
 *
 */
bool used_space_insert(as_area_t *area, uintptr_t page, size_t count)
{
	ASSERT(mutex_locked(&area->lock));
	ASSERT(page == ALIGN_DOWN(page, PAGE_SIZE));
	ASSERT(count);
	
	btree_node_t *leaf;
	size_t pages = (size_t) btree_search(&area->used_space, page, &leaf);
	if (pages) {
		/*
		 * We hit the beginning of some used space.
		 */
		return false;
	}
	
	if (!leaf->keys) {
		btree_insert(&area->used_space, page, (void *) count, leaf);
		goto success;
	}
	
	btree_node_t *node = btree_leaf_node_left_neighbour(&area->used_space, leaf);
	if (node) {
		uintptr_t left_pg = node->key[node->keys - 1];
		uintptr_t right_pg = leaf->key[0];
		size_t left_cnt = (size_t) node->value[node->keys - 1];
		size_t right_cnt = (size_t) leaf->value[0];
		
		/*
		 * Examine the possibility that the interval fits
		 * somewhere between the rightmost interval of
		 * the left neigbour and the first interval of the leaf.
		 */
		
		if (page >= right_pg) {
			/* Do nothing. */
		} else if (overlaps(page, P2SZ(count), left_pg,
		    P2SZ(left_cnt))) {
			/* The interval intersects with the left interval. */
			return false;
		} else if (overlaps(page, P2SZ(count), right_pg,
		    P2SZ(right_cnt))) {
			/* The interval intersects with the right interval. */
			return false;
		} else if ((page == left_pg + P2SZ(left_cnt)) &&
		    (page + P2SZ(count) == right_pg)) {
			/*
			 * The interval can be added by merging the two already
			 * present intervals.
			 */
			node->value[node->keys - 1] += count + right_cnt;
			btree_remove(&area->used_space, right_pg, leaf);
			goto success;
		} else if (page == left_pg + P2SZ(left_cnt)) {
			/*
			 * The interval can be added by simply growing the left
			 * interval.
			 */
			node->value[node->keys - 1] += count;
			goto success;
		} else if (page + P2SZ(count) == right_pg) {
			/*
			 * The interval can be addded by simply moving base of
			 * the right interval down and increasing its size
			 * accordingly.
			 */
			leaf->value[0] += count;
			leaf->key[0] = page;
			goto success;
		} else {
			/*
			 * The interval is between both neigbouring intervals,
			 * but cannot be merged with any of them.
			 */
			btree_insert(&area->used_space, page, (void *) count,
			    leaf);
			goto success;
		}
	} else if (page < leaf->key[0]) {
		uintptr_t right_pg = leaf->key[0];
		size_t right_cnt = (size_t) leaf->value[0];
		
		/*
		 * Investigate the border case in which the left neighbour does
		 * not exist but the interval fits from the left.
		 */
		
		if (overlaps(page, P2SZ(count), right_pg, P2SZ(right_cnt))) {
			/* The interval intersects with the right interval. */
			return false;
		} else if (page + P2SZ(count) == right_pg) {
			/*
			 * The interval can be added by moving the base of the
			 * right interval down and increasing its size
			 * accordingly.
			 */
			leaf->key[0] = page;
			leaf->value[0] += count;
			goto success;
		} else {
			/*
			 * The interval doesn't adjoin with the right interval.
			 * It must be added individually.
			 */
			btree_insert(&area->used_space, page, (void *) count,
			    leaf);
			goto success;
		}
	}
	
	node = btree_leaf_node_right_neighbour(&area->used_space, leaf);
	if (node) {
		uintptr_t left_pg = leaf->key[leaf->keys - 1];
		uintptr_t right_pg = node->key[0];
		size_t left_cnt = (size_t) leaf->value[leaf->keys - 1];
		size_t right_cnt = (size_t) node->value[0];
		
		/*
		 * Examine the possibility that the interval fits
		 * somewhere between the leftmost interval of
		 * the right neigbour and the last interval of the leaf.
		 */
		
		if (page < left_pg) {
			/* Do nothing. */
		} else if (overlaps(page, P2SZ(count), left_pg,
		    P2SZ(left_cnt))) {
			/* The interval intersects with the left interval. */
			return false;
		} else if (overlaps(page, P2SZ(count), right_pg,
		    P2SZ(right_cnt))) {
			/* The interval intersects with the right interval. */
			return false;
		} else if ((page == left_pg + P2SZ(left_cnt)) &&
		    (page + P2SZ(count) == right_pg)) {
			/*
			 * The interval can be added by merging the two already
			 * present intervals.
			 */
			leaf->value[leaf->keys - 1] += count + right_cnt;
			btree_remove(&area->used_space, right_pg, node);
			goto success;
		} else if (page == left_pg + P2SZ(left_cnt)) {
			/*
			 * The interval can be added by simply growing the left
			 * interval.
			 */
			leaf->value[leaf->keys - 1] += count;
			goto success;
		} else if (page + P2SZ(count) == right_pg) {
			/*
			 * The interval can be addded by simply moving base of
			 * the right interval down and increasing its size
			 * accordingly.
			 */
			node->value[0] += count;
			node->key[0] = page;
			goto success;
		} else {
			/*
			 * The interval is between both neigbouring intervals,
			 * but cannot be merged with any of them.
			 */
			btree_insert(&area->used_space, page, (void *) count,
			    leaf);
			goto success;
		}
	} else if (page >= leaf->key[leaf->keys - 1]) {
		uintptr_t left_pg = leaf->key[leaf->keys - 1];
		size_t left_cnt = (size_t) leaf->value[leaf->keys - 1];
		
		/*
		 * Investigate the border case in which the right neighbour
		 * does not exist but the interval fits from the right.
		 */
		
		if (overlaps(page, P2SZ(count), left_pg, P2SZ(left_cnt))) {
			/* The interval intersects with the left interval. */
			return false;
		} else if (left_pg + P2SZ(left_cnt) == page) {
			/*
			 * The interval can be added by growing the left
			 * interval.
			 */
			leaf->value[leaf->keys - 1] += count;
			goto success;
		} else {
			/*
			 * The interval doesn't adjoin with the left interval.
			 * It must be added individually.
			 */
			btree_insert(&area->used_space, page, (void *) count,
			    leaf);
			goto success;
		}
	}
	
	/*
	 * Note that if the algorithm made it thus far, the interval can fit
	 * only between two other intervals of the leaf. The two border cases
	 * were already resolved.
	 */
	btree_key_t i;
	for (i = 1; i < leaf->keys; i++) {
		if (page < leaf->key[i]) {
			uintptr_t left_pg = leaf->key[i - 1];
			uintptr_t right_pg = leaf->key[i];
			size_t left_cnt = (size_t) leaf->value[i - 1];
			size_t right_cnt = (size_t) leaf->value[i];
			
			/*
			 * The interval fits between left_pg and right_pg.
			 */
			
			if (overlaps(page, P2SZ(count), left_pg,
			    P2SZ(left_cnt))) {
				/*
				 * The interval intersects with the left
				 * interval.
				 */
				return false;
			} else if (overlaps(page, P2SZ(count), right_pg,
			    P2SZ(right_cnt))) {
				/*
				 * The interval intersects with the right
				 * interval.
				 */
				return false;
			} else if ((page == left_pg + P2SZ(left_cnt)) &&
			    (page + P2SZ(count) == right_pg)) {
				/*
				 * The interval can be added by merging the two
				 * already present intervals.
				 */
				leaf->value[i - 1] += count + right_cnt;
				btree_remove(&area->used_space, right_pg, leaf);
				goto success;
			} else if (page == left_pg + P2SZ(left_cnt)) {
				/*
				 * The interval can be added by simply growing
				 * the left interval.
				 */
				leaf->value[i - 1] += count;
				goto success;
			} else if (page + P2SZ(count) == right_pg) {
				/*
				 * The interval can be addded by simply moving
				 * base of the right interval down and
				 * increasing its size accordingly.
				 */
				leaf->value[i] += count;
				leaf->key[i] = page;
				goto success;
			} else {
				/*
				 * The interval is between both neigbouring
				 * intervals, but cannot be merged with any of
				 * them.
				 */
				btree_insert(&area->used_space, page,
				    (void *) count, leaf);
				goto success;
			}
		}
	}
	
	panic("Inconsistency detected while adding %zu pages of used "
	    "space at %p.", count, (void *) page);
	
success:
	area->resident += count;
	return true;
}

/** Mark portion of address space area as unused.
 *
 * The address space area must be already locked.
 *
 * @param area  Address space area.
 * @param page  First page to be marked.
 * @param count Number of page to be marked.
 *
 * @return False on failure or true on success.
 *
 */
bool used_space_remove(as_area_t *area, uintptr_t page, size_t count)
{
	ASSERT(mutex_locked(&area->lock));
	ASSERT(page == ALIGN_DOWN(page, PAGE_SIZE));
	ASSERT(count);
	
	btree_node_t *leaf;
	size_t pages = (size_t) btree_search(&area->used_space, page, &leaf);
	if (pages) {
		/*
		 * We are lucky, page is the beginning of some interval.
		 */
		if (count > pages) {
			return false;
		} else if (count == pages) {
			btree_remove(&area->used_space, page, leaf);
			goto success;
		} else {
			/*
			 * Find the respective interval.
			 * Decrease its size and relocate its start address.
			 */
			btree_key_t i;
			for (i = 0; i < leaf->keys; i++) {
				if (leaf->key[i] == page) {
					leaf->key[i] += P2SZ(count);
					leaf->value[i] -= count;
					goto success;
				}
			}
			
			goto error;
		}
	}
	
	btree_node_t *node = btree_leaf_node_left_neighbour(&area->used_space,
	    leaf);
	if ((node) && (page < leaf->key[0])) {
		uintptr_t left_pg = node->key[node->keys - 1];
		size_t left_cnt = (size_t) node->value[node->keys - 1];
		
		if (overlaps(left_pg, P2SZ(left_cnt), page, P2SZ(count))) {
			if (page + P2SZ(count) == left_pg + P2SZ(left_cnt)) {
				/*
				 * The interval is contained in the rightmost
				 * interval of the left neighbour and can be
				 * removed by updating the size of the bigger
				 * interval.
				 */
				node->value[node->keys - 1] -= count;
				goto success;
			} else if (page + P2SZ(count) <
			    left_pg + P2SZ(left_cnt)) {
				size_t new_cnt;

				/*
				 * The interval is contained in the rightmost
				 * interval of the left neighbour but its
				 * removal requires both updating the size of
				 * the original interval and also inserting a
				 * new interval.
				 */
				new_cnt = ((left_pg + P2SZ(left_cnt)) -
				    (page + P2SZ(count))) >> PAGE_WIDTH;
				node->value[node->keys - 1] -= count + new_cnt;
				btree_insert(&area->used_space, page +
				    P2SZ(count), (void *) new_cnt, leaf);
				goto success;
			}
		}
		
		return false;
	} else if (page < leaf->key[0])
		return false;
	
	if (page > leaf->key[leaf->keys - 1]) {
		uintptr_t left_pg = leaf->key[leaf->keys - 1];
		size_t left_cnt = (size_t) leaf->value[leaf->keys - 1];
		
		if (overlaps(left_pg, P2SZ(left_cnt), page, P2SZ(count))) {
			if (page + P2SZ(count) == left_pg + P2SZ(left_cnt)) {
				/*
				 * The interval is contained in the rightmost
				 * interval of the leaf and can be removed by
				 * updating the size of the bigger interval.
				 */
				leaf->value[leaf->keys - 1] -= count;
				goto success;
			} else if (page + P2SZ(count) < left_pg +
			    P2SZ(left_cnt)) {
				size_t new_cnt;

				/*
				 * The interval is contained in the rightmost
				 * interval of the leaf but its removal
				 * requires both updating the size of the
				 * original interval and also inserting a new
				 * interval.
				 */
				new_cnt = ((left_pg + P2SZ(left_cnt)) -
				    (page + P2SZ(count))) >> PAGE_WIDTH;
				leaf->value[leaf->keys - 1] -= count + new_cnt;
				btree_insert(&area->used_space, page +
				    P2SZ(count), (void *) new_cnt, leaf);
				goto success;
			}
		}
		
		return false;
	}
	
	/*
	 * The border cases have been already resolved.
	 * Now the interval can be only between intervals of the leaf.
	 */
	btree_key_t i;
	for (i = 1; i < leaf->keys - 1; i++) {
		if (page < leaf->key[i]) {
			uintptr_t left_pg = leaf->key[i - 1];
			size_t left_cnt = (size_t) leaf->value[i - 1];
			
			/*
			 * Now the interval is between intervals corresponding
			 * to (i - 1) and i.
			 */
			if (overlaps(left_pg, P2SZ(left_cnt), page,
			    P2SZ(count))) {
				if (page + P2SZ(count) ==
				    left_pg + P2SZ(left_cnt)) {
					/*
					 * The interval is contained in the
					 * interval (i - 1) of the leaf and can
					 * be removed by updating the size of
					 * the bigger interval.
					 */
					leaf->value[i - 1] -= count;
					goto success;
				} else if (page + P2SZ(count) <
				    left_pg + P2SZ(left_cnt)) {
					size_t new_cnt;

					/*
					 * The interval is contained in the
					 * interval (i - 1) of the leaf but its
					 * removal requires both updating the
					 * size of the original interval and
					 * also inserting a new interval.
					 */
					new_cnt = ((left_pg + P2SZ(left_cnt)) -
					    (page + P2SZ(count))) >>
					    PAGE_WIDTH;
					leaf->value[i - 1] -= count + new_cnt;
					btree_insert(&area->used_space, page +
					    P2SZ(count), (void *) new_cnt,
					    leaf);
					goto success;
				}
			}
			
			return false;
		}
	}
	
error:
	panic("Inconsistency detected while removing %zu pages of used "
	    "space from %p.", count, (void *) page);
	
success:
	area->resident -= count;
	return true;
}

/*
 * Address space related syscalls.
 */

sysarg_t sys_as_area_create(uintptr_t base, size_t size, unsigned int flags,
    uintptr_t bound)
{
	uintptr_t virt = base;
	as_area_t *area = as_area_create(AS, flags | AS_AREA_CACHEABLE, size,
	    AS_AREA_ATTR_NONE, &anon_backend, NULL, &virt, bound);
	if (area == NULL)
		return (sysarg_t) -1;
	
	return (sysarg_t) virt;
}

sysarg_t sys_as_area_resize(uintptr_t address, size_t size, unsigned int flags)
{
	return (sysarg_t) as_area_resize(AS, address, size, 0);
}

sysarg_t sys_as_area_change_flags(uintptr_t address, unsigned int flags)
{
	return (sysarg_t) as_area_change_flags(AS, flags, address);
}

sysarg_t sys_as_area_destroy(uintptr_t address)
{
	return (sysarg_t) as_area_destroy(AS, address);
}

/** Get list of adress space areas.
 *
 * @param as    Address space.
 * @param obuf  Place to save pointer to returned buffer.
 * @param osize Place to save size of returned buffer.
 *
 */
void as_get_area_info(as_t *as, as_area_info_t **obuf, size_t *osize)
{
	mutex_lock(&as->lock);
	
	/* First pass, count number of areas. */
	
	size_t area_cnt = 0;
	
	list_foreach(as->as_area_btree.leaf_list, cur) {
		btree_node_t *node =
		    list_get_instance(cur, btree_node_t, leaf_link);
		area_cnt += node->keys;
	}
	
	size_t isize = area_cnt * sizeof(as_area_info_t);
	as_area_info_t *info = malloc(isize, 0);
	
	/* Second pass, record data. */
	
	size_t area_idx = 0;
	
	list_foreach(as->as_area_btree.leaf_list, cur) {
		btree_node_t *node =
		    list_get_instance(cur, btree_node_t, leaf_link);
		btree_key_t i;
		
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];
			
			ASSERT(area_idx < area_cnt);
			mutex_lock(&area->lock);
			
			info[area_idx].start_addr = area->base;
			info[area_idx].size = P2SZ(area->pages);
			info[area_idx].flags = area->flags;
			++area_idx;
			
			mutex_unlock(&area->lock);
		}
	}
	
	mutex_unlock(&as->lock);
	
	*obuf = info;
	*osize = isize;
}

/** Print out information about address space.
 *
 * @param as Address space.
 *
 */
void as_print(as_t *as)
{
	mutex_lock(&as->lock);
	
	/* Print out info about address space areas */
	list_foreach(as->as_area_btree.leaf_list, cur) {
		btree_node_t *node
		    = list_get_instance(cur, btree_node_t, leaf_link);
		btree_key_t i;
		
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];
			
			mutex_lock(&area->lock);
			printf("as_area: %p, base=%p, pages=%zu"
			    " (%p - %p)\n", area, (void *) area->base,
			    area->pages, (void *) area->base,
			    (void *) (area->base + P2SZ(area->pages)));
			mutex_unlock(&area->lock);
		}
	}
	
	mutex_unlock(&as->lock);
}

/** @}
 */
