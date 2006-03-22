/*
 * Copyright (C) 2006 Jakub Jermar
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

/*
 * TLB management.
 */

#include <mm/tlb.h>
#include <mm/asid.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <arch/barrier.h>
#include <arch/interrupt.h>
#include <arch/pal/pal.h>
#include <arch/asm.h>
#include <typedefs.h>
#include <panic.h>
#include <print.h>
#include <arch.h>

/** Invalidate all TLB entries. */
void tlb_invalidate_all(void)
{
		ipl_t ipl;
		__address adr;
		__u32 count1, count2, stride1, stride2;
		
		int i,j;
		
		adr = PAL_PTCE_INFO_BASE();
		count1 = PAL_PTCE_INFO_COUNT1();
		count2 = PAL_PTCE_INFO_COUNT2();
		stride1 = PAL_PTCE_INFO_STRIDE1();
		stride2 = PAL_PTCE_INFO_STRIDE2();
		
		ipl = interrupts_disable();

		for(i = 0; i < count1; i++) {
			for(j = 0; j < count2; j++) {
				__asm__ volatile (
					"ptc.e %0 ;;"
					:
					: "r" (adr)
				);
				adr += stride2;
			}
			adr += stride1;
		}

		interrupts_restore(ipl);

		srlz_d();
		srlz_i();
}

/** Invalidate entries belonging to an address space.
 *
 * @param asid Address space identifier.
 */
void tlb_invalidate_asid(asid_t asid)
{
	tlb_invalidate_all();
}


void tlb_invalidate_pages(asid_t asid, __address page, count_t cnt)
{
	region_register rr;
	bool restore_rr = false;
	int b = 0;
	int c = cnt;

	__address va;
	va = page;

	rr.word = rr_read(VA2VRN(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(va));
		rr_write(VA2VRN(va), rr0.word);
		srlz_d();
		srlz_i();
	}
	
	while(c >>= 1)
		b++;
	b >>= 1;
	__u64 ps;
	
	switch (b) {
		case 0: /*cnt 1-3*/
			ps = PAGE_WIDTH;
			break;
		case 1: /*cnt 4-15*/
			/*cnt=((cnt-1)/4)+1;*/
			ps = PAGE_WIDTH+2;
			va &= ~((1<<ps)-1);
			break;
		case 2: /*cnt 16-63*/
			/*cnt=((cnt-1)/16)+1;*/
			ps = PAGE_WIDTH+4;
			va &= ~((1<<ps)-1);
			break;
		case 3: /*cnt 64-255*/
			/*cnt=((cnt-1)/64)+1;*/
			ps = PAGE_WIDTH+6;
			va &= ~((1<<ps)-1);
			break;
		case 4: /*cnt 256-1023*/
			/*cnt=((cnt-1)/256)+1;*/
			ps = PAGE_WIDTH+8;
			va &= ~((1<<ps)-1);
			break;
		case 5: /*cnt 1024-4095*/
			/*cnt=((cnt-1)/1024)+1;*/
			ps = PAGE_WIDTH+10;
			va &= ~((1<<ps)-1);
			break;
		case 6: /*cnt 4096-16383*/
			/*cnt=((cnt-1)/4096)+1;*/
			ps = PAGE_WIDTH+12;
			va &= ~((1<<ps)-1);
			break;
		case 7: /*cnt 16384-65535*/
		case 8: /*cnt 65536-(256K-1)*/
			/*cnt=((cnt-1)/16384)+1;*/
			ps = PAGE_WIDTH+14;
			va &= ~((1<<ps)-1);
			break;
		default:
			/*cnt=((cnt-1)/(16384*16))+1;*/
			ps=PAGE_WIDTH+18;
			va&=~((1<<ps)-1);
			break;
	}
	/*cnt+=(page!=va);*/
	for(; va<(page+cnt*(PAGE_SIZE)); va += (1<<ps))	{
		__asm__ volatile (
			"ptc.l %0,%1;;"
			:
			: "r" (va), "r" (ps<<2)
		);
	}
	srlz_d();
	srlz_i();
	
	if (restore_rr) {
		rr_write(VA2VRN(va), rr.word);
		srlz_d();
		srlz_i();
	}
}


/** Insert data into data translation cache.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 */
void dtc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry)
{
	tc_mapping_insert(va, asid, entry, true);
}

/** Insert data into instruction translation cache.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 */
void itc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry)
{
	tc_mapping_insert(va, asid, entry, false);
}

/** Insert data into instruction or data translation cache.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 * @param dtc If true, insert into data translation cache, use instruction translation cache otherwise.
 */
void tc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, bool dtc)
{
	region_register rr;
	bool restore_rr = false;

	rr.word = rr_read(VA2VRN(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(va));
		rr_write(VA2VRN(va), rr0.word);
		srlz_d();
		srlz_i();
	}
	
	__asm__ volatile (
		"mov r8=psr;;\n"
		"rsm %0;;\n"   			/* PSR_IC_MASK */
		"srlz.d;;\n"
		"srlz.i;;\n"
		"mov cr.ifa=%1\n"		/* va */
		"mov cr.itir=%2;;\n"		/* entry.word[1] */
		"cmp.eq p6,p7 = %4,r0;;\n"	/* decide between itc and dtc */ 
		"(p6) itc.i %3;;\n"
		"(p7) itc.d %3;;\n"
		"mov psr.l=r8;;\n"
		"srlz.d;;\n"
		:
		: "i" (PSR_IC_MASK), "r" (va), "r" (entry.word[1]), "r" (entry.word[0]), "r" (dtc)
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
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 * @param tr Translation register.
 */
void itr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, index_t tr)
{
	tr_mapping_insert(va, asid, entry, false, tr);
}

/** Insert data into data translation register.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 * @param tr Translation register.
 */
void dtr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, index_t tr)
{
	tr_mapping_insert(va, asid, entry, true, tr);
}

/** Insert data into instruction or data translation register.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 * @param dtc If true, insert into data translation register, use instruction translation register otherwise.
 * @param tr Translation register.
 */
void tr_mapping_insert(__address va, asid_t asid, tlb_entry_t entry, bool dtr, index_t tr)
{
	region_register rr;
	bool restore_rr = false;

	rr.word = rr_read(VA2VRN(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA2VRN(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA2VRN(va));
		rr_write(VA2VRN(va), rr0.word);
		srlz_d();
		srlz_i();
	}

	__asm__ volatile (
		"mov r8=psr;;\n"
		"rsm %0;;\n"			/* PSR_IC_MASK */
		"srlz.d;;\n"
		"srlz.i;;\n"
		"mov cr.ifa=%1\n"        	/* va */		 
		"mov cr.itir=%2;;\n"		/* entry.word[1] */ 
		"cmp.eq p6,p7=%5,r0;;\n"	/* decide between itr and dtr */
		"(p6) itr.i itr[%4]=%3;;\n"
		"(p7) itr.d dtr[%4]=%3;;\n"
		"mov psr.l=r8;;\n"
		"srlz.d;;\n"
		:
		: "i" (PSR_IC_MASK), "r" (va), "r" (entry.word[1]), "r" (entry.word[0]), "r" (tr), "r" (dtr)
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
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 * @param dtr If true, insert into data translation register, use data translation cache otherwise.
 * @param tr Translation register if dtr is true, ignored otherwise.
 */
void dtlb_kernel_mapping_insert(__address page, __address frame, bool dtr, index_t tr)
{
	tlb_entry_t entry;
	
	entry.word[0] = 0;
	entry.word[1] = 0;
	
	entry.p = true;			/* present */
	entry.ma = MA_WRITEBACK;
	entry.a = true;			/* already accessed */
	entry.d = true;			/* already dirty */
	entry.pl = PL_KERNEL;
	entry.ar = AR_READ | AR_WRITE;
	entry.ppn = frame >> PPN_SHIFT;
	entry.ps = PAGE_WIDTH;
	
	if (dtr)
		dtr_mapping_insert(page, ASID_KERNEL, entry, tr);
	else
		dtc_mapping_insert(page, ASID_KERNEL, entry);
}

/** Copy content of PTE into data translation cache.
 *
 * @param t PTE.
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
}

/** Copy content of PTE into instruction translation cache.
 *
 * @param t PTE.
 */
void itc_pte_copy(pte_t *t)
{
	tlb_entry_t entry;

	entry.word[0] = 0;
	entry.word[1] = 0;
	
	ASSERT(t->x);
	
	entry.p = t->p;
	entry.ma = t->c ? MA_WRITEBACK : MA_UNCACHEABLE;
	entry.a = t->a;
	entry.pl = t->k ? PL_KERNEL : PL_USER;
	entry.ar = t->x ? (AR_EXECUTE | AR_READ) : AR_READ;
	entry.ppn = t->frame >> PPN_SHIFT;
	entry.ps = PAGE_WIDTH;
	
	itc_mapping_insert(t->page, t->as->asid, entry);
}

/** Instruction TLB fault handler for faults with VHPT turned off.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void alternate_instruction_tlb_fault(__u64 vector, istate_t *istate)
{
	region_register rr;
	__address va;
	pte_t *t;
	
	va = istate->cr_ifa;	/* faulting address */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	if (t) {
		/*
		 * The mapping was found in software page hash table.
		 * Insert it into data translation cache.
		 */
		itc_pte_copy(t);
		page_table_unlock(AS, true);
	} else {
		/*
		 * Forward the page fault to address space page fault handler.
		 */
		page_table_unlock(AS, true);
		if (!as_page_fault(va)) {
			panic("%s: va=%P, rid=%d, iip=%P\n", __FUNCTION__, istate->cr_ifa, rr.map.rid, istate->cr_iip);
		}
	}
}

/** Data TLB fault handler for faults with VHPT turned off.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void alternate_data_tlb_fault(__u64 vector, istate_t *istate)
{
	region_register rr;
	rid_t rid;
	__address va;
	pte_t *t;
	
	va = istate->cr_ifa;	/* faulting address */
	rr.word = rr_read(VA2VRN(va));
	rid = rr.map.rid;
	if (RID2ASID(rid) == ASID_KERNEL) {
		if (VA2VRN(va) == VRN_KERNEL) {
			/*
			 * Provide KA2PA(identity) mapping for faulting piece of
			 * kernel address space.
			 */
			dtlb_kernel_mapping_insert(va, KA2PA(va), false, 0);
			return;
		}
	}

	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	if (t) {
		/*
		 * The mapping was found in software page hash table.
		 * Insert it into data translation cache.
		 */
		dtc_pte_copy(t);
		page_table_unlock(AS, true);
	} else {
		/*
		 * Forward the page fault to address space page fault handler.
		 */
		page_table_unlock(AS, true);
		if (!as_page_fault(va)) {
			panic("%s: va=%P, rid=%d, iip=%P\n", __FUNCTION__, va, rid, istate->cr_iip);
		}
	}
}

/** Data nested TLB fault handler.
 *
 * This fault should not occur.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void data_nested_tlb_fault(__u64 vector, istate_t *istate)
{
	panic("%s\n", __FUNCTION__);
}

/** Data Dirty bit fault handler.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void data_dirty_bit_fault(__u64 vector, istate_t *istate)
{
	pte_t *t;

	page_table_lock(AS, true);
	t = page_mapping_find(AS, istate->cr_ifa);
	ASSERT(t && t->p);
	if (t && t->p) {
		/*
		 * Update the Dirty bit in page tables and reinsert
		 * the mapping into DTC.
		 */
		t->d = true;
		dtc_pte_copy(t);
	}
	page_table_unlock(AS, true);
}

/** Instruction access bit fault handler.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void instruction_access_bit_fault(__u64 vector, istate_t *istate)
{
	pte_t *t;

	page_table_lock(AS, true);
	t = page_mapping_find(AS, istate->cr_ifa);
	ASSERT(t && t->p);
	if (t && t->p) {
		/*
		 * Update the Accessed bit in page tables and reinsert
		 * the mapping into ITC.
		 */
		t->a = true;
		itc_pte_copy(t);
	}
	page_table_unlock(AS, true);
}

/** Data access bit fault handler.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void data_access_bit_fault(__u64 vector, istate_t *istate)
{
	pte_t *t;

	page_table_lock(AS, true);
	t = page_mapping_find(AS, istate->cr_ifa);
	ASSERT(t && t->p);
	if (t && t->p) {
		/*
		 * Update the Accessed bit in page tables and reinsert
		 * the mapping into DTC.
		 */
		t->a = true;
		dtc_pte_copy(t);
	}
	page_table_unlock(AS, true);
}

/** Page not present fault handler.
 *
 * @param vector Interruption vector.
 * @param istate Structure with saved interruption state.
 */
void page_not_present(__u64 vector, istate_t *istate)
{
	region_register rr;
	__address va;
	pte_t *t;
	
	va = istate->cr_ifa;	/* faulting address */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, va);
	ASSERT(t);
	
	if (t->p) {
		/*
		 * If the Present bit is set in page hash table, just copy it
		 * and update ITC/DTC.
		 */
		if (t->x)
			itc_pte_copy(t);
		else
			dtc_pte_copy(t);
		page_table_unlock(AS, true);
	} else {
		page_table_unlock(AS, true);
		if (!as_page_fault(va)) {
			panic("%s: va=%P, rid=%d\n", __FUNCTION__, va, rr.map.rid);
		}
	}
}
