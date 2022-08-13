/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */

#include <mm/as.h>
#include <arch/mm/as.h>
#include <genarch/mm/page_pt.h>

void as_arch_init(void)
{
	as_operations = &as_pt_operations;
}

/** Install address space.
 *
 * Install ASID.
 *
 * @param as Address space structure.
 */
void as_install_arch(as_t *as)
{
	// FIXME
}

/** @}
 */
