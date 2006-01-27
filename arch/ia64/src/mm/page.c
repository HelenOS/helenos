/*
 * Copyright (C) 2006 Jakub Jermar
 * Copyright (C) 2006 Jakub Vana
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

#include <arch/mm/page.h>
#include <genarch/mm/page_ht.h>
#include <mm/asid.h>
#include <arch/types.h>
#include <print.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <config.h>
#include <panic.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <memstr.h>

/** Initialize VHPT and region registers. */
static void set_vhpt_environment(void)
{
	region_register rr;
	pta_register pta;	
	int i;
	
	/*
	 * First set up kernel region register.
	 */
	rr.word = rr_read(VRN_KERNEL);
	rr.map.ve = 0;                  /* disable VHPT walker */
	rr.map.ps = PAGE_WIDTH;
	rr.map.rid = ASID_KERNEL;
	rr_write(VRN_KERNEL, rr.word);
	srlz_i();
	srlz_d();
	
	/*
	 * And invalidate the rest of region register.
	 */
	for(i = 0; i < REGION_REGISTERS; i++) {
		/* skip kernel rr */
		if (i == VRN_KERNEL)
			continue;
	
		rr.word == rr_read(i);
		rr.map.ve = 0;		/* disable VHPT walker */
		rr.map.rid = ASID_INVALID;
		rr_write(i, rr.word);
		srlz_i();
		srlz_d();
	}

	/*
	 * Allocate VHPT and invalidate all its entries.
	 */
	page_ht = (pte_t *) frame_alloc(FRAME_KA, VHPT_WIDTH - FRAME_WIDTH, NULL);
	memsetb((__address) page_ht, VHPT_SIZE, 0);
	ht_invalidate_all();	
	
	/*
	 * Set up PTA register.
	 */
	pta.word = pta_read();
	pta.map.ve = 0;                   /* disable VHPT walker */
	pta.map.vf = 1;                   /* large entry format */
	pta.map.size = VHPT_WIDTH;
	pta.map.base = (__address) page_ht;
	pta_write(pta.word);
	srlz_i();
	srlz_d();
}

/** Initialize ia64 virtual address translation subsystem. */
void page_arch_init(void)
{
	page_operations = &page_ht_operations;
	pk_disable();
	set_vhpt_environment();
}

/** Calculate address of collision chain from VPN and ASID.
 *
 * This is rather non-trivial function.
 * First, it has to translate ASID to RID.
 * This is achieved by taking VRN bits of
 * page into account.
 * Second, it must preserve the region register
 * it writes the RID to.
 *
 * @param page Address of virtual page including VRN bits.
 * @param asid Address space identifier.
 *
 * @return Head of VHPT collision chain for page and asid.
 */
pte_t *vhpt_hash(__address page, asid_t asid)
{
	region_register rr_save, rr;
	pte_t *t;

	rr_save.word = rr_read(VRN_WORK);
	rr.word = rr_save.word;
	if ((page >> VRN_SHIFT) != VRN_KERNEL)
		rr.map.rid = (asid * RIDS_PER_ASID) + (page >> VRN_SHIFT);
	else
		rr.map.rid = ASID_KERNEL;
	rr_write(VRN_WORK, rr.word);
	srlz_i();
	t = (pte_t *) thash((VRN_WORK << VRN_SHIFT) | (~(VRN_MASK) & page));
	rr_write(VRN_WORK, rr_save.word);
	srlz_i();
	srlz_d();

	return t;
}
