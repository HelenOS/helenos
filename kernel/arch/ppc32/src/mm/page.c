/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#include <genarch/mm/page_pt.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <align.h>
#include <config.h>

void page_arch_init(void)
{
	if (config.cpu_active == 1)
		page_mapping_operations = &pt_mapping_operations;
	as_switch(NULL, AS_KERNEL);
}

/** @}
 */
