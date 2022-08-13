/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief TLB related functions.
 */

#include <arch/mm/asid.h>
#include <arch/mm/page.h>
#include <arch/regutils.h>
#include <mm/tlb.h>
#include <typedefs.h>

/** Invalidate all entries in TLB. */
void tlb_invalidate_all(void)
{
	asm volatile (
	    /* TLB Invalidate All, EL1, Inner Shareable. */
	    "tlbi alle1is\n"
	    /* Ensure completion on all PEs. */
	    "dsb ish\n"
	    /* Synchronize context on this PE. */
	    "isb\n"
	    : : : "memory"
	);
}

/** Invalidate all entries in TLB that belong to specified address space.
 *
 * @param asid Address Space ID.
 */
void tlb_invalidate_asid(asid_t asid)
{
	uintptr_t val = (uintptr_t)asid << TLBI_ASID_SHIFT;

	asm volatile (
	    /* TLB Invalidate by ASID, EL1, Inner Shareable. */
	    "tlbi aside1is, %[val]\n"
	    /* Ensure completion on all PEs. */
	    "dsb ish\n"
	    /* Synchronize context on this PE. */
	    "isb\n"
	    : : [val] "r" (val) : "memory"
	);
}

/** Invalidate TLB entries for specified page range belonging to specified
 * address space.
 *
 * @param asid Address Space ID.
 * @param page Address of the first page whose entry is to be invalidated.
 * @param cnt  Number of entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		uintptr_t val;

		val = (page + i * PAGE_SIZE) >> PAGE_WIDTH;
		val |= (uintptr_t) asid << TLBI_ASID_SHIFT;

		asm volatile (
		    /* TLB Invalidate by Virt. Address, EL1, Inner Shareable. */
		    "tlbi vae1is, %[val]\n"
		    /* Ensure completion on all PEs. */
		    "dsb ish\n"
		    /* Synchronize context on this PE. */
		    "isb\n"
		    : : [val] "r" (val) : "memory"
		);
	}
}

void tlb_arch_init(void)
{
}

void tlb_print(void)
{
}

/** @}
 */
