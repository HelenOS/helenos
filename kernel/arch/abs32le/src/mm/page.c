/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <align.h>
#include <config.h>
#include <halt.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <stdio.h>
#include <interrupt.h>

void page_arch_init(void)
{
	if (config.cpu_active == 1)
		page_mapping_operations = &pt_mapping_operations;
}

void page_fault(unsigned int n __attribute__((unused)), istate_t *istate)
{
}

/** @}
 */
