/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips64mm
 * @{
 */
/** @file
 */

#include <arch/mm/tlb.h>
#include <mm/asid.h>
#include <mm/tlb.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch/cp0.h>
#include <panic.h>
#include <arch.h>
#include <synch/mutex.h>
#include <print.h>
#include <debug.h>
#include <align.h>
#include <interrupt.h>
#include <symtab.h>

/** Initialize TLB.
 *
 * Invalidate all entries and mark wired entries.
 *
 */
void tlb_arch_init(void)
{
	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	cp0_entry_hi_write(0);
	cp0_entry_lo0_write(0);
	cp0_entry_lo1_write(0);
	
	/* Clear and initialize TLB. */
	
	for (unsigned int i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbwi();
	}
	
	/*
	 * The kernel is going to make use of some wired
	 * entries (e.g. mapping kernel stacks in kseg3).
	 */
	cp0_wired_write(TLB_WIRED);
}

/** Try to find PTE for faulting address.
 *
 * @param badvaddr Faulting virtual address.
 * @param access   Access mode that caused the fault.
 * @param istate   Pointer to interrupted state.
 * @param pfrc     Pointer to variable where as_page_fault()
 *                 return code will be stored.
 *
 * @return PTE on success, NULL otherwise.
 *
 */
static pte_t *find_mapping_and_check(uintptr_t badvaddr, int access,
    istate_t *istate, int *pfrc)
{
	entry_hi_t hi;
	hi.value = cp0_entry_hi_read();
	
	/*
	 * Handler cannot succeed if the ASIDs don't match.
	 */
	if (hi.asid != AS->asid) {
		printf("EntryHi.asid=%d, AS->asid=%d\n", hi.asid, AS->asid);
		return NULL;
	}
	
	/*
	 * Check if the mapping exists in page tables.
	 */
	pte_t *pte = page_mapping_find(AS, badvaddr, true);
	if ((pte) && (pte->p) && ((pte->w) || (access != PF_ACCESS_WRITE))) {
		/*
		 * Mapping found in page tables.
		 * Immediately succeed.
		 */
		return pte;
	} else {
		int rc;
		
		/*
		 * Mapping not found in page tables.
		 * Resort to higher-level page fault handler.
		 */
		switch (rc = as_page_fault(badvaddr, access, istate)) {
		case AS_PF_OK:
			/*
			 * The higher-level page fault handler succeeded,
			 * The mapping ought to be in place.
			 */
			pte = page_mapping_find(AS, badvaddr, true);
			ASSERT(pte);
			ASSERT(pte->p);
			ASSERT((pte->w) || (access != PF_ACCESS_WRITE));
			return pte;
		case AS_PF_DEFER:
			*pfrc = AS_PF_DEFER;
			return NULL;
		case AS_PF_FAULT:
			*pfrc = AS_PF_FAULT;
			return NULL;
		default:
			panic("Unexpected return code (%d).", rc);
		}
	}
}

void tlb_prepare_entry_lo(entry_lo_t *lo, bool g, bool v, bool d,
    bool c, uintptr_t addr)
{
	lo->value = 0;
	lo->g = g;
	lo->v = v;
	lo->d = d;
	lo->c = c ? PAGE_CACHEABLE_EXC_WRITE : PAGE_UNCACHED;
	lo->pfn = ADDR2PFN(addr);
}

void tlb_prepare_entry_hi(entry_hi_t *hi, asid_t asid, uintptr_t addr)
{
	hi->value = ALIGN_DOWN(addr, PAGE_SIZE * 2);
	hi->asid = asid;
}

static void tlb_refill_fail(istate_t *istate)
{
	uintptr_t va = cp0_badvaddr_read();
	
	fault_if_from_uspace(istate, "TLB Refill Exception on %p.",
	    (void *) va);
	panic_memtrap(istate, PF_ACCESS_UNKNOWN, va, "TLB Refill Exception.");
}

static void tlb_invalid_fail(istate_t *istate)
{
	uintptr_t va = cp0_badvaddr_read();
	
	fault_if_from_uspace(istate, "TLB Invalid Exception on %p.",
	    (void *) va);
	panic_memtrap(istate, PF_ACCESS_UNKNOWN, va, "TLB Invalid Exception.");
}

static void tlb_modified_fail(istate_t *istate)
{
	uintptr_t va = cp0_badvaddr_read();
	
	fault_if_from_uspace(istate, "TLB Modified Exception on %p.",
	    (void *) va);
	panic_memtrap(istate, PF_ACCESS_WRITE, va, "TLB Modified Exception.");
}

/** Process TLB Refill Exception.
 *
 * @param istate Interrupted register context.
 *
 */
void tlb_refill(istate_t *istate)
{
	uintptr_t badvaddr = cp0_badvaddr_read();
	
	mutex_lock(&AS->lock);
	asid_t asid = AS->asid;
	mutex_unlock(&AS->lock);
	
	int pfrc;
	pte_t *pte = find_mapping_and_check(badvaddr, PF_ACCESS_READ,
	    istate, &pfrc);
	if (!pte) {
		switch (pfrc) {
		case AS_PF_FAULT:
			goto fail;
			break;
		case AS_PF_DEFER:
			/*
			 * The page fault came during copy_from_uspace()
			 * or copy_to_uspace().
			 */
			return;
		default:
			panic("Unexpected pfrc (%d).", pfrc);
		}
	}
	
	/*
	 * Record access to PTE.
	 */
	pte->a = 1;
	
	entry_lo_t lo;
	entry_hi_t hi;
	
	tlb_prepare_entry_hi(&hi, asid, badvaddr);
	tlb_prepare_entry_lo(&lo, pte->g, pte->p, pte->d, pte->c,
	    pte->frame);
	
	/*
	 * New entry is to be inserted into TLB
	 */
	cp0_entry_hi_write(hi.value);
	
	if ((badvaddr / PAGE_SIZE) % 2 == 0) {
		cp0_entry_lo0_write(lo.value);
		cp0_entry_lo1_write(0);
	} else {
		cp0_entry_lo0_write(0);
		cp0_entry_lo1_write(lo.value);
	}
	
	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	tlbwr();
	
	return;
	
fail:
	tlb_refill_fail(istate);
}

/** Process TLB Invalid Exception.
 *
 * @param istate Interrupted register context.
 *
 */
void tlb_invalid(istate_t *istate)
{
	uintptr_t badvaddr = cp0_badvaddr_read();
	
	/*
	 * Locate the faulting entry in TLB.
	 */
	entry_hi_t hi;
	hi.value = cp0_entry_hi_read();
	
	tlb_prepare_entry_hi(&hi, hi.asid, badvaddr);
	cp0_entry_hi_write(hi.value);
	tlbp();
	
	tlb_index_t index;
	index.value = cp0_index_read();
	
	/*
	 * Fail if the entry is not in TLB.
	 */
	if (index.p) {
		printf("TLB entry not found.\n");
		goto fail;
	}
	
	int pfrc;
	pte_t *pte = find_mapping_and_check(badvaddr, PF_ACCESS_READ,
	    istate, &pfrc);
	if (!pte) {
		switch (pfrc) {
		case AS_PF_FAULT:
			goto fail;
			break;
		case AS_PF_DEFER:
			/*
			 * The page fault came during copy_from_uspace()
			 * or copy_to_uspace().
			 */
			return;
		default:
			panic("Unexpected pfrc (%d).", pfrc);
		}
	}
	
	/*
	 * Read the faulting TLB entry.
	 */
	tlbr();
	
	/*
	 * Record access to PTE.
	 */
	pte->a = 1;
	
	entry_lo_t lo;
	tlb_prepare_entry_lo(&lo, pte->g, pte->p, pte->d, pte->c,
	    pte->frame);
	
	/*
	 * The entry is to be updated in TLB.
	 */
	if ((badvaddr / PAGE_SIZE) % 2 == 0)
		cp0_entry_lo0_write(lo.value);
	else
		cp0_entry_lo1_write(lo.value);
	
	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	tlbwi();
	
	return;
	
fail:
	tlb_invalid_fail(istate);
}

/** Process TLB Modified Exception.
 *
 * @param istate Interrupted register context.
 *
 */
void tlb_modified(istate_t *istate)
{
	uintptr_t badvaddr = cp0_badvaddr_read();
	
	/*
	 * Locate the faulting entry in TLB.
	 */
	entry_hi_t hi;
	hi.value = cp0_entry_hi_read();
	
	tlb_prepare_entry_hi(&hi, hi.asid, badvaddr);
	cp0_entry_hi_write(hi.value);
	tlbp();
	
	tlb_index_t index;
	index.value = cp0_index_read();
	
	/*
	 * Fail if the entry is not in TLB.
	 */
	if (index.p) {
		printf("TLB entry not found.\n");
		goto fail;
	}
	
	int pfrc;
	pte_t *pte = find_mapping_and_check(badvaddr, PF_ACCESS_WRITE,
	    istate, &pfrc);
	if (!pte) {
		switch (pfrc) {
		case AS_PF_FAULT:
			goto fail;
			break;
		case AS_PF_DEFER:
			/*
			 * The page fault came during copy_from_uspace()
			 * or copy_to_uspace().
			 */
			return;
		default:
			panic("Unexpected pfrc (%d).", pfrc);
		}
	}
	
	/*
	 * Read the faulting TLB entry.
	 */
	tlbr();
	
	/*
	 * Record access and write to PTE.
	 */
	pte->a = 1;
	pte->d = 1;
	
	entry_lo_t lo;
	tlb_prepare_entry_lo(&lo, pte->g, pte->p, pte->w, pte->c,
	    pte->frame);
	
	/*
	 * The entry is to be updated in TLB.
	 */
	if ((badvaddr / PAGE_SIZE) % 2 == 0)
		cp0_entry_lo0_write(lo.value);
	else
		cp0_entry_lo1_write(lo.value);
	
	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	tlbwi();
	
	return;
	
fail:
	tlb_modified_fail(istate);
}

/** Print contents of TLB. */
void tlb_print(void)
{
	entry_hi_t hi_save;
	hi_save.value = cp0_entry_hi_read();
	
	printf("[nr] [asid] [vpn2] [mask] [gvdc] [pfn ]\n");
	
	for (unsigned int i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();
		
		page_mask_t mask;
		mask.value = cp0_pagemask_read();
		
		entry_hi_t hi;
		hi.value = cp0_entry_hi_read();
		
		entry_lo_t lo0;
		lo0.value = cp0_entry_lo0_read();
		
		entry_lo_t lo1;
		lo1.value = cp0_entry_lo1_read();
		
		printf("%-4u %-6u %#6x %#6x  %1u%1u%1u%1u  %#6x\n",
		    i, hi.asid, hi.vpn2, mask.mask,
		    lo0.g, lo0.v, lo0.d, lo0.c, lo0.pfn);
		printf("                           %1u%1u%1u%1u  %#6x\n",
		    lo1.g, lo1.v, lo1.d, lo1.c, lo1.pfn);
	}
	
	cp0_entry_hi_write(hi_save.value);
}

/** Invalidate all not wired TLB entries. */
void tlb_invalidate_all(void)
{
	entry_hi_t hi_save;
	hi_save.value = cp0_entry_hi_read();
	ipl_t ipl = interrupts_disable();
	
	for (unsigned int i = TLB_WIRED; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();
		
		entry_lo_t lo0;
		lo0.value = cp0_entry_lo0_read();
		
		entry_lo_t lo1;
		lo1.value = cp0_entry_lo1_read();
		
		lo0.v = 0;
		lo1.v = 0;
		
		cp0_entry_lo0_write(lo0.value);
		cp0_entry_lo1_write(lo1.value);
		
		tlbwi();
	}
	
	interrupts_restore(ipl);
	cp0_entry_hi_write(hi_save.value);
}

/** Invalidate all TLB entries belonging to specified address space.
 *
 * @param asid Address space identifier.
 *
 */
void tlb_invalidate_asid(asid_t asid)
{
	ASSERT(asid != ASID_INVALID);
	
	entry_hi_t hi_save;
	hi_save.value = cp0_entry_hi_read();
	ipl_t ipl = interrupts_disable();
	
	for (unsigned int i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();
		
		entry_hi_t hi;
		hi.value = cp0_entry_hi_read();
		
		if (hi.asid == asid) {
			entry_lo_t lo0;
			lo0.value = cp0_entry_lo0_read();
			
			entry_lo_t lo1;
			lo1.value = cp0_entry_lo1_read();
			
			lo0.v = 0;
			lo1.v = 0;
			
			cp0_entry_lo0_write(lo0.value);
			cp0_entry_lo1_write(lo1.value);
			
			tlbwi();
		}
	}
	
	interrupts_restore(ipl);
	cp0_entry_hi_write(hi_save.value);
}

/** Invalidate TLB entries for specified page range belonging to specified
 * address space.
 *
 * @param asid Address space identifier.
 * @param page First page whose TLB entry is to be invalidated.
 * @param cnt  Number of entries to invalidate.
 *
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	if (asid == ASID_INVALID)
		return;
	
	entry_hi_t hi_save;
	hi_save.value = cp0_entry_hi_read();
	ipl_t ipl = interrupts_disable();
	
	for (unsigned int i = 0; i < cnt + 1; i += 2) {
		entry_hi_t hi;
		hi.value = 0;
		tlb_prepare_entry_hi(&hi, asid, page + i * PAGE_SIZE);
		cp0_entry_hi_write(hi.value);
		
		tlbp();
		
		tlb_index_t index;
		index.value = cp0_index_read();
		
		if (!index.p) {
			/*
			 * Entry was found, index register contains valid
			 * index.
			 */
			tlbr();
			
			entry_lo_t lo0;
			lo0.value = cp0_entry_lo0_read();
			
			entry_lo_t lo1;
			lo1.value = cp0_entry_lo1_read();
			
			lo0.v = 0;
			lo1.v = 0;
			
			cp0_entry_lo0_write(lo0.value);
			cp0_entry_lo1_write(lo1.value);
			
			tlbwi();
		}
	}
	
	interrupts_restore(ipl);
	cp0_entry_hi_write(hi_save.value);
}

/** @}
 */
