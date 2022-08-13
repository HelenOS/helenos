/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <align.h>
#include <arch/cp15.h>
#include <arch/cache.h>
#include <arch/barrier.h>
#include <barrier.h>

/*
 * There are multiple ways ICache can be implemented on ARM machines. Namely
 * PIPT, VIPT, and ASID and VMID tagged VIVT (see ARM Architecture Reference
 * Manual B3.11.2 (p. 1383).  However, CortexA8 Manual states: "For maximum
 * compatibility across processors, ARM recommends that operating systems target
 * the ARMv7 base architecture that uses ASID-tagged VIVT instruction caches,
 * and do not assume the presence of the IVIPT extension. Software that relies
 * on the IVIPT extension might fail in an unpredictable way on an ARMv7
 * implementation that does not include the IVIPT extension." (7.2.6 p. 245).
 * Only PIPT invalidates cache for all VA aliases if one block is invalidated.
 *
 * @note: Supporting ASID and VMID tagged VIVT may need to add ICache
 * maintenance to other places than just smc.
 */

// TODO: Determine CP15_C7_MVA_ALIGN dynamically

void smc_coherence(void *a, size_t l)
{
	uintptr_t end = (uintptr_t) a + l;
	uintptr_t begin = ALIGN_DOWN((uintptr_t) a, CP15_C7_MVA_ALIGN);

	for (uintptr_t addr = begin; addr < end; addr += CP15_C7_MVA_ALIGN) {
		dcache_clean_mva_pou(addr);
	}

	/* Wait for completion */
	dsb();

	icache_invalidate();
	dsb();
	/* Wait for Inst refetch */
	isb();
}
