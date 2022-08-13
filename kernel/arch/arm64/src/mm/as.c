/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Address space functions.
 */

#include <arch/mm/as.h>
#include <arch/regutils.h>
#include <genarch/mm/asid_fifo.h>
#include <genarch/mm/page_pt.h>
#include <mm/as.h>
#include <mm/asid.h>

/** Architecture dependent address space init.
 *
 * Since ARM64 supports page tables, #as_pt_operations are used.
 */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
	asid_fifo_init();
}

/** Perform ARM64-specific tasks when an address space becomes active on the
 * processor.
 *
 * Change the level 0 page table (this is normally done by
 * SET_PTL0_ADDRESS_ARCH() on other architectures) and install ASID.
 *
 * @param as Address space.
 */
void as_install_arch(as_t *as)
{
	uint64_t val;

	val = (uint64_t) as->genarch.page_table;
	if (as->asid != ASID_KERNEL) {
		val |= (uint64_t) as->asid << TTBR0_ASID_SHIFT;
		TTBR0_EL1_write(val);
	} else
		TTBR1_EL1_write(val);
}

/** @}
 */
