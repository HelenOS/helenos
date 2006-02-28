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
#include <arch/mm/tlb.h>
#include <arch/barrier.h>
#include <arch/interrupt.h>
#include <typedefs.h>
#include <panic.h>

/** Invalidate all TLB entries. */
void tlb_invalidate_all(void)
{
	/* TODO */
}

/** Invalidate entries belonging to an address space.
 *
 * @param asid Address space identifier.
 */
void tlb_invalidate_asid(asid_t asid)
{
	/* TODO */
}

/** Insert data into data translation cache.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 */
void dtc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry) {
	tc_mapping_insert(va, asid, entry, true);
}

/** Insert data into instruction translation cache.
 *
 * @param va Virtual page address.
 * @param asid Address space identifier.
 * @param entry The rest of TLB entry as required by TLB insertion format.
 */
void itc_mapping_insert(__address va, asid_t asid, tlb_entry_t entry) {
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

	if (!(entry.not_present.p))
		return;

	rr.word = rr_read(VA_REGION(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA_REGION(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA_REGION(va));
		rr_write(VA_REGION(va), rr0.word);
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
		rr_write(VA_REGION(va),rr.word);
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

	if (!(entry.not_present.p))
		return;

	rr.word = rr_read(VA_REGION(va));
	if ((restore_rr = (rr.map.rid != ASID2RID(asid, VA_REGION(va))))) {
		/*
		 * The selected region register does not contain required RID.
		 * Save the old content of the register and replace the RID.
		 */
		region_register rr0;

		rr0 = rr;
		rr0.map.rid = ASID2RID(asid, VA_REGION(va));
		rr_write(VA_REGION(va), rr0.word);
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
		rr_write(VA_REGION(va),rr.word);
		srlz_d();
		srlz_i();
	}
}

void alternate_instruction_tlb_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}

void alternate_data_tlb_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s: %P\n", __FUNCTION__, pstate->cr_ifa);
}

void data_nested_tlb_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}

void data_dirty_bit_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}

void instruction_access_bit_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}

void data_access_bit_fault(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}

void page_not_present(__u64 vector, struct exception_regdump *pstate)
{
	panic("%s\n", __FUNCTION__);
}
