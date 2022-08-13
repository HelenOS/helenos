/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <arch/mm/asid.h>
#include <arch/mm/page.h>
#include <assert.h>
#include <genarch/mm/as_ht.h>
#include <genarch/mm/page_ht.h>
#include <genarch/mm/asid_fifo.h>
#include <mm/asid.h>
#include <barrier.h>

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	as_operations = &as_ht_operations;
	asid_fifo_init();
}

/** Prepare registers for switching to another address space.
 *
 * @param as Address space.
 */
void as_install_arch(as_t *as)
{
	region_register_t rr;
	int i;

	assert(as->asid != ASID_INVALID);

	/*
	 * Load respective ASID (7 consecutive RIDs) to
	 * region registers.
	 */
	for (i = 0; i < REGION_REGISTERS; i++) {
		if (i == VRN_KERNEL)
			continue;

		rr.word = rr_read(i);
		rr.map.ve = false;		/* disable VHPT walker */
		rr.map.rid = ASID2RID(as->asid, i);
		rr.map.ps = PAGE_WIDTH;
		rr_write(i, rr.word);
		srlz_d();
		srlz_i();
	}
}

/** @}
 */
