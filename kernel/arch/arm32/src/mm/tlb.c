/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief TLB related functions.
 */

#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <arch/asm.h>
#include <arch/cp15.h>
#include <typedefs.h>
#include <arch/mm/page.h>
#include <arch/cache.h>
#include <arch/barrier.h>

/** Invalidate all entries in TLB.
 *
 * @note See ARM Architecture reference section 3.7.7 for details.
 */
void tlb_invalidate_all(void)
{
	TLBIALL_write(0);
	/*
	 * "A TLB maintenance operation is only guaranteed to be complete after
	 * the execution of a DSB instruction."
	 * "An ISB instruction, or a return from an exception, causes the
	 * effect of all completed TLB maintenance operations that appear in
	 * program order before the ISB or return from exception to be visible
	 * to all subsequent instructions, including the instruction fetches
	 * for those instructions."
	 * ARM Architecture reference Manual ch. B3.10.1 p. B3-1374 B3-1375
	 */
	dsb();
	isb();
}

/** Invalidate all entries in TLB that belong to specified address space.
 *
 * @param asid Ignored as the ARM architecture doesn't support ASIDs.
 */
void tlb_invalidate_asid(asid_t asid)
{
	tlb_invalidate_all();
	// TODO: why not TLBIASID_write(asid) ?
}

/** Invalidate single entry in TLB
 *
 * @param page Virtual address of the page
 */
static inline void invalidate_page(uintptr_t page)
{
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	if (TLBTR_read() & TLBTR_SEP_FLAG) {
		ITLBIMVA_write(page);
		DTLBIMVA_write(page);
	} else {
		TLBIMVA_write(page);
	}
#elif defined(PROCESSOR_arm920t)
	ITLBIMVA_write(page);
	DTLBIMVA_write(page);
#elif defined(PROCESSOR_arm926ej_s)
	TLBIMVA_write(page);
#else
#error Unknown TLB type
#endif

	/*
	 * "A TLB maintenance operation is only guaranteed to be complete after
	 * the execution of a DSB instruction."
	 * "An ISB instruction, or a return from an exception, causes the
	 * effect of all completed TLB maintenance operations that appear in
	 * program order before the ISB or return from exception to be visible
	 * to all subsequent instructions, including the instruction fetches
	 * for those instructions."
	 * ARM Architecture reference Manual ch. B3.10.1 p. B3-1374 B3-1375
	 */
	dsb();
	isb();
}

/** Invalidate TLB entries for specified page range belonging to specified
 * address space.
 *
 * @param asid Ignored as the ARM architecture doesn't support it.
 * @param page Address of the first page whose entry is to be invalidated.
 * @param cnt Number of entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid __attribute__((unused)), uintptr_t page, size_t cnt)
{
	for (unsigned i = 0; i < cnt; i++)
		invalidate_page(page + i * PAGE_SIZE);
}

void tlb_arch_init(void)
{
}

void tlb_print(void)
{
}

/** @}
 */
