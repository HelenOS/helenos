/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup ppc64mm	
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <arch/asm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch.h>
#include <arch/types.h>
#include <arch/exception.h>
#include <align.h>
#include <config.h>
#include <print.h>
#include <symtab.h>

static phte_t *phte;


/** Try to find PTE for faulting address
 *
 * Try to find PTE for faulting address.
 * The as->lock must be held on entry to this function
 * if lock is true.
 *
 * @param as       Address space.
 * @param lock     Lock/unlock the address space.
 * @param badvaddr Faulting virtual address.
 * @param access   Access mode that caused the fault.
 * @param istate   Pointer to interrupted state.
 * @param pfrc     Pointer to variable where as_page_fault() return code will be stored.
 * @return         PTE on success, NULL otherwise.
 *
 */
static pte_t *find_mapping_and_check(as_t *as, bool lock, uintptr_t badvaddr, int access,
				     istate_t *istate, int *pfrc)
{
	/*
	 * Check if the mapping exists in page tables.
	 */	
	pte_t *pte = page_mapping_find(as, badvaddr);
	if ((pte) && (pte->p)) {
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
		page_table_unlock(as, lock);
		switch (rc = as_page_fault(badvaddr, access, istate)) {
			case AS_PF_OK:
				/*
				 * The higher-level page fault handler succeeded,
				 * The mapping ought to be in place.
				 */
				page_table_lock(as, lock);
				pte = page_mapping_find(as, badvaddr);
				ASSERT((pte) && (pte->p));
				*pfrc = 0;
				return pte;
			case AS_PF_DEFER:
				page_table_lock(as, lock);
				*pfrc = rc;
				return NULL;
			case AS_PF_FAULT:
				page_table_lock(as, lock);
				printf("Page fault.\n");
				*pfrc = rc;
				return NULL;
			default:
				panic("unexpected rc (%d)\n", rc);
		}	
	}
}


static void pht_refill_fail(uintptr_t badvaddr, istate_t *istate)
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


static void pht_insert(const uintptr_t vaddr, const pfn_t pfn)
{
	uint32_t page = (vaddr >> 12) & 0xffff;
	uint32_t api = (vaddr >> 22) & 0x3f;
	uint32_t vsid;
	
	asm volatile (
		"mfsrin %0, %1\n"
		: "=r" (vsid)
		: "r" (vaddr)
	);
	
	/* Primary hash (xor) */
	uint32_t h = 0;
	uint32_t hash = vsid ^ page;
	uint32_t base = (hash & 0x3ff) << 3;
	uint32_t i;
	bool found = false;
	
	/* Find unused or colliding
	   PTE in PTEG */
	for (i = 0; i < 8; i++) {
		if ((!phte[base + i].v) || ((phte[base + i].vsid == vsid) && (phte[base + i].api == api))) {
			found = true;
			break;
		}
	}
	
	if (!found) {
		/* Secondary hash (not) */
		uint32_t base2 = (~hash & 0x3ff) << 3;
		
		/* Find unused or colliding
		   PTE in PTEG */
		for (i = 0; i < 8; i++) {
			if ((!phte[base2 + i].v) || ((phte[base2 + i].vsid == vsid) && (phte[base2 + i].api == api))) {
				found = true;
				base = base2;
				h = 1;
				break;
			}
		}
		
		if (!found) {
			// TODO: A/C precedence groups
			i = page % 8;
		}
	}
	
	phte[base + i].v = 1;
	phte[base + i].vsid = vsid;
	phte[base + i].h = h;
	phte[base + i].api = api;
	phte[base + i].rpn = pfn;
	phte[base + i].r = 0;
	phte[base + i].c = 0;
	phte[base + i].pp = 2; // FIXME
}


/** Process Instruction/Data Storage Interrupt
 *
 * @param data   True if Data Storage Interrupt.
 * @param istate Interrupted register context.
 *
 */
void pht_refill(bool data, istate_t *istate)
{
	uintptr_t badvaddr;
	pte_t *pte;
	int pfrc;
	as_t *as;
	bool lock;
	
	if (AS == NULL) {
		as = AS_KERNEL;
		lock = false;
	} else {
		as = AS;
		lock = true;
	}
	
	if (data) {
		asm volatile (
			"mfdar %0\n"
			: "=r" (badvaddr)
		);
	} else
		badvaddr = istate->pc;
		
	page_table_lock(as, lock);
	
	pte = find_mapping_and_check(as, lock, badvaddr, PF_ACCESS_READ /* FIXME */, istate, &pfrc);
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
				page_table_unlock(as, lock);
				return;
			default:
				panic("Unexpected pfrc (%d)\n", pfrc);
		}
	}
	
	pte->a = 1; /* Record access to PTE */
	pht_insert(badvaddr, pte->pfn);
	
	page_table_unlock(as, lock);
	return;
	
fail:
	page_table_unlock(as, lock);
	pht_refill_fail(badvaddr, istate);
}


void pht_init(void)
{
	memsetb((uintptr_t) phte, 1 << PHT_BITS, 0);
}


void page_arch_init(void)
{
	if (config.cpu_active == 1) {
		page_mapping_operations = &pt_mapping_operations;
		
		uintptr_t cur;
		int flags;
		
		for (cur = 128 << 20; cur < last_frame; cur += FRAME_SIZE) {
			flags = PAGE_CACHEABLE | PAGE_WRITE;
			if ((PA2KA(cur) >= config.base) && (PA2KA(cur) < config.base + config.kernel_size))
				flags |= PAGE_GLOBAL;
			page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);
		}
		
		/* Allocate page hash table */
		phte_t *physical_phte = (phte_t *) frame_alloc(PHT_ORDER, FRAME_KA | FRAME_ATOMIC);
		
		ASSERT((uintptr_t) physical_phte % (1 << PHT_BITS) == 0);
		pht_init();
		
		asm volatile (
			"mtsdr1 %0\n"
			:
			: "r" ((uintptr_t) physical_phte)
		);
	}
}


uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	if (last_frame + ALIGN_UP(size, PAGE_SIZE) > KA2PA(KERNEL_ADDRESS_SPACE_END_ARCH))
		panic("Unable to map physical memory %p (%" PRIs " bytes)", physaddr, size)
	
	uintptr_t virtaddr = PA2KA(last_frame);
	pfn_t i;
	for (i = 0; i < ADDR2PFN(ALIGN_UP(size, PAGE_SIZE)); i++)
		page_mapping_insert(AS_KERNEL, virtaddr + PFN2ADDR(i), physaddr + PFN2ADDR(i), PAGE_NOT_CACHEABLE | PAGE_WRITE);
	
	last_frame = ALIGN_UP(last_frame + size, FRAME_SIZE);
	
	return virtaddr;
}

/** @}
 */
