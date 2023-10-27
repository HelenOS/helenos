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

/** @addtogroup kernel_genarch_mm
 * @{
 */

/**
 * @file
 * @brief Virtual Address Translation for hierarchical 4-level page tables.
 */

#include <assert.h>
#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/km.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <barrier.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memw.h>
#include <align.h>
#include <macros.h>
#include <bitops.h>

static void pt_mapping_insert(as_t *, uintptr_t, uintptr_t, unsigned int);
static void pt_mapping_remove(as_t *, uintptr_t);
static bool pt_mapping_find(as_t *, uintptr_t, bool, pte_t *pte);
static void pt_mapping_update(as_t *, uintptr_t, bool, pte_t *pte);
static void pt_mapping_make_global(uintptr_t, size_t);

const page_mapping_operations_t pt_mapping_operations = {
	.mapping_insert = pt_mapping_insert,
	.mapping_remove = pt_mapping_remove,
	.mapping_find = pt_mapping_find,
	.mapping_update = pt_mapping_update,
	.mapping_make_global = pt_mapping_make_global
};

/** Map page to frame using hierarchical page tables.
 *
 * Map virtual address page to physical address frame
 * using flags.
 *
 * @param as    Address space to wich page belongs.
 * @param page  Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 *
 */
void pt_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame,
    unsigned int flags)
{
	pte_t *ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);

	assert(page_table_locked(as));

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL1_FRAMES, FRAME_LOWMEM, PTL1_SIZE - 1));
		memsetb(newpt, PTL1_SIZE, 0);
		SET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page), KA2PA(newpt));
		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page),
		    PAGE_NOT_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE |
		    PAGE_WRITE);
		/*
		 * Make sure that a concurrent hardware page table walk or
		 * pt_mapping_find() will see the new PTL1 only after it is
		 * fully initialized.
		 */
		write_barrier();
		SET_PTL1_PRESENT(ptl0, PTL0_INDEX(page));
	}

	pte_t *ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL2_FRAMES, FRAME_LOWMEM, PTL2_SIZE - 1));
		memsetb(newpt, PTL2_SIZE, 0);
		SET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page), KA2PA(newpt));
		SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page),
		    PAGE_NOT_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE |
		    PAGE_WRITE);
		/*
		 * Make the new PTL2 visible only after it is fully initialized.
		 */
		write_barrier();
		SET_PTL2_PRESENT(ptl1, PTL1_INDEX(page));
	}

	pte_t *ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT) {
		pte_t *newpt = (pte_t *)
		    PA2KA(frame_alloc(PTL3_FRAMES, FRAME_LOWMEM, PTL2_SIZE - 1));
		memsetb(newpt, PTL2_SIZE, 0);
		SET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page), KA2PA(newpt));
		SET_PTL3_FLAGS(ptl2, PTL2_INDEX(page),
		    PAGE_NOT_PRESENT | PAGE_USER | PAGE_EXEC | PAGE_CACHEABLE |
		    PAGE_WRITE);
		/*
		 * Make the new PTL3 visible only after it is fully initialized.
		 */
		write_barrier();
		SET_PTL3_PRESENT(ptl2, PTL2_INDEX(page));
	}

	pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(page), frame);
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), flags | PAGE_NOT_PRESENT);
	/*
	 * Make the new mapping visible only after it is fully initialized.
	 */
	write_barrier();
	SET_FRAME_PRESENT(ptl3, PTL3_INDEX(page));
}

/** Remove mapping of page from hierarchical page tables.
 *
 * Remove any mapping of page within address space as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * Empty page tables except PTL0 are freed.
 *
 * @param as   Address space to wich page belongs.
 * @param page Virtual address of the page to be demapped.
 *
 */
void pt_mapping_remove(as_t *as, uintptr_t page)
{
	assert(page_table_locked(as));

	/*
	 * First, remove the mapping, if it exists.
	 */

	pte_t *ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);
	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));
	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));
	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return;

	pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	/*
	 * Destroy the mapping.
	 * Setting to PAGE_NOT_PRESENT is not sufficient.
	 * But we need SET_FRAME for possible PT coherence maintenance.
	 * At least on ARM.
	 */
	//TODO: Fix this inconsistency
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), PAGE_NOT_PRESENT);
	memsetb(&ptl3[PTL3_INDEX(page)], sizeof(pte_t), 0);

	/*
	 * Second, free all empty tables along the way from PTL3 down to PTL0
	 * except those needed for sharing the kernel non-identity mappings.
	 */

	/* Check PTL3 */
	bool empty = true;

	unsigned int i;
	for (i = 0; i < PTL3_ENTRIES; i++) {
		if (PTE_VALID(&ptl3[i])) {
			empty = false;
			break;
		}
	}

	if (empty) {
		/*
		 * PTL3 is empty.
		 * Release the frame and remove PTL3 pointer from the parent
		 * table.
		 */
#if (PTL2_ENTRIES != 0)
		memsetb(&ptl2[PTL2_INDEX(page)], sizeof(pte_t), 0);
#elif (PTL1_ENTRIES != 0)
		memsetb(&ptl1[PTL1_INDEX(page)], sizeof(pte_t), 0);
#else
		if (km_is_non_identity(page))
			return;

		memsetb(&ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
#endif
		frame_free(KA2PA((uintptr_t) ptl3), PTL3_FRAMES);
	} else {
		/*
		 * PTL3 is not empty.
		 * Therefore, there must be a path from PTL0 to PTL3 and
		 * thus nothing to free in higher levels.
		 *
		 */
		return;
	}

	/* Check PTL2, empty is still true */
#if (PTL2_ENTRIES != 0)
	for (i = 0; i < PTL2_ENTRIES; i++) {
		if (PTE_VALID(&ptl2[i])) {
			empty = false;
			break;
		}
	}

	if (empty) {
		/*
		 * PTL2 is empty.
		 * Release the frame and remove PTL2 pointer from the parent
		 * table.
		 */
#if (PTL1_ENTRIES != 0)
		memsetb(&ptl1[PTL1_INDEX(page)], sizeof(pte_t), 0);
#else
		if (km_is_non_identity(page))
			return;

		memsetb(&ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
#endif
		frame_free(KA2PA((uintptr_t) ptl2), PTL2_FRAMES);
	} else {
		/*
		 * PTL2 is not empty.
		 * Therefore, there must be a path from PTL0 to PTL2 and
		 * thus nothing to free in higher levels.
		 *
		 */
		return;
	}
#endif /* PTL2_ENTRIES != 0 */

	/* check PTL1, empty is still true */
#if (PTL1_ENTRIES != 0)
	for (i = 0; i < PTL1_ENTRIES; i++) {
		if (PTE_VALID(&ptl1[i])) {
			empty = false;
			break;
		}
	}

	if (empty) {
		/*
		 * PTL1 is empty.
		 * Release the frame and remove PTL1 pointer from the parent
		 * table.
		 */
		if (km_is_non_identity(page))
			return;

		memsetb(&ptl0[PTL0_INDEX(page)], sizeof(pte_t), 0);
		frame_free(KA2PA((uintptr_t) ptl1), PTL1_FRAMES);
	}
#endif /* PTL1_ENTRIES != 0 */
}

static pte_t *pt_mapping_find_internal(as_t *as, uintptr_t page, bool nolock)
{
	assert(nolock || page_table_locked(as));

	pte_t *ptl0 = (pte_t *) PA2KA((uintptr_t) as->genarch.page_table);
	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	read_barrier();

	pte_t *ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));
	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

#if (PTL1_ENTRIES != 0)
	/*
	 * Always read ptl2 only after we are sure it is present.
	 */
	read_barrier();
#endif

	pte_t *ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));
	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

#if (PTL2_ENTRIES != 0)
	/*
	 * Always read ptl3 only after we are sure it is present.
	 */
	read_barrier();
#endif

	pte_t *ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	return &ptl3[PTL3_INDEX(page)];
}

/** Find mapping for virtual page in hierarchical page tables.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param[out] pte Structure that will receive a copy of the found PTE.
 *
 * @return True if the mapping was found, false otherwise.
 */
bool pt_mapping_find(as_t *as, uintptr_t page, bool nolock, pte_t *pte)
{
	pte_t *t = pt_mapping_find_internal(as, page, nolock);
	if (t)
		*pte = *t;
	return t != NULL;
}

/** Update mapping for virtual page in hierarchical page tables.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param[in] pte  New PTE.
 */
void pt_mapping_update(as_t *as, uintptr_t page, bool nolock, pte_t *pte)
{
	pte_t *t = pt_mapping_find_internal(as, page, nolock);
	if (!t)
		panic("Updating non-existent PTE");

	assert(PTE_VALID(t) == PTE_VALID(pte));
	assert(PTE_PRESENT(t) == PTE_PRESENT(pte));
	assert(PTE_GET_FRAME(t) == PTE_GET_FRAME(pte));
	assert(PTE_WRITABLE(t) == PTE_WRITABLE(pte));
	assert(PTE_EXECUTABLE(t) == PTE_EXECUTABLE(pte));

	*t = *pte;
}

/** Return the size of the region mapped by a single PTL0 entry.
 *
 * @return Size of the region mapped by a single PTL0 entry.
 */
static uintptr_t ptl0_step_get(void)
{
	size_t va_bits;

	va_bits = fnzb(PTL0_ENTRIES) + fnzb(PTL1_ENTRIES) + fnzb(PTL2_ENTRIES) +
	    fnzb(PTL3_ENTRIES) + PAGE_WIDTH;

	return 1UL << (va_bits - fnzb(PTL0_ENTRIES));
}

/** Make the mappings in the given range global accross all address spaces.
 *
 * All PTL0 entries in the given range will be mapped to a next level page
 * table. The next level page table will be allocated and cleared.
 *
 * pt_mapping_remove() will never deallocate these page tables even when there
 * are no PTEs in them.
 *
 * @param as   Address space.
 * @param base Base address corresponding to the first PTL0 entry that will be
 *             altered by this function.
 * @param size Size in bytes defining the range of PTL0 entries that will be
 *             altered by this function.
 *
 */
void pt_mapping_make_global(uintptr_t base, size_t size)
{
	assert(size > 0);

	uintptr_t ptl0 = PA2KA((uintptr_t) AS_KERNEL->genarch.page_table);
	uintptr_t ptl0_step = ptl0_step_get();
	size_t frames;

#if (PTL1_ENTRIES != 0)
	frames = PTL1_FRAMES;
#elif (PTL2_ENTRIES != 0)
	frames = PTL2_FRAMES;
#else
	frames = PTL3_FRAMES;
#endif

	for (uintptr_t addr = ALIGN_DOWN(base, ptl0_step);
	    addr - 1 < base + size - 1;
	    addr += ptl0_step) {
		if (GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(addr)) != 0) {
			assert(overlaps(addr, ptl0_step,
			    config.identity_base, config.identity_size));

			/*
			 * This PTL0 entry also maps the kernel identity region,
			 * so it is already global and initialized.
			 */
			continue;
		}

		uintptr_t l1 = PA2KA(frame_alloc(frames, FRAME_LOWMEM, 0));
		memsetb((void *) l1, FRAMES2SIZE(frames), 0);
		SET_PTL1_ADDRESS(ptl0, PTL0_INDEX(addr), KA2PA(l1));
		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(addr),
		    PAGE_PRESENT | PAGE_USER | PAGE_CACHEABLE |
		    PAGE_EXEC | PAGE_WRITE | PAGE_READ);
	}
}

/** @}
 */
