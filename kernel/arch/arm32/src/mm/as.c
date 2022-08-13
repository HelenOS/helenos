/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik, Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief Address space functions.
 */

#include <arch/mm/as.h>
#include <genarch/mm/as_pt.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/asid_fifo.h>
#include <mm/as.h>
#include <mm/tlb.h>
#include <arch.h>

/** Architecture dependent address space init.
 *
 *  Since ARM supports page tables, #as_pt_operations are used.
 */
void as_arch_init(void)
{
	as_operations = &as_pt_operations;
}

void as_install_arch(as_t *as)
{
	tlb_invalidate_all();
}

/** @}
 */
