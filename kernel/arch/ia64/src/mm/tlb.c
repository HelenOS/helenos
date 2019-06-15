/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

/*
 * TLB management.
 */

#include <mm/tlb.h>
#include <mm/asid.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <arch/mm/vhpt.h>
#include <barrier.h>
#include <arch/interrupt.h>
#include <arch/pal/pal.h>
#include <arch/asm.h>
#include <assert.h>
#include <panic.h>
#include <arch.h>
#include <interrupt.h>
#include <arch/legacyio.h>

/** Invalidate all TLB entries. */
void tlb_invalidate_all(void)
{
	ipl_t ipl;
	uintptr_t adr;
	uint32_t count1, count2, stride1, stride2;

	unsigned int i, j;

	adr = PAL_PTCE_INFO_BASE();
	count1 = PAL_PTCE_INFO_COUNT1();
	count2 = PAL_PTCE_INFO_COUNT2();
	stride1 = PAL_PTCE_INFO_STRIDE1();
	stride2 = PAL_PTCE_INFO_STRIDE2();

	ipl = interrupts_disable();

	for (i = 0; i < count1; i++) {
		for (j = 0; j < count2; j++) {
			asm volatile (
			    "ptc.e %[adr] ;;"
			    :: [adr] "r" (adr)
			);
			adr += stride2;
		}
		adr += stride1;
	}

	interrupts_restore(ipl);

	srlz_d();
	srlz_i();

#ifdef CONFIG_VHPT
	vhpt_invalidate_all();
#endif
}

/** Invalidate entries belonging to an address space.
 *
 * @param asid Address space identifier.
 *
 */
void tlb_invalidate_asid(asid_t asid)
{
	tlb_invalidate_all();
}

void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	region_register_t rr;
	bool restore_rr = false;
	int b = 0;
	int c = cnt;

	uintptr_t va;
	va = page;

	rr.word = rr_read(VA2VRN(page));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(page))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register_t rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(page));
		rr_write(VA2VRN(page), rr0.word);
		srlz_d();
		srlz_i();
	}

	while (c >>= 1)
		b++;
	b >>= 1;
	uint64_t ps;

	switch (b) {
	case 0: /* cnt 1 - 3 */
		ps = PAGE_WIDTH;
		break;
	case 1: /* cnt 4 - 15 */
		ps = PAGE_WIDTH + 2;
		va &= ~((1UL << ps) - 1);
		break;
	case 2: /* cnt 16 - 63 */
		ps = PAGE_WIDTH + 4;
		va &= ~((1UL << ps) - 1);
		break;
	case 3: /* cnt 64 - 255 */
		ps = PAGE_WIDTH + 6;
		va &= ~((1UL << ps) - 1);
		break;
	case 4: /* cnt 256 - 1023 */
		ps = PAGE_WIDTH + 8;
		va &= ~((1UL << ps) - 1);
		break;
	case 5: /* cnt 1024 - 4095 */
		ps = PAGE_WIDTH + 10;
		va &= ~((1UL << ps) - 1);
		break;
	case 6: /* cnt 4096 - 16383 */
		ps = PAGE_WIDTH + 12;
		va &= ~((1UL << ps) - 1);
		break;
	case 7: /* cnt 16384 - 65535 */
	case 8: /* cnt 65536 - (256K - 1) */
		ps = PAGE_WIDTH + 14;
		va &= ~((1UL << ps) - 1);
		break;
	default:
		ps = PAGE_WIDTH + 18;
		va &= ~((1UL << ps) - 1);
		break;
	}

	for (; va < (page + cnt * PAGE_SIZE); va += (1UL << ps))
		asm volatile (
		    "ptc.l %[va], %[ps] ;;"
		    :: [va] "r" (va),
		      [ps] "r" (ps << 2)
		);

	srlz_d();
	srlz_i();

	if (restore_rr) {
		rr_write(VA2VRN(page), rr.word);
		srlz_d();
		srlz_i();
	}
}

/** Insert data into data translation cache.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 *
 */
void dtc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry)
{
	tc_mapping_insert(va, asid, entry, true);
}

/** Insert data into instruction translation cache.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 */
void itc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry)
{
	tc_mapping_insert(va, asid, entry, false);
}

/** Insert data into instruction or data translation cache.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 * @param dtc   If true, insert into data translation cache, use
 *              instruction translation cache otherwise.
 *
 */
void tc_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, bool dtc)
{
	region_register_t rr;
	bool restore_rr = false;

	rr.word = rr_read(VA2VRN(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register_t rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(va));
		rr_write(VA2VRN(va), rr0.word);
		srlz_d();
		srlz_i();
	}

	asm volatile (
	    "mov r8 = psr ;;\n"
	    "rsm %[mask] ;;\n"                 /* PSR_IC_MASK */
	    "srlz.d ;;\n"
	    "srlz.i ;;\n"
	    "mov cr.ifa = %[va]\n"             /* va */
	    "mov cr.itir = %[word1] ;;\n"      /* entry.word[1] */
	    "cmp.eq p6, p7 = %[dtc], r0 ;;\n"  /* decide between itc and dtc */
	    "(p6) itc.i %[word0] ;;\n"
	    "(p7) itc.d %[word0] ;;\n"
	    "mov psr.l = r8 ;;\n"
	    "srlz.d ;;\n"
	    :: [mask] "i" (PSR_IC_MASK),
	      [va] "r" (va),
	      [word0] "r" (entry.word[0]),
	      [word1] "r" (entry.word[1]),
	      [dtc] "r" (dtc)
	    : "p6", "p7", "r8"
	);

	if (restore_rr) {
		rr_write(VA2VRN(va), rr.word);
		srlz_d();
		srlz_i();
	}
}

/** Insert data into instruction translation register.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 * @param tr    Translation register.
 *
 */
void itr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, size_t tr)
{
	tr_mapping_insert(va, asid, entry, false, tr);
}

/** Insert data into data translation register.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 * @param tr    Translation register.
 *
 */
void dtr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, size_t tr)
{
	tr_mapping_insert(va, asid, entry, true, tr);
}

/** Insert data into instruction or data translation register.
 *
 * @param va    Virtual page address.
 * @param asid  Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion
 *              format.
 * @param dtr   If true, insert into data translation register, use
 *              instruction translation register otherwise.
 * @param tr    Translation register.
 *
 */
void tr_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry, bool dtr,
    size_t tr)
{
	region_register_t rr;
	bool restore_rr = false;

	rr.word = rr_read(VA2VRN(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register_t rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(va));
		rr_write(VA2VRN(va), rr0.word);
		srlz_d();
		srlz_i();
	}

	asm volatile (
	    "mov r8 = psr ;;\n"
	    "rsm %[mask] ;;\n"                       /* PSR_IC_MASK */
	    "srlz.d ;;\n"
	    "srlz.i ;;\n"
	    "mov cr.ifa = %[va]\n"                   /* va */
	    "mov cr.itir = %[word1] ;;\n"            /* entry.word[1] */
	    "cmp.eq p6, p7 = %[dtr], r0 ;;\n"        /* decide between itr and dtr */
	    "(p6) itr.i itr[%[tr]] = %[word0] ;;\n"
	    "(p7) itr.d dtr[%[tr]] = %[word0] ;;\n"
	    "mov psr.l = r8 ;;\n"
	    "srlz.d ;;\n"
	    :: [mask] "i" (PSR_IC_MASK),
	      [va] "r" (va),
	      [word1] "r" (entry.word[1]),
	      [word0] "r" (entry.word[0]),
	      [tr] "r" (tr),
	      [dtr] "r" (dtr)
	    : "p6", "p7", "r8"
	);

	if (restore_rr) {
		rr_write(VA2VRN(va), rr.word);
		srlz_d();
		srlz_i();
	}
}

/** Insert data into DTLB.
 *
 * @param page  Virtual page address including VRN bits.
 * @param frame Physical frame address.
 * @param dtr   If true, insert into data translation register, use data
 *              translation cache otherwise.
 * @param tr    Translation register if dtr is true, ignored otherwise.
 *
 */
void dtlb_kernel_mapping_insert(uintptr_t page, uintptr_t frame, bool dtr,
    size_t tr)
{
	tlb_entry_t entry;

	entry.word[0] = 0;
	entry.word[1] = 0;

	entry.p = true;           /* present */
	entry.ma = MA_WRITEBACK;
	entry.a = true;           /* already accessed */
	entry.d = true;           /* already dirty */
	entry.pl = PL_KERNEL;
	entry.ar = AR_READ | AR_WRITE;
	entry.ppn = frame >> PPN_SHIFT;
	entry.ps = PAGE_WIDTH;

	if (dtr)
		dtr_mapping_insert(page, ASID_KERNEL, entry, tr);
	else
		dtc_mapping_insert(page, ASID_KERNEL, entry);
}

/** Purge kernel entries from DTR.
 *
 * Purge DTR entries used by the kernel.
 *
 * @param page  Virtual page address including VRN bits.
 * @param width Width of the purge in bits.
 *
 */
void dtr_purge(uintptr_t page, size_t width)
{
	asm volatile (
	    "ptr.d %[page], %[width]\n"
	    :: [page] "r" (page),
	      [width] "r" (width << 2)
	);
}

/** Copy content of PTE into data translation cache.
 *
 * @param t PTE.
 *
 */
void dtc_pte_copy(pte_t *t)
{
	tlb_entry_t entry;

	entry.word[0] = 0;
	entry.word[1] = 0;

	entry.p = t->p;
	entry.ma = t->c ? MA_WRITEBACK : MA_UNCACHEABLE;
	entry.a = t->a;
	entry.d = t->d;
	entry.pl = t->k ? PL_KERNEL : PL_USER;
	entry.ar = t->w ? AR_WRITE : AR_READ;
	entry.ppn = t->frame >> PPN_SHIFT;
	entry.ps = PAGE_WIDTH;

	dtc_mapping_insert(t->page, t->as->asid, entry);

#ifdef CONFIG_VHPT
	vhpt_mapping_insert(t->page, t->as->asid, entry);
#endif
}

/** Copy content of PTE into instruction translation cache.
 *
 * @param t PTE.
 *
 */
void itc_pte_copy(pte_t *t)
{
	tlb_entry_t entry;

	entry.word[0] = 0;
	entry.word[1] = 0;

	assert(t->x);

	entry.p = t->p;
	entry.ma = t->c ? MA_WRITEBACK : MA_UNCACHEABLE;
	entry.a = t->a;
	entry.pl = t->k ? PL_KERNEL : PL_USER;
	entry.ar = t->x ? (AR_EXECUTE | AR_READ) : AR_READ;
	entry.ppn = t->frame >> PPN_SHIFT;
	entry.ps = PAGE_WIDTH;

	itc_mapping_insert(t->page, t->as->asid, entry);

#ifdef CONFIG_VHPT
	vhpt_mapping_insert(t->page, t->as->asid, entry);
#endif
}

static bool is_kernel_fault(istate_t *istate, uintptr_t va)
{
	region_register_t rr;

	if (istate_from_uspace(istate))
		return false;

	rr.word = rr_read(VA2VRN(va));
	rid_t rid = rr.map.rid;
	return (RID2ASID(rid) == ASID_KERNEL) && (VA2VRN(va) == VRN_KERNEL);
}

/** Instruction TLB fault handler for faults with VHPT turned off.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void alternate_instruction_tlb_fault(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;

	assert(istate_from_uspace(istate));

	va = istate->cr_ifa; /* faulting address */

	bool found = page_mapping_find(AS, va, true, &t);
	if (found) {
		assert(t.p);

		/*
		 * The mapping was found in software page hash table.
		 * Insert it into data translation cache.
		 */
		itc_pte_copy(&t);
	} else {
		/*
		 * Forward the page fault to address space page fault handler.
		 */
		as_page_fault(va, PF_ACCESS_EXEC, istate);
	}
}

static int is_io_page_accessible(int page)
{
	if (TASK->arch.iomap)
		return bitmap_get(TASK->arch.iomap, page);
	else
		return 0;
}

/**
 * There is special handling of memory mapped legacy io, because of 4KB sized
 * access for userspace.
 *
 * @param va     Virtual address of page fault.
 * @param istate Structure with saved interruption state.
 *
 * @return One on success, zero on failure.
 *
 */
static int try_memmap_io_insertion(uintptr_t va, istate_t *istate)
{
	if ((va >= LEGACYIO_USER_BASE) && (va < LEGACYIO_USER_BASE + (1 << LEGACYIO_PAGE_WIDTH))) {
		if (TASK) {
			uint64_t io_page = (va & ((1 << LEGACYIO_PAGE_WIDTH) - 1)) >>
			    LEGACYIO_SINGLE_PAGE_WIDTH;

			if (is_io_page_accessible(io_page)) {
				uint64_t page, frame;

				page = LEGACYIO_USER_BASE +
				    (1 << LEGACYIO_SINGLE_PAGE_WIDTH) * io_page;
				frame = LEGACYIO_PHYS_BASE +
				    (1 << LEGACYIO_SINGLE_PAGE_WIDTH) * io_page;

				tlb_entry_t entry;

				entry.word[0] = 0;
				entry.word[1] = 0;

				entry.p = true;             /* present */
				entry.ma = MA_UNCACHEABLE;
				entry.a = true;             /* already accessed */
				entry.d = true;             /* already dirty */
				entry.pl = PL_USER;
				entry.ar = AR_READ | AR_WRITE;
				entry.ppn = frame >> PPN_SHIFT;
				entry.ps = LEGACYIO_SINGLE_PAGE_WIDTH;

				dtc_mapping_insert(page, TASK->as->asid, entry);
				return 1;
			} else {
				fault_if_from_uspace(istate,
				    "IO access fault at %p.", (void *) va);
			}
		}
	}

	return 0;
}

/** Data TLB fault handler for faults with VHPT turned off.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void alternate_data_tlb_fault(unsigned int n, istate_t *istate)
{
	if (istate->cr_isr.sp) {
		/*
		 * Speculative load. Deffer the exception until a more clever
		 * approach can be used. Currently if we try to find the
		 * mapping for the speculative load while in the kernel, we
		 * might introduce a livelock because of the possibly invalid
		 * values of the address.
		 */
		istate->cr_ipsr.ed = true;
		return;
	}

	uintptr_t va = istate->cr_ifa;  /* faulting address */
	as_t *as = AS;

	if (is_kernel_fault(istate, va)) {
		if (va < end_of_identity) {
			/*
			 * Create kernel identity mapping for low memory.
			 */
			dtlb_kernel_mapping_insert(va, KA2PA(va), false, 0);
			return;
		} else {
			as = AS_KERNEL;
		}
	}

	pte_t t;
	bool found = page_mapping_find(as, va, true, &t);
	if (found) {
		assert(t.p);

		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into data translation cache.
		 */
		dtc_pte_copy(&t);
	} else {
		if (try_memmap_io_insertion(va, istate))
			return;

		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */
		as_page_fault(va, PF_ACCESS_READ, istate);
	}
}

/** Data nested TLB fault handler.
 *
 * This fault should not occur.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void data_nested_tlb_fault(unsigned int n, istate_t *istate)
{
	assert(false);
}

/** Data Dirty bit fault handler.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void data_dirty_bit_fault(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;
	as_t *as = AS;

	va = istate->cr_ifa;  /* faulting address */

	if (is_kernel_fault(istate, va))
		as = AS_KERNEL;

	bool found = page_mapping_find(as, va, true, &t);

	assert(found);
	assert(t.p);

	if (found && t.p && t.w) {
		/*
		 * Update the Dirty bit in page tables and reinsert
		 * the mapping into DTC.
		 */
		t.d = true;
		dtc_pte_copy(&t);
		page_mapping_update(as, va, true, &t);
	} else {
		as_page_fault(va, PF_ACCESS_WRITE, istate);
	}
}

/** Instruction access bit fault handler.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void instruction_access_bit_fault(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;

	assert(istate_from_uspace(istate));

	va = istate->cr_ifa;  /* faulting address */

	bool found = page_mapping_find(AS, va, true, &t);

	assert(found);
	assert(t.p);

	if (found && t.p && t.x) {
		/*
		 * Update the Accessed bit in page tables and reinsert
		 * the mapping into ITC.
		 */
		t.a = true;
		itc_pte_copy(&t);
		page_mapping_update(AS, va, true, &t);
	} else {
		as_page_fault(va, PF_ACCESS_EXEC, istate);
	}
}

/** Data access bit fault handler.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void data_access_bit_fault(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;
	as_t *as = AS;

	va = istate->cr_ifa;  /* faulting address */

	if (is_kernel_fault(istate, va))
		as = AS_KERNEL;

	bool found = page_mapping_find(as, va, true, &t);

	assert(found);
	assert(t.p);

	if (found && t.p) {
		/*
		 * Update the Accessed bit in page tables and reinsert
		 * the mapping into DTC.
		 */
		t.a = true;
		dtc_pte_copy(&t);
		page_mapping_update(as, va, true, &t);
	} else {
		if (as_page_fault(va, PF_ACCESS_READ, istate) == AS_PF_FAULT) {
			fault_if_from_uspace(istate, "Page fault at %p.",
			    (void *) va);
			panic_memtrap(istate, PF_ACCESS_UNKNOWN, va, NULL);
		}
	}
}

/** Data access rights fault handler.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void data_access_rights_fault(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;

	assert(istate_from_uspace(istate));

	va = istate->cr_ifa;  /* faulting address */

	/*
	 * Assume a write to a read-only page.
	 */
	bool found = page_mapping_find(AS, va, true, &t);

	assert(found);
	assert(t.p);
	assert(!t.w);

	as_page_fault(va, PF_ACCESS_WRITE, istate);
}

/** Page not present fault handler.
 *
 * @param n Interruption vector.
 * @param istate Structure with saved interruption state.
 *
 */
void page_not_present(unsigned int n, istate_t *istate)
{
	uintptr_t va;
	pte_t t;

	assert(istate_from_uspace(istate));

	va = istate->cr_ifa;  /* faulting address */

	bool found = page_mapping_find(AS, va, true, &t);

	assert(found);

	if (t.p) {
		/*
		 * If the Present bit is set in page hash table, just copy it
		 * and update ITC/DTC.
		 */
		if (t.x)
			itc_pte_copy(&t);
		else
			dtc_pte_copy(&t);
	} else {
		as_page_fault(va, PF_ACCESS_READ, istate);
	}
}

void tlb_arch_init(void)
{
}

void tlb_print(void)
{
}

/** @}
 */
