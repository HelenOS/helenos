/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#include <arch/mm/tlb.h>
#include <arch/mm/asid.h>
#include <mm/tlb.h>
#include <mm/page.h>
#include <mm/vm.h>
#include <arch/cp0.h>
#include <panic.h>
#include <arch.h>
#include <symtab.h>
#include <synch/spinlock.h>
#include <print.h>

static void tlb_refill_fail(struct exception_regdump *pstate);
static void tlb_invalid_fail(struct exception_regdump *pstate);
static void tlb_modified_fail(struct exception_regdump *pstate);

static pte_t *find_mapping_and_check(__address badvaddr);
static void prepare_entry_lo(struct entry_lo *lo, bool g, bool v, bool d, int c, __address pfn);

/** Initialize TLB
 *
 * Initialize TLB.
 * Invalidate all entries and mark wired entries.
 */
void tlb_init_arch(void)
{
	int i;

	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	cp0_entry_hi_write(0);
	cp0_entry_lo0_write(0);
	cp0_entry_lo1_write(0);

	/*
	 * Invalidate all entries.
	 */
	for (i = 0; i < TLB_SIZE; i++) {
		cp0_index_write(i);
		tlbwi();
	}
	
	/*
	 * The kernel is going to make use of some wired
	 * entries (e.g. mapping kernel stacks in kseg3).
	 */
	cp0_wired_write(TLB_WIRED);
}

/** Process TLB Refill Exception
 *
 * Process TLB Refill Exception.
 *
 * @param pstate Interrupted register context.
 */
void tlb_refill(struct exception_regdump *pstate)
{
	struct entry_lo lo;
	__address badvaddr;
	pte_t *pte;
	
	badvaddr = cp0_badvaddr_read();
	
	spinlock_lock(&VM->lock);		
	pte = find_mapping_and_check(badvaddr);
	if (!pte)
		goto fail;

	/*
	 * Record access to PTE.
	 */
	pte->a = 1;

	prepare_entry_lo(&lo, pte->g, pte->v, pte->d, pte->c, pte->pfn);

	/*
	 * New entry is to be inserted into TLB
	 */
	if ((badvaddr/PAGE_SIZE) % 2 == 0) {
		cp0_entry_lo0_write(*((__u32 *) &lo));
		cp0_entry_lo1_write(0);
	}
	else {
		cp0_entry_lo0_write(0);
		cp0_entry_lo1_write(*((__u32 *) &lo));
	}
	tlbwr();

	spinlock_unlock(&VM->lock);
	return;
	
fail:
	spinlock_unlock(&VM->lock);
	tlb_refill_fail(pstate);
}

/** Process TLB Invalid Exception
 *
 * Process TLB Invalid Exception.
 *
 * @param pstate Interrupted register context.
 */
void tlb_invalid(struct exception_regdump *pstate)
{
	struct index index;
	__address badvaddr;
	struct entry_lo lo;
	pte_t *pte;

	badvaddr = cp0_badvaddr_read();

	/*
	 * Locate the faulting entry in TLB.
	 */
	tlbp();
	*((__u32 *) &index) = cp0_index_read();
	
	spinlock_lock(&VM->lock);	
	
	/*
	 * Fail if the entry is not in TLB.
	 */
	if (index.p)
		goto fail;

	pte = find_mapping_and_check(badvaddr);
	if (!pte)
		goto fail;

	/*
	 * Read the faulting TLB entry.
	 */
	tlbr();

	/*
	 * Record access to PTE.
	 */
	pte->a = 1;

	prepare_entry_lo(&lo, pte->g, pte->v, pte->d, pte->c, pte->pfn);

	/*
	 * The entry is to be updated in TLB.
	 */
	if ((badvaddr/PAGE_SIZE) % 2 == 0)
		cp0_entry_lo0_write(*((__u32 *) &lo));
	else
		cp0_entry_lo1_write(*((__u32 *) &lo));
	tlbwi();

	spinlock_unlock(&VM->lock);	
	return;
	
fail:
	spinlock_unlock(&VM->lock);
	tlb_invalid_fail(pstate);
}

/** Process TLB Modified Exception
 *
 * Process TLB Modified Exception.
 *
 * @param pstate Interrupted register context.
 */

void tlb_modified(struct exception_regdump *pstate)
{
	struct index index;
	__address badvaddr;
	struct entry_lo lo;
	pte_t *pte;

	badvaddr = cp0_badvaddr_read();

	/*
	 * Locate the faulting entry in TLB.
	 */
	tlbp();
	*((__u32 *) &index) = cp0_index_read();
	
	spinlock_lock(&VM->lock);	
	
	/*
	 * Fail if the entry is not in TLB.
	 */
	if (index.p)
		goto fail;

	pte = find_mapping_and_check(badvaddr);
	if (!pte)
		goto fail;

	/*
	 * Fail if the page is not writable.
	 */
	if (!pte->w)
		goto fail;

	/*
	 * Read the faulting TLB entry.
	 */
	tlbr();

	/*
	 * Record access and write to PTE.
	 */
	pte->a = 1;
	pte->d = 1;

	prepare_entry_lo(&lo, pte->g, pte->v, pte->w, pte->c, pte->pfn);

	/*
	 * The entry is to be updated in TLB.
	 */
	if ((badvaddr/PAGE_SIZE) % 2 == 0)
		cp0_entry_lo0_write(*((__u32 *) &lo));
	else
		cp0_entry_lo1_write(*((__u32 *) &lo));
	tlbwi();

	spinlock_unlock(&VM->lock);	
	return;
	
fail:
	spinlock_unlock(&VM->lock);
	tlb_modified_fail(pstate);
}

void tlb_refill_fail(struct exception_regdump *pstate)
{
	char *symbol = "";
	char *sym2 = "";

	char *s = get_symtab_entry(pstate->epc);
	if (s)
		symbol = s;
	s = get_symtab_entry(pstate->ra);
	if (s)
		sym2 = s;
	panic("%X: TLB Refill Exception at %X(%s<-%s)\n", cp0_badvaddr_read(), pstate->epc, symbol, sym2);
}


void tlb_invalid_fail(struct exception_regdump *pstate)
{
	char *symbol = "";

	char *s = get_symtab_entry(pstate->epc);
	if (s)
		symbol = s;
	panic("%X: TLB Invalid Exception at %X(%s)\n", cp0_badvaddr_read(), pstate->epc, symbol);
}

void tlb_modified_fail(struct exception_regdump *pstate)
{
	char *symbol = "";

	char *s = get_symtab_entry(pstate->epc);
	if (s)
		symbol = s;
	panic("%X: TLB Modified Exception at %X(%s)\n", cp0_badvaddr_read(), pstate->epc, symbol);
}


void tlb_invalidate(int asid)
{
	pri_t pri;
	
	pri = cpu_priority_high();
	
	// TODO
	
	cpu_priority_restore(pri);
}

/** Try to find PTE for faulting address
 *
 * Try to find PTE for faulting address.
 * The VM->lock must be held on entry to this function.
 *
 * @param badvaddr Faulting virtual address.
 *
 * @return PTE on success, NULL otherwise.
 */
pte_t *find_mapping_and_check(__address badvaddr)
{
	struct entry_hi hi;
	pte_t *pte;

	*((__u32 *) &hi) = cp0_entry_hi_read();

	/*
	 * Handler cannot succeed if the ASIDs don't match.
	 */
	if (hi.asid != VM->asid)
		return NULL;
	
	/*
	 * Handler cannot succeed if badvaddr has no mapping.
	 */
	pte = find_mapping(badvaddr, 0);
	if (!pte)
		return NULL;

	/*
	 * Handler cannot succeed if the mapping is marked as invalid.
	 */
	if (!pte->v)
		return NULL;

	return pte;
}

void prepare_entry_lo(struct entry_lo *lo, bool g, bool v, bool d, int c, __address pfn)
{
	lo->g = g;
	lo->v = v;
	lo->d = d;
	lo->c = c;
	lo->pfn = pfn;
	lo->zero = 0;
}
