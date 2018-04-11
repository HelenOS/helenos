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

#include <arch/mm/pht.h>
#include <arch/mm/tlb.h>
#include <assert.h>
#include <interrupt.h>
#include <mm/as.h>
#include <mm/page.h>
#include <macros.h>
#include <typedefs.h>

static unsigned int seed = 42;

/** Try to find PTE for faulting address
 *
 * @param as       Address space.
 * @param badvaddr Faulting virtual address.
 * @param access   Access mode that caused the fault.
 * @param istate   Pointer to interrupted state.
 * @param[out] pte Structure that will receive a copy of the found PTE.
 *
 * @return True if the mapping was found, false otherwise.
 *
 */
static bool find_mapping_and_check(as_t *as, uintptr_t badvaddr, int access,
    istate_t *istate, pte_t *pte)
{
	/*
	 * Check if the mapping exists in page tables.
	 */
	bool found = page_mapping_find(as, badvaddr, true, pte);
	if (found && pte->present) {
		/*
		 * Mapping found in page tables.
		 * Immediately succeed.
		 */
		return true;
	}
	/*
	 * Mapping not found in page tables.
	 * Resort to higher-level page fault handler.
	 */
	if (as_page_fault(badvaddr, access, istate) == AS_PF_OK) {
		/*
		 * The higher-level page fault handler succeeded,
		 * The mapping ought to be in place.
		 */
		found = page_mapping_find(as, badvaddr, true, pte);

		assert(found);
		assert(pte->present);

		return found;
	}

	return false;
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
		if ((phte[base + i].v) &&
		    (phte[base + i].vsid == vsid) &&
		    (phte[base + i].api == api) &&
		    (phte[base + i].h == 0)) {
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
			if ((phte[base2 + i].v) &&
			    (phte[base2 + i].vsid == vsid) &&
			    (phte[base2 + i].api == api) &&
			    (phte[base2 + i].h == 1)) {
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
void pht_refill(unsigned int n, istate_t *istate)
{
	uintptr_t badvaddr;

	if (n == VECTOR_DATA_STORAGE)
		badvaddr = istate->dar;
	else
		badvaddr = istate->pc;

	pte_t pte;
	bool found = find_mapping_and_check(AS, badvaddr,
	    PF_ACCESS_READ /* FIXME */, istate, &pte);

	if (found) {
		/* Record access to PTE */
		pte.accessed = 1;
		pht_insert(badvaddr, &pte);
	}
}

void pht_invalidate(as_t *as, uintptr_t page, size_t pages)
{
	uint32_t sdr1 = sdr1_get();

	// FIXME: compute size of PHT exactly
	phte_t *phte = (phte_t *) PA2KA(sdr1 & 0xffff0000);

	// FIXME: this invalidates all PHT entries,
	// which is an overkill, invalidate only
	// selectively
	for (size_t i = 0; i < 8192; i++) {
		phte[i].v = 0;
	}
}

/** @}
 */
