/*
 * Copyright (c) 2001-2006 Jakub Jermar
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
 * @brief Virtual Address Translation subsystem.
 *
 * This file contains code for creating, destroying and searching
 * mappings between virtual addresses and physical addresses.
 * Functions here are mere wrappers that call the real implementation.
 * They however, define the single interface.
 *
 */

/*
 * Note on memory prefetching and updating memory mappings, also described in:
 * AMD x86-64 Architecture Programmer's Manual, Volume 2, System Programming,
 * 7.2.1 Special Coherency Considerations.
 *
 * The processor which modifies a page table mapping can access prefetched data
 * from the old mapping.  In order to prevent this, we place a memory barrier
 * after a mapping is updated.
 *
 * We assume that the other processors are either not using the mapping yet
 * (i.e. during the bootstrap) or are executing the TLB shootdown code.  While
 * we don't care much about the former case, the processors in the latter case 
 * will do an implicit serialization by virtue of running the TLB shootdown
 * interrupt handler.
 *
 */

#include <mm/page.h>
#include <genarch/mm/page_ht.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <arch/barrier.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memstr.h>
#include <debug.h>
#include <arch.h>
#include <syscall/copy.h>
#include <errno.h>
#include <align.h>

/** Virtual operations for page subsystem. */
page_mapping_operations_t *page_mapping_operations = NULL;

void page_init(void)
{
	page_arch_init();
}

/** Map memory structure
 *
 * Identity-map memory structure
 * considering possible crossings
 * of page boundaries.
 *
 * @param addr Address of the structure.
 * @param size Size of the structure.
 *
 */
void map_structure(uintptr_t addr, size_t size)
{
	size_t length = size + (addr - (addr & ~(PAGE_SIZE - 1)));
	size_t cnt = length / PAGE_SIZE + (length % PAGE_SIZE > 0);
	
	size_t i;
	for (i = 0; i < cnt; i++)
		page_mapping_insert(AS_KERNEL, addr + i * PAGE_SIZE,
		    addr + i * PAGE_SIZE, PAGE_NOT_CACHEABLE | PAGE_WRITE);
	
	/* Repel prefetched accesses to the old mapping. */
	memory_barrier();
}

/** Insert mapping of page to frame.
 *
 * Map virtual address page to physical address frame
 * using flags. Allocate and setup any missing page tables.
 *
 * @param as    Address space to which page belongs.
 * @param page  Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is
 *              done.
 * @param flags Flags to be used for mapping.
 *
 */
NO_TRACE void page_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame,
    unsigned int flags)
{
	ASSERT(page_table_locked(as));
	
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_insert);

	page_mapping_operations->mapping_insert(as, page, frame, flags);
	
	/* Repel prefetched accesses to the old mapping. */
	memory_barrier();
}

/** Remove mapping of page.
 *
 * Remove any mapping of page within address space as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * @param as   Address space to which page belongs.
 * @param page Virtual address of the page to be demapped.
 *
 */
NO_TRACE void page_mapping_remove(as_t *as, uintptr_t page)
{
	ASSERT(page_table_locked(as));
	
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_remove);
	
	page_mapping_operations->mapping_remove(as, page);
	
	/* Repel prefetched accesses to the old mapping. */
	memory_barrier();
}

/** Find mapping for virtual page.
 *
 * @param as     Address space to which page belongs.
 * @param page   Virtual page.
 * @param nolock True if the page tables need not be locked.
 *
 * @return NULL if there is no such mapping; requested mapping
 *         otherwise.
 *
 */
NO_TRACE pte_t *page_mapping_find(as_t *as, uintptr_t page, bool nolock)
{
	ASSERT(nolock || page_table_locked(as));
	
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_find);
	
	return page_mapping_operations->mapping_find(as, page, nolock);
}

uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	uintptr_t virtaddr = (uintptr_t) NULL;	// FIXME
	pfn_t i;

	page_table_lock(AS_KERNEL, true);
	for (i = 0; i < ADDR2PFN(ALIGN_UP(size, PAGE_SIZE)); i++) {
		uintptr_t addr = PFN2ADDR(i);
		page_mapping_insert(AS_KERNEL, virtaddr + addr, physaddr + addr,
		    PAGE_NOT_CACHEABLE | PAGE_WRITE);
	}
	page_table_unlock(AS_KERNEL, true);
	
	return virtaddr;
}


/** Syscall wrapper for getting mapping of a virtual page.
 * 
 * @retval EOK Everything went find, @p uspace_frame and @p uspace_node
 *             contains correct values.
 * @retval ENOENT Virtual address has no mapping.
 */
sysarg_t sys_page_find_mapping(uintptr_t virt_address,
    uintptr_t *uspace_frame)
{
	mutex_lock(&AS->lock);
	
	pte_t *pte = page_mapping_find(AS, virt_address, false);
	if (!PTE_VALID(pte) || !PTE_PRESENT(pte)) {
		mutex_unlock(&AS->lock);
		
		return (sysarg_t) ENOENT;
	}
	
	uintptr_t phys_address = PTE_GET_FRAME(pte);
	
	mutex_unlock(&AS->lock);
	
	int rc = copy_to_uspace(uspace_frame,
	    &phys_address, sizeof(phys_address));
	if (rc != EOK) {
		return (sysarg_t) rc;
	}
	
	return EOK;
}

/** @}
 */
