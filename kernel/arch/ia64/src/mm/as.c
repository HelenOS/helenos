/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <arch/mm/as.h>
#include <arch/mm/asid.h>
#include <arch/mm/page.h>
#include <assert.h>
#include <genarch/mm/as_ht.h>
#include <genarch/mm/page_ht.h>
#include <genarch/mm/asid_fifo.h>
#include <mm/asid.h>
#include <arch/barrier.h>

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
