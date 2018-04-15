/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup ia64mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <assert.h>
#include <genarch/mm/page_ht.h>
#include <mm/asid.h>
#include <arch/mm/asid.h>
#include <arch/mm/vhpt.h>
#include <typedefs.h>
#include <print.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <config.h>
#include <panic.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <align.h>

static void set_environment(void);

/** Initialize ia64 virtual address translation subsystem. */
void page_arch_init(void)
{
	page_mapping_operations = &ht_mapping_operations;
	pk_disable();
	set_environment();
}

/** Initialize VHPT and region registers. */
void set_environment(void)
{
	region_register_t rr;
	pta_register_t pta;
	int i;
#ifdef CONFIG_VHPT
	uintptr_t vhpt_base;
#endif

	/*
	 * Set up kernel region registers.
	 * VRN_KERNEL has already been set in start.S.
	 * For paranoia reasons, we set it again.
	 */
	for (i = 0; i < REGION_REGISTERS; i++) {
		rr.word = rr_read(i);
		rr.map.ve = 0;		/* disable VHPT walker */
		rr.map.rid = ASID2RID(ASID_KERNEL, i);
		rr.map.ps = PAGE_WIDTH;
		rr_write(i, rr.word);
		srlz_i();
		srlz_d();
	}

#ifdef CONFIG_VHPT
	vhpt_base = vhpt_set_up();
#endif
	/*
	 * Set up PTA register.
	 */
	pta.word = pta_read();
#ifndef CONFIG_VHPT
	pta.map.ve = 0;                   /* disable VHPT walker */
	pta.map.base = 0 >> PTA_BASE_SHIFT;
#else
	pta.map.ve = 1;                   /* enable VHPT walker */
	pta.map.base = vhpt_base >> PTA_BASE_SHIFT;
#endif
	pta.map.vf = 1;                   /* large entry format */
	pta.map.size = VHPT_WIDTH;
	pta_write(pta.word);
	srlz_i();
	srlz_d();
}

/** Calculate address of collision chain from VPN and ASID.
 *
 * Interrupts must be disabled.
 *
 * @param page		Address of virtual page including VRN bits.
 * @param asid		Address space identifier.
 *
 * @return		VHPT entry address.
 */
vhpt_entry_t *vhpt_hash(uintptr_t page, asid_t asid)
{
	region_register_t rr_save, rr;
	size_t vrn;
	rid_t rid;
	vhpt_entry_t *v;

	vrn = page >> VRN_SHIFT;
	rid = ASID2RID(asid, vrn);

	rr_save.word = rr_read(vrn);
	if (rr_save.map.rid == rid) {
		/*
		 * The RID is already in place, compute thash and return.
		 */
		v = (vhpt_entry_t *) thash(page);
		return v;
	}

	/*
	 * The RID must be written to some region register.
	 * To speed things up, register indexed by vrn is used.
	 */
	rr.word = rr_save.word;
	rr.map.rid = rid;
	rr_write(vrn, rr.word);
	srlz_i();
	v = (vhpt_entry_t *) thash(page);
	rr_write(vrn, rr_save.word);
	srlz_i();
	srlz_d();

	return v;
}

/** Compare ASID and VPN against PTE.
 *
 * Interrupts must be disabled.
 *
 * @param page		Address of virtual page including VRN bits.
 * @param asid		Address space identifier.
 *
 * @return		True if page and asid match the page and asid of t,
 * 			false otherwise.
 */
bool vhpt_compare(uintptr_t page, asid_t asid, vhpt_entry_t *v)
{
	region_register_t rr_save, rr;
	size_t vrn;
	rid_t rid;
	bool match;

	assert(v);

	vrn = page >> VRN_SHIFT;
	rid = ASID2RID(asid, vrn);

	rr_save.word = rr_read(vrn);
	if (rr_save.map.rid == rid) {
		/*
		 * The RID is already in place, compare ttag with t and return.
		 */
		return ttag(page) == v->present.tag.tag_word;
	}

	/*
	 * The RID must be written to some region register.
	 * To speed things up, register indexed by vrn is used.
	 */
	rr.word = rr_save.word;
	rr.map.rid = rid;
	rr_write(vrn, rr.word);
	srlz_i();
	match = (ttag(page) == v->present.tag.tag_word);
	rr_write(vrn, rr_save.word);
	srlz_i();
	srlz_d();

	return match;
}

/** Set up one VHPT entry.
 *
 * @param v VHPT entry to be set up.
 * @param page		Virtual address of the page mapped by the entry.
 * @param asid		Address space identifier of the address space to which
 * 			page belongs.
 * @param frame		Physical address of the frame to wich page is mapped.
 * @param flags		Different flags for the mapping.
 */
void
vhpt_set_record(vhpt_entry_t *v, uintptr_t page, asid_t asid, uintptr_t frame,
    int flags)
{
	region_register_t rr_save, rr;
	size_t vrn;
	rid_t rid;
	uint64_t tag;

	assert(v);

	vrn = page >> VRN_SHIFT;
	rid = ASID2RID(asid, vrn);

	/*
	 * Compute ttag.
	 */
	rr_save.word = rr_read(vrn);
	rr.word = rr_save.word;
	rr.map.rid = rid;
	rr_write(vrn, rr.word);
	srlz_i();
	tag = ttag(page);
	rr_write(vrn, rr_save.word);
	srlz_i();
	srlz_d();

	/*
	 * Clear the entry.
	 */
	v->word[0] = 0;
	v->word[1] = 0;
	v->word[2] = 0;
	v->word[3] = 0;

	v->present.p = true;
	v->present.ma = (flags & PAGE_CACHEABLE) ?
	    MA_WRITEBACK : MA_UNCACHEABLE;
	v->present.a = false;  /* not accessed */
	v->present.d = false;  /* not dirty */
	v->present.pl = (flags & PAGE_USER) ? PL_USER : PL_KERNEL;
	v->present.ar = (flags & PAGE_WRITE) ? AR_WRITE : AR_READ;
	v->present.ar |= (flags & PAGE_EXEC) ? AR_EXECUTE : 0;
	v->present.ppn = frame >> PPN_SHIFT;
	v->present.ed = false;  /* exception not deffered */
	v->present.ps = PAGE_WIDTH;
	v->present.key = 0;
	v->present.tag.tag_word = tag;
}

/** @}
 */
