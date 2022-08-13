/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 * @ingroup kernel_ia32_mm, kernel_amd64_mm
 */

#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <arch/asm.h>
#include <typedefs.h>

/** Invalidate all entries in TLB. */
void tlb_invalidate_all(void)
{
	write_cr3(read_cr3());
}

/** Invalidate all entries in TLB that belong to specified address space.
 *
 * @param asid This parameter is ignored as the architecture doesn't support it.
 */
void tlb_invalidate_asid(asid_t asid __attribute__((unused)))
{
	tlb_invalidate_all();
}

/** Invalidate TLB entries for specified page range belonging to specified address space.
 *
 * @param asid This parameter is ignored as the architecture doesn't support it.
 * @param page Address of the first page whose entry is to be invalidated.
 * @param cnt Number of entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid __attribute__((unused)), uintptr_t page, size_t cnt)
{
	unsigned int i;

	for (i = 0; i < cnt; i++)
		invlpg(page + i * PAGE_SIZE);
}

void tlb_arch_init(void)
{
}

void tlb_print(void)
{
}

/** @}
 */
