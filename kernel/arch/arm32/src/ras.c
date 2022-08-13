/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Kernel part of Restartable Atomic Sequences support.
 */

#include <arch/ras.h>
#include <mm/mm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/km.h>
#include <mm/tlb.h>
#include <mm/asid.h>
#include <interrupt.h>
#include <arch/exception.h>
#include <arch.h>
#include <mem.h>
#include <typedefs.h>

uintptr_t *ras_page = NULL;

void ras_init(void)
{
	uintptr_t frame = frame_alloc(1, FRAME_HIGHMEM, 0);

	ras_page = (uintptr_t *) km_map(frame, PAGE_SIZE, PAGE_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_CACHEABLE);

	memsetb(ras_page, PAGE_SIZE, 0);
	ras_page[RAS_START] = 0;
	ras_page[RAS_END] = 0xffffffff;
}

void ras_check(unsigned int n, istate_t *istate)
{
	bool restart_needed = false;
	uintptr_t restart_pc = 0;

	if (istate_from_uspace(istate)) {
		if (ras_page[RAS_START]) {
			if ((ras_page[RAS_START] < istate->pc) &&
			    (ras_page[RAS_END] > istate->pc)) {
				restart_needed = true;
				restart_pc = ras_page[RAS_START];
			}
			ras_page[RAS_START] = 0;
			ras_page[RAS_END] = 0xffffffff;
		}
	}

	exc_dispatch(n, istate);
	if (restart_needed)
		istate->pc = restart_pc;
}

/** @}
 */
