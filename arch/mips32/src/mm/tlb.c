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
	struct entry_hi hi;
	__address badvaddr;
	pte_t *pte;
	
	*((__u32 *) &hi) = cp0_entry_hi_read();
	badvaddr = cp0_badvaddr_read();
	
	spinlock_lock(&VM->lock);

	/*
	 * Refill cannot succeed if the ASIDs don't match.
	 */
	if (hi.asid != VM->asid)
		goto fail;

	/*
	 * Refill cannot succeed if badvaddr is not
	 * associated with any mapping.
	 */
	pte = find_mapping(badvaddr, 0);
	if (!pte)
		goto fail;
		
	/*
	 * Refill cannot succeed if the mapping is marked as invalid.
	 */
	if (!pte->v)
		goto fail;

	/*
	 * New entry is to be inserted into TLB
	 */
	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	if ((badvaddr/PAGE_SIZE) % 2 == 0) {
		cp0_entry_lo0_write(*((__u32 *) pte));
		cp0_entry_lo1_write(0);
	}
	else {
		cp0_entry_lo0_write(0);
		cp0_entry_lo1_write(*((__u32 *) pte));
	}
	tlbwr();

	spinlock_unlock(&VM->lock);
	return;
	
fail:
	spinlock_unlock(&VM->lock);
	tlb_refill_fail(pstate);
}

void tlb_invalid(struct exception_regdump *pstate)
{
	tlb_invalid_fail(pstate);
}

void tlb_modified(struct exception_regdump *pstate)
{
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
	panic("%X: TLB Invalid Exception at %X(%s)\n", cp0_badvaddr_read(),
	      pstate->epc, symbol);
}

void tlb_modified_fail(struct exception_regdump *pstate)
{
	char *symbol = "";

	char *s = get_symtab_entry(pstate->epc);
	if (s)
		symbol = s;
	panic("%X: TLB Modified Exception at %X(%s)\n", cp0_badvaddr_read(),
	      pstate->epc, symbol);
}


void tlb_invalidate(int asid)
{
	pri_t pri;
	
	pri = cpu_priority_high();
	
	// TODO
	
	cpu_priority_restore(pri);
}
