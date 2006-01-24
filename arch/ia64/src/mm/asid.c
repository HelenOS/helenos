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
 * ASID management.
 *
 * Because ia64 has much wider ASIDs (18-24 bits) compared to other
 * architectures (e.g. 8 bits on mips32 and 12 bits on sparc32), it is
 * inappropriate to use same methods (i.e. genarch/mm/asid_fifo.c) for
 * all of them.
 *
 * Instead, ia64 assigns ASID values from a counter that eventually
 * overflows. When this happens, the counter is reset and all TLBs are
 * entirely invalidated. Furthermore, all address space structures,
 * except for the one with asid == ASID_KERNEL, are assigned new ASID.
 *
 * It is important to understand that, in SPARTAN, one ASID represents
 * RIDS_PER_ASID consecutive hardware RIDs (Region ID's).
 *
 * Note that the algorithm used can handle only the maximum of
 * ASID_OVERFLOW-ASID_START address spaces at a time.
 */

#include <arch/mm/asid.h>
#include <mm/asid.h>
#include <mm/as.h>
#include <mm/tlb.h>
#include <list.h>
#include <typedefs.h>
#include <debug.h>

/**
 * Stores the ASID to be returned next.
 * Must be only accessed when asidlock is held.
 */
static asid_t next_asid = ASID_START;

/** Assign next ASID.
 *
 * On ia64, this function is used only to allocate ASID
 * for a newly created address space. As a side effect,
 * it might attempt to shootdown TLBs and reassign
 * ASIDs to existing address spaces.
 *
 * When calling this function, interrupts must be disabled
 * and the asidlock must be held.
 *
 * @return ASID for new address space.
 */
asid_t asid_find_free(void)
{
	as_t *as;
	link_t *cur;

	if (next_asid == ASID_OVERFLOW) {
		/*
		 * The counter has overflown.
		 */
		 
		/*
		 * Reset the counter.
		 */
		next_asid = ASID_START;

		/*
		 * Initiate TLB shootdown.
		 */
		tlb_shootdown_start(TLB_INVL_ALL, 0, 0, 0);

		/*
		 * Reassign ASIDs to existing address spaces.
		 */
		for (cur = as_with_asid_head.next; cur != &as_with_asid_head; cur = cur->next) {
			ASSERT(next_asid < ASID_OVERFLOW);
			
			as = list_get_instance(cur, as_t, as_with_asid_link);
			
			spinlock_lock(&as->lock);
			as->asid = next_asid++;
			spinlock_unlock(&as->lock);
		}

		/*
		 * Finish TLB shootdown.
		 */
		tlb_shootdown_finalize();
		tlb_invalidate_all();
	}
	
	ASSERT(next_asid < ASID_OVERFLOW);
	return next_asid++;
}
