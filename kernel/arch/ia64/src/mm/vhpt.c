/*
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

#include <mem.h>
#include <arch/mm/vhpt.h>
#include <mm/frame.h>
#include <print.h>

static vhpt_entry_t *vhpt_base;

uintptr_t vhpt_set_up(void)
{
	uintptr_t vhpt_frame =
	    frame_alloc(SIZE2FRAMES(VHPT_SIZE), FRAME_ATOMIC, 0);
	if (!vhpt_frame)
		panic("Kernel configured with VHPT but no memory for table.");

	vhpt_base = (vhpt_entry_t *) PA2KA(vhpt_frame);
	vhpt_invalidate_all();
	return (uintptr_t) vhpt_base;
}


void vhpt_mapping_insert(uintptr_t va, asid_t asid, tlb_entry_t entry)
{
	region_register_t rr_save, rr;
	size_t vrn;
	rid_t rid;
	uint64_t tag;

	vhpt_entry_t *ventry;

	vrn = va >> VRN_SHIFT;
	rid = ASID2RID(asid, vrn);

	rr_save.word = rr_read(vrn);
	rr.word = rr_save.word;
	rr.map.rid = rid;
	rr_write(vrn, rr.word);
	srlz_i();

	ventry = (vhpt_entry_t *) thash(va);
	tag = ttag(va);
	rr_write(vrn, rr_save.word);
	srlz_i();
	srlz_d();

	ventry->word[0] = entry.word[0];
	ventry->word[1] = entry.word[1];
	ventry->present.tag.tag_word = tag;
}

void vhpt_invalidate_all(void)
{
	memsetb(vhpt_base, VHPT_SIZE, 0);
}

void vhpt_invalidate_asid(asid_t asid)
{
	vhpt_invalidate_all();
}

/** @}
 */
