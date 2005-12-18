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
#include <print.h>
#include <arch/types.h>
#include <typedefs.h>
#include <config.h>
#include <arch/trap/trap.h>

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
	 * For simplicity, we do identity mapping of first 4M of memory.
	 * The very next change should be leaving the first 4M unmapped.
	 */
	tag.value = 0;
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

/** Invalidate all ITLB and DLTB entries for specified page in specified address space.
 *
 * @param asid Address Space ID.
 * @param page Page which to sweep out from ITLB and DTLB.
 */
void tlb_invalidate_page(asid_t asid, __address page)
{
	/* TODO: write asid to some Context register and encode the register in second parameter below. */
	itlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, page);
	dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, page);
}
