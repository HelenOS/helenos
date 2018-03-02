/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <arch/mm/tsb.h>
#include <arch/mm/pagesize.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <arch/barrier.h>
#include <assert.h>
#include <mm/as.h>
#include <typedefs.h>
#include <macros.h>

/** Invalidate portion of TSB.
 *
 * We assume that the address space is already locked. Note that respective
 * portions of both TSBs are invalidated at a time.
 *
 * @param as	Address space.
 * @param page 	First page to invalidate in TSB.
 * @param pages Number of pages to invalidate. Value of (count_t) -1 means the
 * 		whole TSB.
 */
void tsb_invalidate(as_t *as, uintptr_t page, size_t pages)
{
	tsb_entry_t *tsb;
	size_t i0, i;
	size_t cnt;

	assert(as->arch.tsb_description.tsb_base);

	i0 = (page >> MMU_PAGE_WIDTH) & TSB_ENTRY_MASK;

	if (pages == (size_t) -1 || pages > TSB_ENTRY_COUNT)
		cnt = TSB_ENTRY_COUNT;
	else
		cnt = pages;

	tsb = (tsb_entry_t *) PA2KA(as->arch.tsb_description.tsb_base);
	for (i = 0; i < cnt; i++)
		tsb[(i0 + i) & TSB_ENTRY_MASK].data.v = false;
}

/** Copy software PTE to ITSB.
 *
 * @param t 	Software PTE.
 */
void itsb_pte_copy(pte_t *t)
{
	as_t *as;
	tsb_entry_t *tsb;
	tsb_entry_t *tte;
	size_t index;

	as = t->as;
	index = (t->page >> MMU_PAGE_WIDTH) & TSB_ENTRY_MASK;

	tsb = (tsb_entry_t *) PA2KA(as->arch.tsb_description.tsb_base);
	tte = &tsb[index];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tte->data.v = false;

	write_barrier();

	tte->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;

	tte->data.value = 0;
	tte->data.nfo = false;
	tte->data.ra = t->frame >> MMU_FRAME_WIDTH;
	tte->data.ie = false;
	tte->data.e = false;
	tte->data.cp = t->c;	/* cp as cache in phys.-idxed, c as cacheable */
	tte->data.cv = false;
	tte->data.p = t->k;	/* p as privileged, k as kernel */
	tte->data.x = true;
	tte->data.w = false;
	tte->data.size = PAGESIZE_8K;

	write_barrier();

	tte->data.v = t->p;	/* v as valid, p as present */
}

/** Copy software PTE to DTSB.
 *
 * @param t	Software PTE.
 * @param ro	If true, the mapping is copied read-only.
 */
void dtsb_pte_copy(pte_t *t, bool ro)
{
	as_t *as;
	tsb_entry_t *tsb;
	tsb_entry_t *tte;
	size_t index;

	as = t->as;
	index = (t->page >> MMU_PAGE_WIDTH) & TSB_ENTRY_MASK;
	tsb = (tsb_entry_t *) PA2KA(as->arch.tsb_description.tsb_base);
	tte = &tsb[index];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tte->data.v = false;

	write_barrier();

	tte->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;

	tte->data.value = 0;
	tte->data.nfo = false;
	tte->data.ra = t->frame >> MMU_FRAME_WIDTH;
	tte->data.ie = false;
	tte->data.e = false;
	tte->data.cp = t->c;	/* cp as cache in phys.-idxed, c as cacheable */
#ifdef CONFIG_VIRT_IDX_DCACHE
	tte->data.cv = t->c;
#endif /* CONFIG_VIRT_IDX_DCACHE */
	tte->data.p = t->k;	/* p as privileged, k as kernel */
	tte->data.x = true;
	tte->data.w = ro ? false : t->w;
	tte->data.size = PAGESIZE_8K;

	write_barrier();

	tte->data.v = t->p;	/* v as valid, p as present */
}

/** @}
 */
