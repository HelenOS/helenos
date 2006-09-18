/*
 * Copyright (C) 2006 Jakub Jermar
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
#include <arch/mm/tlb.h>
#include <arch/barrier.h>
#include <mm/as.h>
#include <arch/types.h>
#include <typedefs.h>
#include <macros.h>
#include <debug.h>

#define TSB_INDEX_MASK		((1<<(21+1+TSB_SIZE-PAGE_WIDTH))-1)

/** Invalidate portion of TSB.
 *
 * We assume that the address space is already locked.
 * Note that respective portions of both TSBs
 * are invalidated at a time.
 *
 * @param as Address space.
 * @param page First page to invalidate in TSB.
 * @param pages Number of pages to invalidate. Value of (count_t) -1 means the whole TSB. 
 */
void tsb_invalidate(as_t *as, uintptr_t page, count_t pages)
{
	index_t i0, i;
	count_t cnt;
	
	ASSERT(as->arch.itsb && as->arch.dtsb);
	
	i0 = (page >> PAGE_WIDTH) & TSB_INDEX_MASK;
	cnt = min(pages, ITSB_ENTRY_COUNT);
	
	for (i = 0; i < cnt; i++) {
		as->arch.itsb[(i0 + i) & (ITSB_ENTRY_COUNT-1)].tag.invalid = 0;
		as->arch.dtsb[(i0 + i) & (DTSB_ENTRY_COUNT-1)].tag.invalid = 0;
	}
}

/** Copy software PTE to ITSB.
 *
 * @param t Software PTE.
 */
void itsb_pte_copy(pte_t *t)
{
	as_t *as;
	tsb_entry_t *tsb;
	
	as = t->as;
	tsb = &as->arch.itsb[(t->page >> PAGE_WIDTH) & TSB_INDEX_MASK];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tsb->tag.invalid = 1;	/* invalidate the entry (tag target has this set to 0 */

	write_barrier();

	tsb->tag.context = as->asid;
	tsb->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tsb->data.value = 0;
	tsb->data.size = PAGESIZE_8K;
	tsb->data.pfn = t->frame >> PAGE_WIDTH;
	tsb->data.cp = t->c;
	tsb->data.cv = t->c;
	tsb->data.p = t->k;	/* p as privileged */
	tsb->data.v = t->p;
	
	write_barrier();
	
	tsb->tag.invalid = 0;	/* mark the entry as valid */
}

/** Copy software PTE to DTSB.
 *
 * @param t Software PTE.
 * @param ro If true, the mapping is copied read-only.
 */
void dtsb_pte_copy(pte_t *t, bool ro)
{
	as_t *as;
	tsb_entry_t *tsb;
	
	as = t->as;
	tsb = &as->arch.dtsb[(t->page >> PAGE_WIDTH) & TSB_INDEX_MASK];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tsb->tag.invalid = 1;	/* invalidate the entry (tag target has this set to 0) */

	write_barrier();

	tsb->tag.context = as->asid;
	tsb->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tsb->data.value = 0;
	tsb->data.size = PAGESIZE_8K;
	tsb->data.pfn = t->frame >> PAGE_WIDTH;
	tsb->data.cp = t->c;
	tsb->data.cv = t->c;
	tsb->data.p = t->k;	/* p as privileged */
	tsb->data.w = ro ? false : t->w;
	tsb->data.v = t->p;
	
	write_barrier();
	
	tsb->tag.invalid = 0;	/* mark the entry as valid */
}

/** @}
 */
