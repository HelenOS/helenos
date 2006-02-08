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
 * Modern processor architectures optimize TLB utilization
 * by using ASIDs (a.k.a. memory contexts on sparc64 and
 * region identifiers on ia64). These ASIDs help to associate
 * each TLB item with an address space, thus making
 * finer-grained TLB invalidation possible.
 *
 * Unfortunatelly, there are usually less ASIDs available than
 * there can be unique as_t structures (i.e. address spaces
 * recognized by the kernel).
 *
 * When system runs short of ASIDs, it will attempt to steal
 * ASID from an address space that has not been active for
 * a while.
 *
 * This code depends on the fact that ASIDS_ALLOCABLE
 * is greater than number of supported CPUs (i.e. the
 * amount of concurently active address spaces).
 *
 * Architectures that don't have hardware support for address
 * spaces do not compile with this file.
 */

#include <mm/asid.h>
#include <mm/as.h>
#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <adt/list.h>
#include <debug.h>

/**
 * asidlock protects the asids_allocated counter.
 */
SPINLOCK_INITIALIZE(asidlock);

static count_t asids_allocated = 0;

/** Allocate free address space identifier.
 *
 * Interrupts must be disabled and as_lock must be held
 * prior to this call
 *
 * @return New ASID.
 */
asid_t asid_get(void)
{
	asid_t asid;
	link_t *tmp;
	as_t *as;

	/*
	 * Check if there is an unallocated ASID.
	 */
	
	spinlock_lock(&asidlock);
	if (asids_allocated == ASIDS_ALLOCABLE) {

		/*
		 * All ASIDs are already allocated.
		 * Resort to stealing.
		 */
		
		/*
		 * Remove the first item on the list.
		 * It is guaranteed to belong to an
		 * inactive address space.
		 */
		ASSERT(!list_empty(&inactive_as_with_asid_head));
		tmp = inactive_as_with_asid_head.next;
		list_remove(tmp);
		
		as = list_get_instance(tmp, as_t, inactive_as_with_asid_link);
		spinlock_lock(&as->lock);

		/*
		 * Steal the ASID.
		 * Note that the stolen ASID is not active.
		 */
		asid = as->asid;
		ASSERT(asid != ASID_INVALID);

		/*
		 * Notify the address space from wich the ASID
		 * was stolen by invalidating its asid member.
		 */
		as->asid = ASID_INVALID;
		spinlock_unlock(&as->lock);

		/*
		 * Get the system rid of the stolen ASID.
		 */
		tlb_shootdown_start(TLB_INVL_ASID, asid, 0, 0);
		tlb_shootdown_finalize();
		tlb_invalidate_asid(asid);
	} else {

		/*
		 * There is at least one unallocated ASID.
		 * Find it and assign it.
		 */

		asid = asid_find_free();
		asids_allocated++;
	}
	
	spinlock_unlock(&asidlock);
	
	return asid;
}

/** Release address space identifier.
 *
 * This code relies on architecture
 * dependent functionality.
 *
 * @param asid ASID to be released.
 */
void asid_put(asid_t asid)
{
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&asidlock);

	asids_allocated--;
	asid_put_arch(asid);
	
	spinlock_unlock(&asidlock);
	interrupts_restore(ipl);
}
