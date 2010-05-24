/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ppc32mm
 * @{
 */
/** @file
 */

#include <mm/tlb.h>
#include <arch/mm/tlb.h>
#include <arch/interrupt.h>
#include <interrupt.h>
#include <mm/as.h>
#include <mm/page.h>
#include <arch.h>
#include <print.h>
#include <macros.h>
#include <symtab.h>

static unsigned int seed = 10;
static unsigned int seed_real
    __attribute__ ((section("K_UNMAPPED_DATA_START"))) = 42;

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
 * @param pfrc     Pointer to variable where as_page_fault() return code
 *                 will be stored.
 *
 * @return PTE on success, NULL otherwise.
 *
 */
static pte_t *find_mapping_and_check(as_t *as, bool lock, uintptr_t badvaddr,
    int access, istate_t *istate, int *pfrc)
{
	/*
	 * Check if the mapping exists in page tables.
	 */
	pte_t *pte = page_mapping_find(as, badvaddr);
	if ((pte) && (pte->present)) {
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
		page_table_unlock(as, lock);
		
		int rc = as_page_fault(badvaddr, access, istate);
		switch (rc) {
		case AS_PF_OK:
			/*
			 * The higher-level page fault handler succeeded,
			 * The mapping ought to be in place.
			 */
			page_table_lock(as, lock);
			pte = page_mapping_find(as, badvaddr);
			ASSERT((pte) && (pte->present));
			*pfrc = 0;
			return pte;
		case AS_PF_DEFER:
			page_table_lock(as, lock);
			*pfrc = rc;
			return NULL;
		case AS_PF_FAULT:
			page_table_lock(as, lock);
			*pfrc = rc;
			return NULL;
		default:
			panic("Unexpected rc (%d).", rc);
		}
	}
}

static void pht_refill_fail(uintptr_t badvaddr, istate_t *istate)
{
	const char *symbol = symtab_fmt_name_lookup(istate->pc);
	const char *sym2 = symtab_fmt_name_lookup(istate->lr);
	
	fault_if_from_uspace(istate,
	    "PHT Refill Exception on %p.", badvaddr);
	panic("%p: PHT Refill Exception at %p (%s<-%s).", badvaddr,
	    istate->pc, symbol, sym2);
}

static void pht_insert(const uintptr_t vaddr, const pte_t *pte)
{
	uint32_t page = (vaddr >> 12) & 0xffff;
	uint32_t api = (vaddr >> 22) & 0x3f;
	
	uint32_t vsid = sr_get(vaddr);
	uint32_t sdr1 = sdr1_get();
	
	// FIXME: compute size of PHT exactly
	phte_t *phte = (phte_t *) PA2KA(sdr1 & 0xffff0000);
	
	/* Primary hash (xor) */
	uint32_t h = 0;
	uint32_t hash = vsid ^ page;
	uint32_t base = (hash & 0x3ff) << 3;
	uint32_t i;
	bool found = false;
	
	/* Find colliding PTE in PTEG */
	for (i = 0; i < 8; i++) {
		if ((phte[base + i].v)
		    && (phte[base + i].vsid == vsid)
		    && (phte[base + i].api == api)
		    && (phte[base + i].h == 0)) {
			found = true;
			break;
		}
	}
	
	if (!found) {
		/* Find unused PTE in PTEG */
		for (i = 0; i < 8; i++) {
			if (!phte[base + i].v) {
				found = true;
				break;
			}
		}
	}
	
	if (!found) {
		/* Secondary hash (not) */
		uint32_t base2 = (~hash & 0x3ff) << 3;
		
		/* Find colliding PTE in PTEG */
		for (i = 0; i < 8; i++) {
			if ((phte[base2 + i].v)
			    && (phte[base2 + i].vsid == vsid)
			    && (phte[base2 + i].api == api)
			    && (phte[base2 + i].h == 1)) {
				found = true;
				base = base2;
				h = 1;
				break;
			}
		}
		
		if (!found) {
			/* Find unused PTE in PTEG */
			for (i = 0; i < 8; i++) {
				if (!phte[base2 + i].v) {
					found = true;
					base = base2;
					h = 1;
					break;
				}
			}
		}
		
		if (!found)
			i = RANDI(seed) % 8;
	}
	
	phte[base + i].v = 1;
	phte[base + i].vsid = vsid;
	phte[base + i].h = h;
	phte[base + i].api = api;
	phte[base + i].rpn = pte->pfn;
	phte[base + i].r = 0;
	phte[base + i].c = 0;
	phte[base + i].wimg = (pte->page_cache_disable ? WIMG_NO_CACHE : 0);
	phte[base + i].pp = 2; // FIXME
}

/** Process Instruction/Data Storage Exception
 *
 * @param n      Exception vector number.
 * @param istate Interrupted register context.
 *
 */
void pht_refill(int n, istate_t *istate)
{
	as_t *as;
	bool lock;
	
	if (AS == NULL) {
		as = AS_KERNEL;
		lock = false;
	} else {
		as = AS;
		lock = true;
	}
	
	uintptr_t badvaddr;
	
	if (n == VECTOR_DATA_STORAGE)
		badvaddr = istate->dar;
	else
		badvaddr = istate->pc;
	
	page_table_lock(as, lock);
	
	int pfrc;
	pte_t *pte = find_mapping_and_check(as, lock, badvaddr,
	    PF_ACCESS_READ /* FIXME */, istate, &pfrc);
	
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
			panic("Unexpected pfrc (%d).", pfrc);
		}
	}
	
	/* Record access to PTE */
	pte->accessed = 1;
	pht_insert(badvaddr, pte);
	
	page_table_unlock(as, lock);
	return;
	
fail:
	page_table_unlock(as, lock);
	pht_refill_fail(badvaddr, istate);
}

/** Process Instruction/Data Storage Exception in Real Mode
 *
 * @param n      Exception vector number.
 * @param istate Interrupted register context.
 *
 */
bool pht_refill_real(int n, istate_t *istate)
{
	uintptr_t badvaddr;
	
	if (n == VECTOR_DATA_STORAGE)
		badvaddr = istate->dar;
	else
		badvaddr = istate->pc;
	
	uint32_t physmem = physmem_top();
	
	if ((badvaddr < PA2KA(0)) || (badvaddr >= PA2KA(physmem)))
		return false;
	
	uint32_t page = (badvaddr >> 12) & 0xffff;
	uint32_t api = (badvaddr >> 22) & 0x3f;
	
	uint32_t vsid = sr_get(badvaddr);
	uint32_t sdr1 = sdr1_get();
	
	// FIXME: compute size of PHT exactly
	phte_t *phte_real = (phte_t *) (sdr1 & 0xffff0000);
	
	/* Primary hash (xor) */
	uint32_t h = 0;
	uint32_t hash = vsid ^ page;
	uint32_t base = (hash & 0x3ff) << 3;
	uint32_t i;
	bool found = false;
	
	/* Find colliding PTE in PTEG */
	for (i = 0; i < 8; i++) {
		if ((phte_real[base + i].v)
		    && (phte_real[base + i].vsid == vsid)
		    && (phte_real[base + i].api == api)
		    && (phte_real[base + i].h == 0)) {
			found = true;
			break;
		}
	}
	
	if (!found) {
		/* Find unused PTE in PTEG */
		for (i = 0; i < 8; i++) {
			if (!phte_real[base + i].v) {
				found = true;
				break;
			}
		}
	}
	
	if (!found) {
		/* Secondary hash (not) */
		uint32_t base2 = (~hash & 0x3ff) << 3;
		
		/* Find colliding PTE in PTEG */
		for (i = 0; i < 8; i++) {
			if ((phte_real[base2 + i].v)
			    && (phte_real[base2 + i].vsid == vsid)
			    && (phte_real[base2 + i].api == api)
			    && (phte_real[base2 + i].h == 1)) {
				found = true;
				base = base2;
				h = 1;
				break;
			}
		}
		
		if (!found) {
			/* Find unused PTE in PTEG */
			for (i = 0; i < 8; i++) {
				if (!phte_real[base2 + i].v) {
					found = true;
					base = base2;
					h = 1;
					break;
				}
			}
		}
		
		if (!found) {
			/* Use secondary hash to avoid collisions
			   with usual PHT refill handler. */
			i = RANDI(seed_real) % 8;
			base = base2;
			h = 1;
		}
	}
	
	phte_real[base + i].v = 1;
	phte_real[base + i].vsid = vsid;
	phte_real[base + i].h = h;
	phte_real[base + i].api = api;
	phte_real[base + i].rpn = KA2PA(badvaddr) >> 12;
	phte_real[base + i].r = 0;
	phte_real[base + i].c = 0;
	phte_real[base + i].wimg = 0;
	phte_real[base + i].pp = 2; // FIXME
	
	return true;
}

/** Process ITLB/DTLB Miss Exception in Real Mode
 *
 *
 */
void tlb_refill_real(int n, uint32_t tlbmiss, ptehi_t ptehi, ptelo_t ptelo, istate_t *istate)
{
	uint32_t badvaddr = tlbmiss & 0xfffffffc;
	uint32_t physmem = physmem_top();
	
	if ((badvaddr < PA2KA(0)) || (badvaddr >= PA2KA(physmem)))
		return; // FIXME
	
	ptelo.rpn = KA2PA(badvaddr) >> 12;
	ptelo.wimg = 0;
	ptelo.pp = 2; // FIXME
	
	uint32_t index = 0;
	asm volatile (
		"mtspr 981, %[ptehi]\n"
		"mtspr 982, %[ptelo]\n"
		"tlbld %[index]\n"
		"tlbli %[index]\n"
		: [index] "=r" (index)
		: [ptehi] "r" (ptehi),
		  [ptelo] "r" (ptelo)
	);
}

void tlb_arch_init(void)
{
	tlb_invalidate_all();
}

void tlb_invalidate_all(void)
{
	uint32_t index;
	
	asm volatile (
		"li %[index], 0\n"
		"sync\n"
		
		".rept 64\n"
		"	tlbie %[index]\n"
		"	addi %[index], %[index], 0x1000\n"
		".endr\n"
		
		"eieio\n"
		"tlbsync\n"
		"sync\n"
		: [index] "=r" (index)
	);
}

void tlb_invalidate_asid(asid_t asid)
{
	uint32_t sdr1 = sdr1_get();
	
	// FIXME: compute size of PHT exactly
	phte_t *phte = (phte_t *) PA2KA(sdr1 & 0xffff0000);
	
	size_t i;
	for (i = 0; i < 8192; i++) {
		if ((phte[i].v) && (phte[i].vsid >= (asid << 4)) &&
		    (phte[i].vsid < ((asid << 4) + 16)))
			phte[i].v = 0;
	}
	
	tlb_invalidate_all();
}

void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	// TODO
	tlb_invalidate_all();
}

#define PRINT_BAT(name, ureg, lreg) \
	asm volatile ( \
		"mfspr %[upper], " #ureg "\n" \
		"mfspr %[lower], " #lreg "\n" \
		: [upper] "=r" (upper), \
		  [lower] "=r" (lower) \
	); \
	\
	mask = (upper & 0x1ffc) >> 2; \
	if (upper & 3) { \
		uint32_t tmp = mask; \
		length = 128; \
		\
		while (tmp) { \
			if ((tmp & 1) == 0) { \
				printf("ibat[0]: error in mask\n"); \
				break; \
			} \
			length <<= 1; \
			tmp >>= 1; \
		} \
	} else \
		length = 0; \
	\
	printf(name ": page=%.*p frame=%.*p length=%d KB (mask=%#x)%s%s\n", \
	    sizeof(upper) * 2, upper & 0xffff0000, sizeof(lower) * 2, \
	    lower & 0xffff0000, length, mask, \
	    ((upper >> 1) & 1) ? " supervisor" : "", \
	    (upper & 1) ? " user" : "");


void tlb_print(void)
{
	uint32_t sr;
	
	for (sr = 0; sr < 16; sr++) {
		uint32_t vsid = sr_get(sr << 28);
		
		printf("sr[%02u]: vsid=%.*p (asid=%u)%s%s\n", sr,
		    sizeof(vsid) * 2, vsid & 0xffffff, (vsid & 0xffffff) >> 4,
		    ((vsid >> 30) & 1) ? " supervisor" : "",
		    ((vsid >> 29) & 1) ? " user" : "");
	}
	
	uint32_t upper;
	uint32_t lower;
	uint32_t mask;
	uint32_t length;
	
	PRINT_BAT("ibat[0]", 528, 529);
	PRINT_BAT("ibat[1]", 530, 531);
	PRINT_BAT("ibat[2]", 532, 533);
	PRINT_BAT("ibat[3]", 534, 535);
	
	PRINT_BAT("dbat[0]", 536, 537);
	PRINT_BAT("dbat[1]", 538, 539);
	PRINT_BAT("dbat[2]", 540, 541);
	PRINT_BAT("dbat[3]", 542, 543);
}

/** @}
 */
