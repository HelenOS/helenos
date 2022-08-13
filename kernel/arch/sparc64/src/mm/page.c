/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#include <mm/page.h>
#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <bitops.h>
#include <debug.h>
#include <align.h>
#include <config.h>

/** Perform sparc64 specific initialization of paging. */
void page_arch_init(void)
{
	if (config.cpu_active == 1)
		page_mapping_operations = &ht_mapping_operations;
}

/** @}
 */
