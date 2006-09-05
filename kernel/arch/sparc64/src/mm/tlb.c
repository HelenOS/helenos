/*
 * Copyright (C) 2005 Jakub Jermar
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
#include <arch.h>
#include <print.h>
#include <arch/types.h>
#include <typedefs.h>
#include <config.h>
#include <arch/trap/trap.h>
#include <panic.h>
#include <arch/asm.h>
#include <symtab.h>

static void dtlb_pte_copy(pte_t *t, bool ro);
static void itlb_pte_copy(pte_t *t);
static void do_fast_instruction_access_mmu_miss_fault(istate_t *istate, const char *str);
static void do_fast_data_access_mmu_miss_fault(istate_t *istate, tlb_tag_access_reg_t tag, const char *str);
static void do_fast_data_access_protection_fault(istate_t *istate, tlb_tag_access_reg_t tag, const char *str);

char *context_encoding[] = {
	"Primary",
	"Secondary",
	"Nucleus",
	"Reserved"
};

void tlb_arch_init(void)
{
	/*
	 * TLBs are actually initialized early
	 * in start.S.
	 */
}

/** Insert privileged mapping into DMMU TLB.
 *
 * @param page Virtual page address.
 * @param frame Physical frame address.
 * @param pagesize Page size.
 * @param locked True for permanent mappings, false otherwise.
 * @param cacheable True if the mapping is cacheable, false otherwise.
 */
void dtlb_insert_mapping(uintptr_t page, uintptr_t frame, int pagesize, bool locked, bool cacheable)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = page;
	fr.address = frame;

	tag.value = ASID_KERNEL;
	tag.vpn = pg.vpn;

	dtlb_tag_access_write(tag.value);

	data.value = 0;
	data.v = true;
	data.size = pagesize;
	data.pfn = fr.pfn;
	data.l = locked;
	data.cp = cacheable;
	data.cv = cacheable;
	data.p = true;
	data.w = true;
	data.g = false;

	dtlb_data_in_write(data.value);
}

/** Copy PTE to TLB.
 *
 * @param t Page Table Entry to be copied.
 * @param ro If true, the entry will be created read-only, regardless of its w field.
 */
void dtlb_pte_copy(pte_t *t, bool ro)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = t->page;
	fr.address = t->frame;

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
	data.cv = t->c;
	data.p = t->k;		/* p like privileged */
	data.w = ro ? false : t->w;
	data.g = t->g;
	
	dtlb_data_in_write(data.value);
}

void itlb_pte_copy(pte_t *t)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	page_address_t pg;
	frame_address_t fr;

	pg.address = t->page;
	fr.address = t->frame;

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
	data.cv = t->c;
	data.p = t->k;		/* p like privileged */
	data.w = false;
	data.g = t->g;
	
	itlb_data_in_write(data.value);
}

/** ITLB miss handler. */
void fast_instruction_access_mmu_miss(int n, istate_t *istate)
{
	uintptr_t va = ALIGN_DOWN(istate->tpc, PAGE_SIZE);
	pte_t *t;

	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	if (t && PTE_EXECUTABLE(t)) {
		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into ITLB.
		 */
		t->a = true;
		itlb_pte_copy(t);
		page_table_unlock(AS, true);
	} else {
		/*
		 * Forward the page fault to the address space page fault handler.
		 */		
		page_table_unlock(AS, true);
		if (as_page_fault(va, PF_ACCESS_EXEC, istate) == AS_PF_FAULT) {
			do_fast_instruction_access_mmu_miss_fault(istate, __FUNCTION__);
		}
	}
}

/** DTLB miss handler.
 *
 * Note that some faults (e.g. kernel faults) were already resolved
 * by the low-level, assembly language part of the fast_data_access_mmu_miss
 * handler.
 */
void fast_data_access_mmu_miss(int n, istate_t *istate)
{
	tlb_tag_access_reg_t tag;
	uintptr_t va;
	pte_t *t;

	tag.value = dtlb_tag_access_read();
	va = tag.vpn << PAGE_WIDTH;

	if (tag.context == ASID_KERNEL) {
		if (!tag.vpn) {
			/* NULL access in kernel */
			do_fast_data_access_mmu_miss_fault(istate, tag, __FUNCTION__);
		}
		do_fast_data_access_mmu_miss_fault(istate, tag, "Unexpected kernel page fault.");
	}

	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	if (t) {
		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into DTLB.
		 */
		t->a = true;
		dtlb_pte_copy(t, true);
		page_table_unlock(AS, true);
	} else {
		/*
		 * Forward the page fault to the address space page fault handler.
		 */		
		page_table_unlock(AS, true);
		if (as_page_fault(va, PF_ACCESS_READ, istate) == AS_PF_FAULT) {
			do_fast_data_access_mmu_miss_fault(istate, tag, __FUNCTION__);
		}
	}
}

/** DTLB protection fault handler. */
void fast_data_access_protection(int n, istate_t *istate)
{
	tlb_tag_access_reg_t tag;
	uintptr_t va;
	pte_t *t;

	tag.value = dtlb_tag_access_read();
	va = tag.vpn << PAGE_WIDTH;

	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	if (t && PTE_WRITABLE(t)) {
		/*
		 * The mapping was found in the software page hash table and is writable.
		 * Demap the old mapping and insert an updated mapping into DTLB.
		 */
		t->a = true;
		t->d = true;
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_SECONDARY, va);
		dtlb_pte_copy(t, false);
		page_table_unlock(AS, true);
	} else {
		/*
		 * Forward the page fault to the address space page fault handler.
		 */		
		page_table_unlock(AS, true);
		if (as_page_fault(va, PF_ACCESS_WRITE, istate) == AS_PF_FAULT) {
			do_fast_data_access_protection_fault(istate, tag, __FUNCTION__);
		}
	}
}

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
		
		printf("%d: vpn=%#llx, context=%d, v=%d, size=%d, nfo=%d, ie=%d, soft2=%#x, diag=%#x, pfn=%#x, soft=%#x, l=%d, cp=%d, cv=%d, e=%d, p=%d, w=%d, g=%d\n",
			i, t.vpn, t.context, d.v, d.size, d.nfo, d.ie, d.soft2, d.diag, d.pfn, d.soft, d.l, d.cp, d.cv, d.e, d.p, d.w, d.g);
	}

	printf("D-TLB contents:\n");
	for (i = 0; i < DTLB_ENTRY_COUNT; i++) {
		d.value = dtlb_data_access_read(i);
		t.value = dtlb_tag_read_read(i);
		
		printf("%d: vpn=%#llx, context=%d, v=%d, size=%d, nfo=%d, ie=%d, soft2=%#x, diag=%#x, pfn=%#x, soft=%#x, l=%d, cp=%d, cv=%d, e=%d, p=%d, w=%d, g=%d\n",
			i, t.vpn, t.context, d.v, d.size, d.nfo, d.ie, d.soft2, d.diag, d.pfn, d.soft, d.l, d.cp, d.cv, d.e, d.p, d.w, d.g);
	}

}

void do_fast_instruction_access_mmu_miss_fault(istate_t *istate, const char *str)
{
	char *tpc_str = get_symtab_entry(istate->tpc);

	printf("TPC=%p, (%s)\n", istate->tpc, tpc_str);
	panic("%s\n", str);
}

void do_fast_data_access_mmu_miss_fault(istate_t *istate, tlb_tag_access_reg_t tag, const char *str)
{
	uintptr_t va;
	char *tpc_str = get_symtab_entry(istate->tpc);

	va = tag.vpn << PAGE_WIDTH;

	printf("Faulting page: %p, ASID=%d\n", va, tag.context);
	printf("TPC=%p, (%s)\n", istate->tpc, tpc_str);
	panic("%s\n", str);
}

void do_fast_data_access_protection_fault(istate_t *istate, tlb_tag_access_reg_t tag, const char *str)
{
	uintptr_t va;
	char *tpc_str = get_symtab_entry(istate->tpc);

	va = tag.vpn << PAGE_WIDTH;

	printf("Faulting page: %p, ASID=%d\n", va, tag.context);
	printf("TPC=%p, (%s)\n", istate->tpc, tpc_str);
	panic("%s\n", str);
}

/** Invalidate all unlocked ITLB and DTLB entries. */
void tlb_invalidate_all(void)
{
	int i;
	tlb_data_t d;
	tlb_tag_read_reg_t t;

	for (i = 0; i < ITLB_ENTRY_COUNT; i++) {
		d.value = itlb_data_access_read(i);
		if (!d.l) {
			t.value = itlb_tag_read_read(i);
			d.v = false;
			itlb_tag_access_write(t.value);
			itlb_data_access_write(i, d.value);
		}
	}
	
	for (i = 0; i < DTLB_ENTRY_COUNT; i++) {
		d.value = dtlb_data_access_read(i);
		if (!d.l) {
			t.value = dtlb_tag_read_read(i);
			d.v = false;
			dtlb_tag_access_write(t.value);
			dtlb_data_access_write(i, d.value);
		}
	}
	
}

/** Invalidate all ITLB and DTLB entries that belong to specified ASID (Context).
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

/** Invalidate all ITLB and DTLB entries for specified page range in specified address space.
 *
 * @param asid Address Space ID.
 * @param page First page which to sweep out from ITLB and DTLB.
 * @param cnt Number of ITLB and DTLB entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, count_t cnt)
{
	int i;
	tlb_context_reg_t pc_save, ctx;
	
	/* switch to nucleus because we are mapped by the primary context */
	nucleus_enter();
	
	ctx.v = pc_save.v = mmu_primary_context_read();
	ctx.context = asid;
	mmu_primary_context_write(ctx.v);
	
	for (i = 0; i < cnt; i++) {
		itlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_PRIMARY, page + i * PAGE_SIZE);
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_PRIMARY, page + i * PAGE_SIZE);
	}
	
	mmu_primary_context_write(pc_save.v);
	
	nucleus_leave();
}

/** @}
 */
