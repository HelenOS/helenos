/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
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
#include <typedefs.h>
#include <align.h>
#include <config.h>
#include <halt.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <interrupt.h>
#include <macros.h>

void page_arch_init(void)
{
	uintptr_t cur;
	int flags;

	if (config.cpu_active > 1) {
		/* Fast path for non-boot CPUs */
		write_cr3((uintptr_t) AS_KERNEL->genarch.page_table);
		paging_on();
		return;
	}

	page_mapping_operations = &pt_mapping_operations;

	/*
	 * PA2KA(identity) mapping for all low-memory frames.
	 */
	page_table_lock(AS_KERNEL, true);
	for (cur = 0; cur < min(config.identity_size, config.physmem_end);
	    cur += FRAME_SIZE) {
		flags = PAGE_GLOBAL | PAGE_CACHEABLE | PAGE_WRITE | PAGE_READ;
		page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, flags);
	}
	page_table_unlock(AS_KERNEL, true);

	exc_register(VECTOR_PF, "page_fault", true, (iroutine_t) page_fault);
	write_cr3((uintptr_t) AS_KERNEL->genarch.page_table);

	paging_on();
}

void page_fault(unsigned int n __attribute__((unused)), istate_t *istate)
{
	uintptr_t badvaddr;
	pf_access_t access;

	badvaddr = read_cr2();

	if (istate->error_word & PFERR_CODE_RSVD)
		panic("Reserved bit set in page directory.");

	if (istate->error_word & PFERR_CODE_RW)
		access = PF_ACCESS_WRITE;
	else
		access = PF_ACCESS_READ;

	(void) as_page_fault(badvaddr, access, istate);
}

/** @}
 */
