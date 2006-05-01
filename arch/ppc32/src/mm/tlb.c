/*
 * Copyright (C) 2006 Martin Decky
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
#include <arch/types.h>
#include <mm/tlb.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch.h>
#include <print.h>
#include <symtab.h>


static phte_t *phte;


/** Initialize Page Hash Table.
 *
 * Setup the Page Hash Table with no entries.
 *
 */
void tlb_arch_init(void)
{
	phte_t *physical_phte = (phte_t *) PFN2ADDR(frame_alloc(PHT_ORDER, FRAME_KA | FRAME_PANIC));
	phte =(phte_t *) PA2KA((__address) physical_phte);
	
	ASSERT((__address) physical_phte % (1 << PHT_BITS) == 0);
	
	memsetb((__address) phte, 1 << PHT_BITS, 0);
	
	asm volatile (
		"mtsdr1 %0\n"
		:
		: "r" ((__address) physical_phte)
	);
}


/** Try to find PTE for faulting address
 *
 * Try to find PTE for faulting address.
 * The AS->lock must be held on entry to this function.
 *
 * @param badvaddr Faulting virtual address.
 * @return         PTE on success, NULL otherwise.
 *
 */
static pte_t *find_mapping_and_check(__address badvaddr)
{
	/*
	 * Check if the mapping exists in page tables.
	 */	
	pte_t *pte = page_mapping_find(AS, badvaddr);
	if ((pte) && (pte->p)) {
		/*
		 * Mapping found in page tables.
		 * Immediately succeed.
		 */
		return pte;
	} else {
		/*
		 * Mapping not found in page tables.
		 * Resort to higher-level page fault handler.
		 */
		page_table_unlock(AS, true);
		if (as_page_fault(badvaddr)) {
			/*
			 * The higher-level page fault handler succeeded,
			 * The mapping ought to be in place.
			 */
			page_table_lock(AS, true);
			pte = page_mapping_find(AS, badvaddr);
			ASSERT((pte) && (pte->p));
			return pte;
		} else {
			page_table_lock(AS, true);
			printf("Page fault.\n");
			return NULL;
		}
		
	}
}


static void pht_refill_fail(__address badvaddr, istate_t *istate)
{
	char *symbol = "";
	char *sym2 = "";

	char *s = get_symtab_entry(istate->pc);
	if (s)
		symbol = s;
	s = get_symtab_entry(istate->lr);
	if (s)
		sym2 = s;
	panic("%p: PHT Refill Exception at %p (%s<-%s)\n", badvaddr, istate->pc, symbol, sym2);
}


/** Process Instruction/Data Storage Interrupt
 *
 * @param data   True if Data Storage Interrupt.
 * @param istate Interrupted register context.
 *
 */
void pht_refill(bool data, istate_t *istate)
{
	asid_t asid;
	__address badvaddr;
	pte_t *pte;
	__u32 page;
	__u32 api;
	__u32 vsid;
	__u32 hash;
	__u32 i;
	
	if (data) {
		asm volatile (
			"mfdar %0\n"
			: "=r" (badvaddr)
		);
	} else
		badvaddr = istate->pc;
		
	spinlock_lock(&AS->lock);
	asid = AS->asid;
	spinlock_unlock(&AS->lock);
	
	page_table_lock(AS, true);
	
	pte = find_mapping_and_check(badvaddr);
	if (!pte)
		goto fail;

	/* Record access to PTE */
	pte->a = 1;
	
	page = (badvaddr >> 12) & 0xffff;
	api = (badvaddr >> 22) & 0x3f;
	asm volatile (
		"mfsrin %0, %1\n"
		: "=r" (vsid)
		: "r" (badvaddr)
	);
	
	/* Primary hash (xor) */
	hash = ((vsid ^ page) & 0x3ff) << 3;
	
	/* Find invalid PTE in PTEG */
	for (i = 0; i < 8; i++) {
		if (!phte[hash + i].v)
			break;
	}
	
	// TODO: Check access/change bits, secondary hash
	
	if (i == 8)
		i = page % 8;
	
	phte[hash + i].v = 1;
	phte[hash + i].vsid = vsid;
	phte[hash + i].h = 0;
	phte[hash + i].api = api;
	phte[hash + i].rpn = pte->pfn;
	phte[hash + i].r = 0;
	phte[hash + i].c = 0;
	phte[hash + i].pp = 2; // FIXME
	
	page_table_unlock(AS, true);
	return;
	
fail:
	page_table_unlock(AS, true);
	pht_refill_fail(badvaddr, istate);
}


/** Print contents of Page Hash Table. */
void tlb_print(void)
{
}
