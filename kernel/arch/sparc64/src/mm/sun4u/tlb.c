/*
 * Copyright (c) 2005 Jakub Jermar
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

#include <arch/mm/tlb.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <mm/asid.h>
#include <arch/mm/frame.h>
#include <arch/mm/page.h>
#include <arch/mm/mmu.h>
#include <arch/interrupt.h>
#include <assert.h>
#include <interrupt.h>
#include <arch.h>
#include <print.h>
#include <typedefs.h>
#include <config.h>
#include <arch/trap/trap.h>
#include <arch/trap/exception.h>
#include <panic.h>
#include <arch/asm.h>
#include <genarch/mm/page_ht.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

static void dtlb_pte_copy(pte_t *, size_t, bool);
static void itlb_pte_copy(pte_t *, size_t);

const char *context_encoding[] = {
	"Primary",
	"Secondary",
	"Nucleus",
	"Reserved"
};

void tlb_arch_init(void)
{
	/*
	 * Invalidate all non-locked DTLB and ITLB entries.
	 */
	tlb_invalidate_all();

	/*
	 * Clear both SFSRs.
	 */
	dtlb_sfsr_write(0);
	itlb_sfsr_write(0);
}

/** Insert privileged mapping into DMMU TLB.
 *
 * @param page		Virtual page address.
 * @param frame		Physical frame address.
 * @param pagesize	Page size.
 * @param locked	True for permanent mappings, false otherwise.
 * @param cacheable	True if the mapping is cacheable, false otherwise.
 */
void dtlb_insert_mapping(uintptr_t page, uintptr_t frame, int pagesize,
    bool locked, bool cacheable)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = page;
	fr.address = frame;

	tag.context = ASID_KERNEL;
	tag.vpn = pg.vpn;

	dtlb_tag_access_write(tag.value);

	data.value = 0;
	data.v = true;
	data.size = pagesize;
	data.pfn = fr.pfn;
	data.l = locked;
	data.cp = cacheable;
#ifdef CONFIG_VIRT_IDX_DCACHE
	data.cv = cacheable;
#endif /* CONFIG_VIRT_IDX_DCACHE */
	data.p = true;
	data.w = true;
	data.g = false;

	dtlb_data_in_write(data.value);
}

/** Copy PTE to TLB.
 *
 * @param t 		Page Table Entry to be copied.
 * @param index		Zero if lower 8K-subpage, one if higher 8K-subpage.
 * @param ro		If true, the entry will be created read-only, regardless
 * 			of its w field.
 */
void dtlb_pte_copy(pte_t *t, size_t index, bool ro)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = t->page + (index << MMU_PAGE_WIDTH);
	fr.address = t->frame + (index << MMU_PAGE_WIDTH);

	tag.value = 0;
	tag.context = t->as->asid;
	tag.vpn = pg.vpn;

	dtlb_tag_access_write(tag.value);

	data.value = 0;
	data.v = true;
	data.size = PAGESIZE_8K;
	data.pfn = fr.pfn;
	data.l = false;
	data.cp = t->c;
#ifdef CONFIG_VIRT_IDX_DCACHE
	data.cv = t->c;
#endif /* CONFIG_VIRT_IDX_DCACHE */
	data.p = t->k;		/* p like privileged */
	data.w = ro ? false : t->w;
	data.g = t->g;

	dtlb_data_in_write(data.value);
}

/** Copy PTE to ITLB.
 *
 * @param t		Page Table Entry to be copied.
 * @param index		Zero if lower 8K-subpage, one if higher 8K-subpage.
 */
void itlb_pte_copy(pte_t *t, size_t index)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = t->page + (index << MMU_PAGE_WIDTH);
	fr.address = t->frame + (index << MMU_PAGE_WIDTH);

	tag.value = 0;
	tag.context = t->as->asid;
	tag.vpn = pg.vpn;

	itlb_tag_access_write(tag.value);

	data.value = 0;
	data.v = true;
	data.size = PAGESIZE_8K;
	data.pfn = fr.pfn;
	data.l = false;
	data.cp = t->c;
	data.p = t->k;		/* p like privileged */
	data.w = false;
	data.g = t->g;

	itlb_data_in_write(data.value);
}

/** ITLB miss handler. */
void fast_instruction_access_mmu_miss(unsigned int tt, istate_t *istate)
{
	size_t index = (istate->tpc >> MMU_PAGE_WIDTH) % MMU_PAGES_PER_PAGE;
	pte_t t;

	bool found = page_mapping_find(AS, istate->tpc, true, &t);
	if (found && PTE_EXECUTABLE(&t)) {
		assert(t.p);

		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into ITLB.
		 */
		t.a = true;
		itlb_pte_copy(&t, index);
#ifdef CONFIG_TSB
		itsb_pte_copy(&t, index);
#endif
		page_mapping_update(AS, istate->tpc, true, &t);
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */
		as_page_fault(istate->tpc, PF_ACCESS_EXEC, istate);
	}
}

/** DTLB miss handler.
 *
 * Note that some faults (e.g. kernel faults) were already resolved by the
 * low-level, assembly language part of the fast_data_access_mmu_miss handler.
 *
 * @param tt		Trap type.
 * @param istate	Interrupted state saved on the stack.
 */
void fast_data_access_mmu_miss(unsigned int tt, istate_t *istate)
{
	tlb_tag_access_reg_t tag;
	uintptr_t page_8k;
	uintptr_t page_16k;
	size_t index;
	pte_t t;
	as_t *as = AS;

	tag.value = istate->tlb_tag_access;
	page_8k = (uint64_t) tag.vpn << MMU_PAGE_WIDTH;
	page_16k = ALIGN_DOWN(page_8k, PAGE_SIZE);
	index = tag.vpn % MMU_PAGES_PER_PAGE;

	if (tag.context == ASID_KERNEL) {
		if (!tag.vpn) {
			/* NULL access in kernel */
			panic("NULL pointer dereference.");
		} else if (page_8k >= end_of_identity) {
			/* Kernel non-identity. */
			as = AS_KERNEL;
		} else {
			panic("Unexpected kernel page fault.");
		}
	}

	bool found = page_mapping_find(as, page_16k, true, &t);
	if (found) {
		assert(t.p);

		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into DTLB.
		 */
		t.a = true;
		dtlb_pte_copy(&t, index, true);
#ifdef CONFIG_TSB
		dtsb_pte_copy(&t, index, true);
#endif
		page_mapping_update(as, page_16k, true, &t);
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */
		as_page_fault(page_16k, PF_ACCESS_READ, istate);
	}
}

/** DTLB protection fault handler.
 *
 * @param tt		Trap type.
 * @param istate	Interrupted state saved on the stack.
 */
void fast_data_access_protection(unsigned int tt, istate_t *istate)
{
	tlb_tag_access_reg_t tag;
	uintptr_t page_16k;
	size_t index;
	pte_t t;
	as_t *as = AS;

	tag.value = istate->tlb_tag_access;
	page_16k = ALIGN_DOWN((uint64_t) tag.vpn << MMU_PAGE_WIDTH, PAGE_SIZE);
	index = tag.vpn % MMU_PAGES_PER_PAGE;	/* 16K-page emulation */

	if (tag.context == ASID_KERNEL)
		as = AS_KERNEL;

	bool found = page_mapping_find(as, page_16k, true, &t);
	if (found && PTE_WRITABLE(&t)) {
		assert(t.p);

		/*
		 * The mapping was found in the software page hash table and is
		 * writable. Demap the old mapping and insert an updated mapping
		 * into DTLB.
		 */
		t.a = true;
		t.d = true;
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_SECONDARY,
		    page_16k + index * MMU_PAGE_SIZE);
		dtlb_pte_copy(&t, index, false);
#ifdef CONFIG_TSB
		dtsb_pte_copy(&t, index, false);
#endif
		page_mapping_update(as, page_16k, true, &t);
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */
		as_page_fault(page_16k, PF_ACCESS_WRITE, istate);
	}
}

/** Print TLB entry (for debugging purposes).
 *
 * The diag field has been left out in order to make this function more generic
 * (there is no diag field in US3 architeture).
 *
 * @param i		TLB entry number
 * @param t		TLB entry tag
 * @param d		TLB entry data
 */
static void print_tlb_entry(int i, tlb_tag_read_reg_t t, tlb_data_t d)
{
	printf("%u: vpn=%#" PRIx64 ", context=%u, v=%u, size=%u, nfo=%u, "
	    "ie=%u, soft2=%#x, pfn=%#x, soft=%#x, l=%u, "
	    "cp=%u, cv=%u, e=%u, p=%u, w=%u, g=%u\n", i, (uint64_t) t.vpn,
	    t.context, d.v, d.size, d.nfo, d.ie, d.soft2,
	    d.pfn, d.soft, d.l, d.cp, d.cv, d.e, d.p, d.w, d.g);
}

#if defined (US)

/** Print contents of both TLBs. */
void tlb_print(void)
{
	int i;
	tlb_data_t d;
	tlb_tag_read_reg_t t;

	printf("I-TLB contents:\n");
	for (i = 0; i < ITLB_ENTRY_COUNT; i++) {
		d.value = itlb_data_access_read(i);
		t.value = itlb_tag_read_read(i);
		print_tlb_entry(i, t, d);
	}

	printf("D-TLB contents:\n");
	for (i = 0; i < DTLB_ENTRY_COUNT; i++) {
		d.value = dtlb_data_access_read(i);
		t.value = dtlb_tag_read_read(i);
		print_tlb_entry(i, t, d);
	}
}

#elif defined (US3)

/** Print contents of all TLBs. */
void tlb_print(void)
{
	int i;
	tlb_data_t d;
	tlb_tag_read_reg_t t;

	printf("TLB_ISMALL contents:\n");
	for (i = 0; i < tlb_ismall_size(); i++) {
		d.value = dtlb_data_access_read(TLB_ISMALL, i);
		t.value = dtlb_tag_read_read(TLB_ISMALL, i);
		print_tlb_entry(i, t, d);
	}

	printf("TLB_IBIG contents:\n");
	for (i = 0; i < tlb_ibig_size(); i++) {
		d.value = dtlb_data_access_read(TLB_IBIG, i);
		t.value = dtlb_tag_read_read(TLB_IBIG, i);
		print_tlb_entry(i, t, d);
	}

	printf("TLB_DSMALL contents:\n");
	for (i = 0; i < tlb_dsmall_size(); i++) {
		d.value = dtlb_data_access_read(TLB_DSMALL, i);
		t.value = dtlb_tag_read_read(TLB_DSMALL, i);
		print_tlb_entry(i, t, d);
	}

	printf("TLB_DBIG_1 contents:\n");
	for (i = 0; i < tlb_dbig_size(); i++) {
		d.value = dtlb_data_access_read(TLB_DBIG_0, i);
		t.value = dtlb_tag_read_read(TLB_DBIG_0, i);
		print_tlb_entry(i, t, d);
	}

	printf("TLB_DBIG_2 contents:\n");
	for (i = 0; i < tlb_dbig_size(); i++) {
		d.value = dtlb_data_access_read(TLB_DBIG_1, i);
		t.value = dtlb_tag_read_read(TLB_DBIG_1, i);
		print_tlb_entry(i, t, d);
	}
}

#endif

void describe_dmmu_fault(void)
{
	tlb_sfsr_reg_t sfsr;
	uintptr_t sfar;

	sfsr.value = dtlb_sfsr_read();
	sfar = dtlb_sfar_read();

#if defined (US)
	printf("DTLB SFSR: asi=%#x, ft=%#x, e=%d, ct=%d, pr=%d, w=%d, ow=%d, "
	    "fv=%d\n", sfsr.asi, sfsr.ft, sfsr.e, sfsr.ct, sfsr.pr, sfsr.w,
	    sfsr.ow, sfsr.fv);
#elif defined (US3)
	printf("DTLB SFSR: nf=%d, asi=%#x, tm=%d, ft=%#x, e=%d, ct=%d, pr=%d, "
	    "w=%d, ow=%d, fv=%d\n", sfsr.nf, sfsr.asi, sfsr.tm, sfsr.ft,
	    sfsr.e, sfsr.ct, sfsr.pr, sfsr.w, sfsr.ow, sfsr.fv);
#endif

	printf("DTLB SFAR: address=%p\n", (void *) sfar);

	dtlb_sfsr_write(0);
}

void dump_sfsr_and_sfar(void)
{
	tlb_sfsr_reg_t sfsr;
	uintptr_t sfar;

	sfsr.value = dtlb_sfsr_read();
	sfar = dtlb_sfar_read();

#if defined (US)
	printf("DTLB SFSR: asi=%#x, ft=%#x, e=%d, ct=%d, pr=%d, w=%d, ow=%d, "
	    "fv=%d\n", sfsr.asi, sfsr.ft, sfsr.e, sfsr.ct, sfsr.pr, sfsr.w,
	    sfsr.ow, sfsr.fv);
#elif defined (US3)
	printf("DTLB SFSR: nf=%d, asi=%#x, tm=%d, ft=%#x, e=%d, ct=%d, pr=%d, "
	    "w=%d, ow=%d, fv=%d\n", sfsr.nf, sfsr.asi, sfsr.tm, sfsr.ft,
	    sfsr.e, sfsr.ct, sfsr.pr, sfsr.w, sfsr.ow, sfsr.fv);
#endif

	printf("DTLB SFAR: address=%p\n", (void *) sfar);

	dtlb_sfsr_write(0);
}

#if defined (US)
/** Invalidate all unlocked ITLB and DTLB entries. */
void tlb_invalidate_all(void)
{
	int i;

	/*
	 * Walk all ITLB and DTLB entries and remove all unlocked mappings.
	 *
	 * The kernel doesn't use global mappings so any locked global mappings
	 * found must have been created by someone else. Their only purpose now
	 * is to collide with proper mappings. Invalidate immediately. It should
	 * be safe to invalidate them as late as now.
	 */

	tlb_data_t d;
	tlb_tag_read_reg_t t;

	for (i = 0; i < ITLB_ENTRY_COUNT; i++) {
		d.value = itlb_data_access_read(i);
		if (!d.l || d.g) {
			t.value = itlb_tag_read_read(i);
			d.v = false;
			itlb_tag_access_write(t.value);
			itlb_data_access_write(i, d.value);
		}
	}

	for (i = 0; i < DTLB_ENTRY_COUNT; i++) {
		d.value = dtlb_data_access_read(i);
		if (!d.l || d.g) {
			t.value = dtlb_tag_read_read(i);
			d.v = false;
			dtlb_tag_access_write(t.value);
			dtlb_data_access_write(i, d.value);
		}
	}

}

#elif defined (US3)

/** Invalidate all unlocked ITLB and DTLB entries. */
void tlb_invalidate_all(void)
{
	itlb_demap(TLB_DEMAP_ALL, 0, 0);
	dtlb_demap(TLB_DEMAP_ALL, 0, 0);
}

#endif

/** Invalidate all ITLB and DTLB entries that belong to specified ASID
 * (Context).
 *
 * @param asid Address Space ID.
 */
void tlb_invalidate_asid(asid_t asid)
{
	tlb_context_reg_t pc_save, ctx;

	/* switch to nucleus because we are mapped by the primary context */
	nucleus_enter();

	ctx.v = pc_save.v = mmu_primary_context_read();
	ctx.context = asid;
	mmu_primary_context_write(ctx.v);

	itlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_PRIMARY, 0);
	dtlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_PRIMARY, 0);

	mmu_primary_context_write(pc_save.v);

	nucleus_leave();
}

/** Invalidate all ITLB and DTLB entries for specified page range in specified
 * address space.
 *
 * @param asid		Address Space ID.
 * @param page		First page which to sweep out from ITLB and DTLB.
 * @param cnt		Number of ITLB and DTLB entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	unsigned int i;
	tlb_context_reg_t pc_save, ctx;

	/* switch to nucleus because we are mapped by the primary context */
	nucleus_enter();

	ctx.v = pc_save.v = mmu_primary_context_read();
	ctx.context = asid;
	mmu_primary_context_write(ctx.v);

	for (i = 0; i < cnt * MMU_PAGES_PER_PAGE; i++) {
		itlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_PRIMARY,
		    page + i * MMU_PAGE_SIZE);
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_PRIMARY,
		    page + i * MMU_PAGE_SIZE);
	}

	mmu_primary_context_write(pc_save.v);

	nucleus_leave();
}

/** @}
 */
