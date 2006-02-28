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

#include <arch/mm/tlb.h>
#include <mm/tlb.h>
#include <arch/mm/frame.h>
#include <arch/mm/page.h>
#include <arch/mm/mmu.h>
#include <mm/asid.h>
#include <print.h>
#include <arch/types.h>
#include <typedefs.h>
#include <config.h>
#include <arch/trap/trap.h>
#include <panic.h>
#include <arch/asm.h>
#include <symtab.h>

#include <arch/drivers/fb.h>
#include <arch/drivers/i8042.h>

char *context_encoding[] = {
	"Primary",
	"Secondary",
	"Nucleus",
	"Reserved"
};

/** Initialize ITLB and DTLB.
 *
 * The goal of this function is to disable MMU
 * so that both TLBs can be purged and new
 * kernel 4M locked entry can be installed.
 * After TLB is initialized, MMU is enabled
 * again.
 *
 * Switching MMU off imposes the requirement for
 * the kernel to run in identity mapped environment.
 */
void tlb_arch_init(void)
{
	tlb_tag_access_reg_t tag;
	tlb_data_t data;
	frame_address_t fr;
	page_address_t pg;

	fr.address = config.base;
	pg.address = config.base;

	immu_disable();
	dmmu_disable();
	
	/*
	 * We do identity mapping of 4M-page at 4M.
	 */
	tag.value = ASID_KERNEL;
	tag.vpn = pg.vpn;

	itlb_tag_access_write(tag.value);
	dtlb_tag_access_write(tag.value);

	data.value = 0;
	data.v = true;
	data.size = PAGESIZE_4M;
	data.pfn = fr.pfn;
	data.l = true;
	data.cp = 1;
	data.cv = 1;
	data.p = true;
	data.w = true;
	data.g = true;

	itlb_data_in_write(data.value);
	dtlb_data_in_write(data.value);

	/*
	 * Register window traps can occur before MMU is enabled again.
	 * This ensures that any such traps will be handled from 
	 * kernel identity mapped trap handler.
	 */
	trap_switch_trap_table();
	
	tlb_invalidate_all();

	dmmu_enable();
	immu_enable();
}

/** Insert privileged mapping into DMMU TLB.
 *
 * @param page Virtual page address.
 * @param frame Physical frame address.
 * @param pagesize Page size.
 * @param locked True for permanent mappings, false otherwise.
 * @param cacheable True if the mapping is cacheable, false otherwise.
 */
void dtlb_insert_mapping(__address page, __address frame, int pagesize, bool locked, bool cacheable)
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
	data.g = true;

	dtlb_data_in_write(data.value);
}

/** ITLB miss handler. */
void fast_instruction_access_mmu_miss(void)
{
	panic("%s\n", __FUNCTION__);
}

/** DTLB miss handler. */
void fast_data_access_mmu_miss(void)
{
	tlb_tag_access_reg_t tag;
	__address tpc;
	char *tpc_str;

	tag.value = dtlb_tag_access_read();
	if (tag.context != ASID_KERNEL || tag.vpn == 0) {
		tpc = tpc_read();
		tpc_str = get_symtab_entry(tpc);

		printf("Faulting page: %P, ASID=%d\n", tag.vpn * PAGE_SIZE, tag.context);
		printf("TPC=%P, (%s)\n", tpc, tpc_str ? tpc_str : "?");
		panic("%s\n", __FUNCTION__);
	}

	/*
	 * Identity map piece of faulting kernel address space.
	 */
	dtlb_insert_mapping(tag.vpn * PAGE_SIZE, tag.vpn * FRAME_SIZE, PAGESIZE_8K, false, true);
}

/** DTLB protection fault handler. */
void fast_data_access_protection(void)
{
	panic("%s\n", __FUNCTION__);
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
		
		printf("%d: vpn=%Q, context=%d, v=%d, size=%d, nfo=%d, ie=%d, soft2=%X, diag=%X, pfn=%X, soft=%X, l=%d, cp=%d, cv=%d, e=%d, p=%d, w=%d, g=%d\n",
			i, t.vpn, t.context, d.v, d.size, d.nfo, d.ie, d.soft2, d.diag, d.pfn, d.soft, d.l, d.cp, d.cv, d.e, d.p, d.w, d.g);
	}

	printf("D-TLB contents:\n");
	for (i = 0; i < DTLB_ENTRY_COUNT; i++) {
		d.value = dtlb_data_access_read(i);
		t.value = dtlb_tag_read_read(i);
		
		printf("%d: vpn=%Q, context=%d, v=%d, size=%d, nfo=%d, ie=%d, soft2=%X, diag=%X, pfn=%X, soft=%X, l=%d, cp=%d, cv=%d, e=%d, p=%d, w=%d, g=%d\n",
			i, t.vpn, t.context, d.v, d.size, d.nfo, d.ie, d.soft2, d.diag, d.pfn, d.soft, d.l, d.cp, d.cv, d.e, d.p, d.w, d.g);
	}

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
	/* TODO: write asid to some Context register and encode the register in second parameter below. */
	itlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_NUCLEUS, 0);
	dtlb_demap(TLB_DEMAP_CONTEXT, TLB_DEMAP_NUCLEUS, 0);
}

/** Invalidate all ITLB and DTLB entries for specified page range in specified address space.
 *
 * @param asid Address Space ID.
 * @param page First page which to sweep out from ITLB and DTLB.
 * @param cnt Number of ITLB and DTLB entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, __address page, count_t cnt)
{
	int i;
	
	for (i = 0; i < cnt; i++) {
		/* TODO: write asid to some Context register and encode the register in second parameter below. */
		itlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, page + i * PAGE_SIZE);
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, page + i * PAGE_SIZE);
	}
}
