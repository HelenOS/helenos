/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */

#include <mm/tlb.h>
#include <arch/mm/asid.h>
#include <arch/asm.h>
#include <typedefs.h>

void tlb_invalidate_all(void)
{
}

void tlb_invalidate_asid(asid_t asid __attribute__((unused)))
{
	tlb_invalidate_all();
}

void tlb_invalidate_pages(asid_t asid __attribute__((unused)), uintptr_t page, size_t cnt)
{
	tlb_invalidate_all();
}

void tlb_arch_init(void)
{
}

void tlb_print(void)
{
}

/** @}
 */
