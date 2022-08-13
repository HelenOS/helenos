/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 * @ingroup kernel_ia32_mm, kernel_amd64_mm
 */

#include <arch/mm/as.h>
#include <genarch/mm/page_pt.h>

/** Architecture dependent address space init. */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
}

/** @}
 */
