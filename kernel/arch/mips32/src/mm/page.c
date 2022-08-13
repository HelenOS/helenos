/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/page.h>
#include <mm/frame.h>

void page_arch_init(void)
{
	page_mapping_operations = &pt_mapping_operations;
	as_switch(NULL, AS_KERNEL);
}

/** @}
 */
