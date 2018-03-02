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
#include <arch.h>
#include <assert.h>
#include <syscall/copy.h>
#include <errno.h>
#include <align.h>

/** Virtual operations for page subsystem. */
page_mapping_operations_t *page_mapping_operations = NULL;

void page_init(void)
{
	page_arch_init();
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
	assert(page_table_locked(as));

	assert(page_mapping_operations);
	assert(page_mapping_operations->mapping_insert);

	page_mapping_operations->mapping_insert(as, ALIGN_DOWN(page, PAGE_SIZE),
	    ALIGN_DOWN(frame, FRAME_SIZE), flags);

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
	assert(page_table_locked(as));

	assert(page_mapping_operations);
	assert(page_mapping_operations->mapping_remove);

	page_mapping_operations->mapping_remove(as,
	    ALIGN_DOWN(page, PAGE_SIZE));

	/* Repel prefetched accesses to the old mapping. */
	memory_barrier();
}

/** Find mapping for virtual page.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param[out] pte Structure that will receive a copy of the found PTE.
 *
 * @return True if a valid PTE is returned, false otherwise. Note that
 *         the PTE is not guaranteed to be present.
 */
NO_TRACE bool page_mapping_find(as_t *as, uintptr_t page, bool nolock,
    pte_t *pte)
{
	assert(nolock || page_table_locked(as));

	assert(page_mapping_operations);
	assert(page_mapping_operations->mapping_find);

	return page_mapping_operations->mapping_find(as,
	    ALIGN_DOWN(page, PAGE_SIZE), nolock, pte);
}

/** Update mapping for virtual page.
 *
 * Use only to update accessed and modified/dirty bits.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param pte      New PTE.
 */
NO_TRACE void page_mapping_update(as_t *as, uintptr_t page, bool nolock,
    pte_t *pte)
{
	assert(nolock || page_table_locked(as));

	assert(page_mapping_operations);
	assert(page_mapping_operations->mapping_find);

	page_mapping_operations->mapping_update(as,
	    ALIGN_DOWN(page, PAGE_SIZE), nolock, pte);
}

/** Make the mapping shared by all page tables (not address spaces).
 *
 * @param base Starting virtual address of the range that is made global.
 * @param size Size of the address range that is made global.
 */
void page_mapping_make_global(uintptr_t base, size_t size)
{
	assert(page_mapping_operations);
	assert(page_mapping_operations->mapping_make_global);

	return page_mapping_operations->mapping_make_global(base, size);
}

errno_t page_find_mapping(uintptr_t virt, uintptr_t *phys)
{
	page_table_lock(AS, true);

	pte_t pte;
	bool found = page_mapping_find(AS, virt, false, &pte);
	if (!found || !PTE_VALID(&pte) || !PTE_PRESENT(&pte)) {
		page_table_unlock(AS, true);
		return ENOENT;
	}

	*phys = PTE_GET_FRAME(&pte) +
	    (virt - ALIGN_DOWN(virt, PAGE_SIZE));

	page_table_unlock(AS, true);

	return EOK;
}

/** Syscall wrapper for getting mapping of a virtual page.
 *
 * @return EOK on success.
 * @return ENOENT if no virtual address mapping found.
 *
 */
sys_errno_t sys_page_find_mapping(uintptr_t virt, uintptr_t *phys_ptr)
{
	uintptr_t phys;
	errno_t rc = page_find_mapping(virt, &phys);
	if (rc != EOK)
		return rc;

	rc = copy_to_uspace(phys_ptr, &phys, sizeof(phys));
	return (sys_errno_t) rc;
}

/** @}
 */
