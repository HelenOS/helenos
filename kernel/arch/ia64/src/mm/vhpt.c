/*
 * SPDX-FileCopyrightText: 2006 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#include <mem.h>
#include <arch/barrier.h>
#include <arch/mm/vhpt.h>
#include <mm/frame.h>

static vhpt_entry_t *vhpt_base;

uintptr_t vhpt_set_up(void)
{
	uintptr_t vhpt_frame =
	    frame_alloc(SIZE2FRAMES(VHPT_SIZE), FRAME_LOWMEM | FRAME_ATOMIC, 0);
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
