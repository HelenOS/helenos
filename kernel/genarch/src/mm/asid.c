/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
 * @{
 */

/**
 * @file
 * @brief ASID management.
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

#include <assert.h>
#include <mm/asid.h>
#include <mm/as.h>
#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <adt/list.h>

static size_t asids_allocated = 0;

/** Allocate free address space identifier.
 *
 * @return New ASID.
 */
asid_t asid_get(void)
{
	asid_t asid;
	link_t *tmp;
	as_t *as;

	assert(interrupts_disabled());
	assert(spinlock_locked(&asidlock));

	/*
	 * Check if there is an unallocated ASID.
	 */

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
		tmp = list_first(&inactive_as_with_asid_list);
		assert(tmp != NULL);
		list_remove(tmp);

		as = list_get_instance(tmp, as_t, inactive_as_with_asid_link);

		/*
		 * Steal the ASID.
		 * Note that the stolen ASID is not active.
		 */
		asid = as->asid;
		assert(asid != ASID_INVALID);

		/*
		 * Notify the address space from wich the ASID
		 * was stolen by invalidating its asid member.
		 */
		as->asid = ASID_INVALID;

		/*
		 * If the architecture uses some software cache
		 * of TLB entries (e.g. TSB on sparc64), the
		 * cache must be invalidated as well.
		 */
		as_invalidate_translation_cache(as, 0, (size_t) -1);

		/*
		 * Get the system rid of the stolen ASID.
		 */
		ipl_t ipl = tlb_shootdown_start(TLB_INVL_ASID, asid, 0, 0);
		tlb_invalidate_asid(asid);
		tlb_shootdown_finalize(ipl);
	} else {

		/*
		 * There is at least one unallocated ASID.
		 * Find it and assign it.
		 */

		asid = asid_find_free();
		asids_allocated++;

		/*
		 * Purge the allocated ASID from TLBs.
		 */
		ipl_t ipl = tlb_shootdown_start(TLB_INVL_ASID, asid, 0, 0);
		tlb_invalidate_asid(asid);
		tlb_shootdown_finalize(ipl);
	}

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
	asids_allocated--;
	asid_put_arch(asid);
}

/** @}
 */
