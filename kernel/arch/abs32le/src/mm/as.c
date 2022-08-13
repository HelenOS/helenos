/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */

#include <mm/as.h>
#include <arch/mm/as.h>
#include <genarch/mm/page_pt.h>

void as_arch_init(void)
{
	as_operations = &as_pt_operations;
}

/** @}
 */
