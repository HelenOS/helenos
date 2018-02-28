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

/** @addtogroup mips32mm
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
#include <log.h>
#include <assert.h>
#include <align.h>
#include <interrupt.h>
#include <symtab.h>

#define PFN_SHIFT  12
#define VPN_SHIFT  12

#define ADDR2HI_VPN(a)   ((a) >> VPN_SHIFT)
#define ADDR2HI_VPN2(a)  (ADDR2HI_VPN((a)) >> 1)

#define HI_VPN2ADDR(vpn)    ((vpn) << VPN_SHIFT)
#define HI_VPN22ADDR(vpn2)  (HI_VPN2ADDR(vpn2) << 1)

#define LO_PFN2ADDR(pfn)  ((pfn) << PFN_SHIFT)

#define BANK_SELECT_BIT(a)  (((a) >> PAGE_WIDTH) & 1)

/** Initialize TLB.
 *
 * Invalidate all entries and mark wired entries.
 */
void tlb_arch_init(void)
{
	int i;

	cp0_pagemask_write(TLB_PAGE_MASK_16K);
	cp0_entry_hi_write(0);
	cp0_entry_lo0_write(0);
	cp0_entry_lo1_write(0);

	/* Clear and initialize TLB. */

	for (i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbwi();
	}

	/*
	 * The kernel is going to make use of some wired
	 * entries (e.g. mapping kernel stacks in kseg3).
	 */
	cp0_wired_write(TLB_WIRED);
}

/** Process TLB Refill Exception.
 *
 * @param istate	Interrupted register context.
 */
void tlb_refill(istate_t *istate)
{
	entry_lo_t lo;
	uintptr_t badvaddr;
	pte_t pte;

	badvaddr = cp0_badvaddr_read();

	bool found = page_mapping_find(AS, badvaddr, true, &pte);
	if (found && pte.p) {
		/*
		 * Record access to PTE.
		 */
		pte.a = 1;

		tlb_prepare_entry_lo(&lo, pte.g, pte.p, pte.d,
		    pte.cacheable, pte.pfn);

		page_mapping_update(AS, badvaddr, true, &pte);

		/*
		 * New entry is to be inserted into TLB
		 */
		if (BANK_SELECT_BIT(badvaddr) == 0) {
			cp0_entry_lo0_write(lo.value);
			cp0_entry_lo1_write(0);
		} else {
			cp0_entry_lo0_write(0);
			cp0_entry_lo1_write(lo.value);
		}
		cp0_pagemask_write(TLB_PAGE_MASK_16K);
		tlbwr();
		return;
	}

	(void) as_page_fault(badvaddr, PF_ACCESS_READ, istate);
}

/** Process TLB Invalid Exception.
 *
 * @param istate	Interrupted register context.
 */
void tlb_invalid(istate_t *istate)
{
	entry_lo_t lo;
	tlb_index_t index;
	uintptr_t badvaddr;
	pte_t pte;

	/*
	 * Locate the faulting entry in TLB.
	 */
	tlbp();
	index.value = cp0_index_read();

#if defined(PROCESSOR_4Kc)
	/*
	 * This can happen on a 4Kc when Status.EXL is 1 and there is a TLB miss.
	 * EXL is 1 when interrupts are disabled. The combination of a TLB miss
	 * and disabled interrupts is possible in copy_to/from_uspace().
	 */
	if (index.p) {
		tlb_refill(istate);
		return;
	}
#endif

	assert(!index.p);

	badvaddr = cp0_badvaddr_read();

	bool found = page_mapping_find(AS, badvaddr, true, &pte);
	if (found && pte.p) {
		/*
		 * Read the faulting TLB entry.
		 */
		tlbr();

		/*
		 * Record access to PTE.
		 */
		pte.a = 1;

		tlb_prepare_entry_lo(&lo, pte.g, pte.p, pte.d,
		    pte.cacheable, pte.pfn);

		page_mapping_update(AS, badvaddr, true, &pte);

		/*
		 * The entry is to be updated in TLB.
		 */
		if (BANK_SELECT_BIT(badvaddr) == 0)
			cp0_entry_lo0_write(lo.value);
		else
			cp0_entry_lo1_write(lo.value);
		tlbwi();
		return;
	}

	(void) as_page_fault(badvaddr, PF_ACCESS_READ, istate);
}

/** Process TLB Modified Exception.
 *
 * @param istate	Interrupted register context.
 */
void tlb_modified(istate_t *istate)
{
	entry_lo_t lo;
	tlb_index_t index;
	uintptr_t badvaddr;
	pte_t pte;

	badvaddr = cp0_badvaddr_read();

	/*
	 * Locate the faulting entry in TLB.
	 */
	tlbp();
	index.value = cp0_index_read();

	/*
	 * Emit warning if the entry is not in TLB.
	 *
	 * We do not assert on this because this could be a manifestation of
	 * an emulator bug, such as QEMU Bug #1128935:
	 * https://bugs.launchpad.net/qemu/+bug/1128935
	 */
	if (index.p) {
		log(LF_ARCH, LVL_WARN, "%s: TLBP failed in exception handler (badvaddr=%#"
		    PRIxn ", ASID=%d).\n", __func__, badvaddr,
		    AS ? AS->asid : -1);
		return;
	}

	bool found = page_mapping_find(AS, badvaddr, true, &pte);
	if (found && pte.p && pte.w) {
		/*
		 * Read the faulting TLB entry.
		 */
		tlbr();

		/*
		 * Record access and write to PTE.
		 */
		pte.a = 1;
		pte.d = 1;

		tlb_prepare_entry_lo(&lo, pte.g, pte.p, pte.w,
		    pte.cacheable, pte.pfn);

		page_mapping_update(AS, badvaddr, true, &pte);

		/*
		 * The entry is to be updated in TLB.
		 */
		if (BANK_SELECT_BIT(badvaddr) == 0)
			cp0_entry_lo0_write(lo.value);
		else
			cp0_entry_lo1_write(lo.value);
		tlbwi();
		return;
	}

	(void) as_page_fault(badvaddr, PF_ACCESS_WRITE, istate);
}

void
tlb_prepare_entry_lo(entry_lo_t *lo, bool g, bool v, bool d, bool cacheable,
    uintptr_t pfn)
{
	lo->value = 0;
	lo->g = g;
	lo->v = v;
	lo->d = d;
	lo->c = cacheable ? PAGE_CACHEABLE_EXC_WRITE : PAGE_UNCACHED;
	lo->pfn = pfn;
}

void tlb_prepare_entry_hi(entry_hi_t *hi, asid_t asid, uintptr_t addr)
{
	hi->value = 0;
	hi->vpn2 = ADDR2HI_VPN2(ALIGN_DOWN(addr, PAGE_SIZE));
	hi->asid = asid;
}

/** Print contents of TLB. */
void tlb_print(void)
{
	page_mask_t mask, mask_save;
	entry_lo_t lo0, lo0_save, lo1, lo1_save;
	entry_hi_t hi, hi_save;
	unsigned int i;

	hi_save.value = cp0_entry_hi_read();
	lo0_save.value = cp0_entry_lo0_read();
	lo1_save.value = cp0_entry_lo1_read();
	mask_save.value = cp0_pagemask_read();

	printf("[nr] [asid] [vpn2    ] [mask] [gvdc] [pfn     ]\n");

	for (i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();

		mask.value = cp0_pagemask_read();
		hi.value = cp0_entry_hi_read();
		lo0.value = cp0_entry_lo0_read();
		lo1.value = cp0_entry_lo1_read();

		printf("%-4u %-6u %0#10x %-#6x  %1u%1u%1u%1u  %0#10x\n",
		    i, hi.asid, HI_VPN22ADDR(hi.vpn2), mask.mask,
		    lo0.g, lo0.v, lo0.d, lo0.c, LO_PFN2ADDR(lo0.pfn));
		printf("                               %1u%1u%1u%1u  %0#10x\n",
		    lo1.g, lo1.v, lo1.d, lo1.c, LO_PFN2ADDR(lo1.pfn));
	}

	cp0_entry_hi_write(hi_save.value);
	cp0_entry_lo0_write(lo0_save.value);
	cp0_entry_lo1_write(lo1_save.value);
	cp0_pagemask_write(mask_save.value);
}

/** Invalidate all not wired TLB entries. */
void tlb_invalidate_all(void)
{
	entry_lo_t lo0, lo1;
	entry_hi_t hi_save;
	int i;

	assert(interrupts_disabled());

	hi_save.value = cp0_entry_hi_read();

	for (i = TLB_WIRED; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();

		lo0.value = cp0_entry_lo0_read();
		lo1.value = cp0_entry_lo1_read();

		lo0.v = 0;
		lo1.v = 0;

		cp0_entry_lo0_write(lo0.value);
		cp0_entry_lo1_write(lo1.value);

		tlbwi();
	}

	cp0_entry_hi_write(hi_save.value);
}

/** Invalidate all TLB entries belonging to specified address space.
 *
 * @param asid Address space identifier.
 */
void tlb_invalidate_asid(asid_t asid)
{
	entry_lo_t lo0, lo1;
	entry_hi_t hi, hi_save;
	int i;

	assert(interrupts_disabled());
	assert(asid != ASID_INVALID);

	hi_save.value = cp0_entry_hi_read();

	for (i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbr();

		hi.value = cp0_entry_hi_read();

		if (hi.asid == asid) {
			lo0.value = cp0_entry_lo0_read();
			lo1.value = cp0_entry_lo1_read();

			lo0.v = 0;
			lo1.v = 0;

			cp0_entry_lo0_write(lo0.value);
			cp0_entry_lo1_write(lo1.value);

			tlbwi();
		}
	}

	cp0_entry_hi_write(hi_save.value);
}

/** Invalidate TLB entries for specified page range belonging to specified
 * address space.
 *
 * @param asid		Address space identifier.
 * @param page		First page whose TLB entry is to be invalidated.
 * @param cnt		Number of entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	unsigned int i;
	entry_lo_t lo0, lo1;
	entry_hi_t hi, hi_save;
	tlb_index_t index;

	assert(interrupts_disabled());

	if (asid == ASID_INVALID)
		return;

	hi_save.value = cp0_entry_hi_read();

	for (i = 0; i < cnt + 1; i += 2) {
		tlb_prepare_entry_hi(&hi, asid, page + i * PAGE_SIZE);
		cp0_entry_hi_write(hi.value);

		tlbp();
		index.value = cp0_index_read();

		if (!index.p) {
			/*
			 * Entry was found, index register contains valid
			 * index.
			 */
			tlbr();

			lo0.value = cp0_entry_lo0_read();
			lo1.value = cp0_entry_lo1_read();

			lo0.v = 0;
			lo1.v = 0;

			cp0_entry_lo0_write(lo0.value);
			cp0_entry_lo1_write(lo1.value);

			tlbwi();
		}
	}

	cp0_entry_hi_write(hi_save.value);
}

/** @}
 */
