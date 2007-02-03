/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genarchmm
 * @{
 */

/**
 * @file
 * @brief	Virtual Address Translation for hierarchical 4-level page tables.
 */

#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <memstr.h>

static void pt_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame, int flags);
static void pt_mapping_remove(as_t *as, uintptr_t page);
static pte_t *pt_mapping_find(as_t *as, uintptr_t page);

page_mapping_operations_t pt_mapping_operations = {
	.mapping_insert = pt_mapping_insert,
	.mapping_remove = pt_mapping_remove,
	.mapping_find = pt_mapping_find
};

/** Map page to frame using hierarchical page tables.
 *
 * Map virtual address page to physical address frame
 * using flags.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 */
void pt_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame, int flags)
{
	pte_t *ptl0, *ptl1, *ptl2, *ptl3;
	pte_t *newpt;

	ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = (pte_t *)frame_alloc(ONE_FRAME, FRAME_KA);
		memsetb((uintptr_t)newpt, PAGE_SIZE, 0);
		SET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page), KA2PA(newpt));
		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE | PAGE_WRITE);
	}

	ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = (pte_t *)frame_alloc(ONE_FRAME, FRAME_KA);
		memsetb((uintptr_t)newpt, PAGE_SIZE, 0);
		SET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page), KA2PA(newpt));
		SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE | PAGE_WRITE);
	}

	ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = (pte_t *)frame_alloc(ONE_FRAME, FRAME_KA);
		memsetb((uintptr_t)newpt, PAGE_SIZE, 0);
		SET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page), KA2PA(newpt));
		SET_PTL3_FLAGS(ptl2, PTL2_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE | PAGE_WRITE);
	}

	ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(page), frame);
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), flags);
}

/** Remove mapping of page from hierarchical page tables.
 *
 * Remove any mapping of page within address space as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * Empty page tables except PTL0 are freed.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual address of the page to be demapped.
 */
void pt_mapping_remove(as_t *as, uintptr_t page)
{
	pte_t *ptl0, *ptl1, *ptl2, *ptl3;
	bool empty = true;
	int i;

	/*
	 * First, remove the mapping, if it exists.
	 */

	ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	/* Destroy the mapping. Setting to PAGE_NOT_PRESENT is not sufficient. */
	memsetb((uintptr_t) &ptl3[PTL3_INDEX(page)], sizeof(pte_t), 0);

	/*
	 * Second, free all empty tables along the way from PTL3 down to PTL0.
	 */
	
	/* check PTL3 */
	for (i = 0; i < PTL3_ENTRIES; i++) {
		if (PTE_VALID(&ptl3[i])) {
			empty = false;
			break;
		}
	}
	if (empty) {
		/*
		 * PTL3 is empty.
		 * Release the frame and remove PTL3 pointer from preceding table.
		 */
		frame_free(KA2PA((uintptr_t) ptl3));
		if (PTL2_ENTRIES)
			memsetb((uintptr_t) &ptl2[PTL2_INDEX(page)], sizeof(pte_t), 0);
		else if (PTL1_ENTRIES)
			memsetb((uintptr_t) &ptl1[PTL1_INDEX(page)], sizeof(pte_t), 0);
		else
			memsetb((uintptr_t) &ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
	} else {
		/*
		 * PTL3 is not empty.
		 * Therefore, there must be a path from PTL0 to PTL3 and
		 * thus nothing to free in higher levels.
		 */
		return;
	}
	
	/* check PTL2, empty is still true */
	if (PTL2_ENTRIES) {
		for (i = 0; i < PTL2_ENTRIES; i++) {
			if (PTE_VALID(&ptl2[i])) {
				empty = false;
				break;
			}
		}
		if (empty) {
			/*
			 * PTL2 is empty.
			 * Release the frame and remove PTL2 pointer from preceding table.
			 */
			frame_free(KA2PA((uintptr_t) ptl2));
			if (PTL1_ENTRIES)
				memsetb((uintptr_t) &ptl1[PTL1_INDEX(page)], sizeof(pte_t), 0);
			else
				memsetb((uintptr_t) &ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
		}
		else {
			/*
			 * PTL2 is not empty.
			 * Therefore, there must be a path from PTL0 to PTL2 and
			 * thus nothing to free in higher levels.
			 */
			return;
		}
	}

	/* check PTL1, empty is still true */
	if (PTL1_ENTRIES) {
		for (i = 0; i < PTL1_ENTRIES; i++) {
			if (PTE_VALID(&ptl1[i])) {
				empty = false;
				break;
			}
		}
		if (empty) {
			/*
			 * PTL1 is empty.
			 * Release the frame and remove PTL1 pointer from preceding table.
			 */
			frame_free(KA2PA((uintptr_t) ptl1));
			memsetb((uintptr_t) &ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
		}
	}

}

/** Find mapping for virtual page in hierarchical page tables.
 *
 * Find mapping for virtual page.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to which page belongs.
 * @param page Virtual page.
 *
 * @return NULL if there is no such mapping; entry from PTL3 describing the mapping otherwise.
 */
pte_t *pt_mapping_find(as_t *as, uintptr_t page)
{
	pte_t *ptl0, *ptl1, *ptl2, *ptl3;

	ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	return &ptl3[PTL3_INDEX(page)];
}

/** @}
 */
