/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <arch/cpu.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <align.h>
#include <config.h>
#include <halt.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <interrupt.h>

void page_arch_init(void)
{
	if (config.cpu_active == 1) {
		page_mapping_operations = &pt_mapping_operations;

		page_table_lock(AS_KERNEL, true);

		/*
		 * PA2KA(identity) mapping for all low-memory frames.
		 */
		for (uintptr_t cur = 0;
		    cur < min(config.identity_size, config.physmem_end);
		    cur += FRAME_SIZE)
			page_mapping_insert(AS_KERNEL, PA2KA(cur), cur,
			    PAGE_GLOBAL | PAGE_CACHEABLE | PAGE_EXEC | PAGE_WRITE | PAGE_READ);

		page_table_unlock(AS_KERNEL, true);

		// FIXME: register page fault extension handler

		write_satp((uintptr_t) AS_KERNEL->genarch.page_table);

		/* The boot page table is no longer needed. */
		// FIXME: frame_mark_available(pt_frame, 1);
	}
}

void page_fault(unsigned int n __attribute__((unused)), istate_t *istate)
{
}

void write_satp(uintptr_t ptl0)
{
	uint64_t satp = ((ptl0 >> FRAME_WIDTH) & SATP_PFN_MASK) |
	    SATP_MODE_SV48;

	asm volatile (
	    "csrw sptbr, %[satp]\n"
	    :: [satp] "r" (satp)
	);
}

/** @}
 */
