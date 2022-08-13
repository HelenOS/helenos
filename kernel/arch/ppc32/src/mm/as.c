/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#include <arch/mm/as.h>
#include <genarch/mm/as_pt.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/asid_fifo.h>
#include <arch.h>

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
 *
 */
void as_install_arch(as_t *as)
{
	uint32_t sr;

	/* Lower 2 GB, user and supervisor access */
	for (sr = 0; sr < 8; sr++)
		sr_set(0x6000, as->asid, sr);

	/* Upper 2 GB, only supervisor access */
	for (sr = 8; sr < 16; sr++)
		sr_set(0x4000, as->asid, sr);
}

/** @}
 */
