/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/tsb.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <barrier.h>
#include <assert.h>
#include <mm/as.h>
#include <typedefs.h>
#include <macros.h>

/** Invalidate portion of TSB.
 *
 * We assume that the address space is already locked. Note that respective
 * portions of both TSBs are invalidated at a time.
 *
 * @param as Address space.
 * @param page First page to invalidate in TSB.
 * @param pages Number of pages to invalidate. Value of (size_t) -1 means the
 * 	whole TSB.
 */
void tsb_invalidate(as_t *as, uintptr_t page, size_t pages)
{
	size_t i0;
	size_t i;
	size_t cnt;

	assert(as->arch.itsb);
	assert(as->arch.dtsb);

	i0 = (page >> MMU_PAGE_WIDTH) & ITSB_ENTRY_MASK;

	if (pages == (size_t) -1 || (pages * 2) > ITSB_ENTRY_COUNT)
		cnt = ITSB_ENTRY_COUNT;
	else
		cnt = pages * 2;

	for (i = 0; i < cnt; i++) {
		as->arch.itsb[(i0 + i) & ITSB_ENTRY_MASK].tag.invalid = true;
		as->arch.dtsb[(i0 + i) & DTSB_ENTRY_MASK].tag.invalid = true;
	}
}

/** Copy software PTE to ITSB.
 *
 * @param t 	Software PTE.
 * @param index	Zero if lower 8K-subpage, one if higher 8K subpage.
 */
void itsb_pte_copy(pte_t *t, size_t index)
{
	as_t *as;
	tsb_entry_t *tte;
	size_t entry;

	assert(index <= 1);

	as = t->as;
	entry = ((t->page >> MMU_PAGE_WIDTH) + index) & ITSB_ENTRY_MASK;
	tte = &as->arch.itsb[entry];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	/* Invalidate the entry (tag target has this set to 0) */
	tte->tag.invalid = true;

	write_barrier();

	tte->tag.context = as->asid;
	/* the shift is bigger than PAGE_WIDTH, do not bother with index  */
	tte->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tte->data.value = 0;
	tte->data.size = PAGESIZE_8K;
	tte->data.pfn = (t->frame >> MMU_FRAME_WIDTH) + index;
	tte->data.cp = t->c;	/* cp as cache in phys.-idxed, c as cacheable */
	tte->data.p = t->k;	/* p as privileged, k as kernel */
	tte->data.v = t->p;	/* v as valid, p as present */

	write_barrier();

	tte->tag.invalid = false;	/* mark the entry as valid */
}

/** Copy software PTE to DTSB.
 *
 * @param t	Software PTE.
 * @param index	Zero if lower 8K-subpage, one if higher 8K-subpage.
 * @param ro	If true, the mapping is copied read-only.
 */
void dtsb_pte_copy(pte_t *t, size_t index, bool ro)
{
	as_t *as;
	tsb_entry_t *tte;
	size_t entry;

	assert(index <= 1);

	as = t->as;
	entry = ((t->page >> MMU_PAGE_WIDTH) + index) & DTSB_ENTRY_MASK;
	tte = &as->arch.dtsb[entry];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	/* Invalidate the entry (tag target has this set to 0) */
	tte->tag.invalid = true;

	write_barrier();

	tte->tag.context = as->asid;
	/* the shift is bigger than PAGE_WIDTH, do not bother with index */
	tte->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tte->data.value = 0;
	tte->data.size = PAGESIZE_8K;
	tte->data.pfn = (t->frame >> MMU_FRAME_WIDTH) + index;
	tte->data.cp = t->c;
#ifdef CONFIG_VIRT_IDX_DCACHE
	tte->data.cv = t->c;
#endif /* CONFIG_VIRT_IDX_DCACHE */
	tte->data.p = t->k;		/* p as privileged */
	tte->data.w = ro ? false : t->w;
	tte->data.v = t->p;

	write_barrier();

	tte->tag.invalid = false;	/* mark the entry as valid */
}

/** @}
 */
