/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <genarch/mm/as_pt.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/asid_fifo.h>
#include <arch/mm/tlb.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <arch/cp0.h>

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
	asid_fifo_init();
}

/** Install address space.
 *
 * Install ASID.
 *
 * @param as Address space structure.
 */
void as_install_arch(as_t *as)
{
	entry_hi_t hi;

	/*
	 * Install ASID.
	 */
	hi.value = cp0_entry_hi_read();

	hi.asid = as->asid;
	cp0_entry_hi_write(hi.value);
}

/** @}
 */
