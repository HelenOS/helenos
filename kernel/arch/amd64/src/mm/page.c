/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <genarch/mm/page_pt.h>
#include <arch/mm/frame.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/asm.h>
#include <config.h>
#include <interrupt.h>
#include <panic.h>
#include <align.h>
#include <macros.h>

void page_arch_init(void)
{
	if (config.cpu_active > 1) {
		write_cr3((uintptr_t) AS_KERNEL->genarch.page_table);
		return;
	}

	uintptr_t cur;
	unsigned int identity_flags =
	    PAGE_GLOBAL | PAGE_CACHEABLE | PAGE_EXEC | PAGE_WRITE | PAGE_READ;

	page_mapping_operations = &pt_mapping_operations;

	page_table_lock(AS_KERNEL, true);

	/*
	 * PA2KA(identity) mapping for all low-memory frames.
	 */
	for (cur = 0; cur < min(config.identity_size, config.physmem_end);
	    cur += FRAME_SIZE)
		page_mapping_insert(AS_KERNEL, PA2KA(cur), cur, identity_flags);

	page_table_unlock(AS_KERNEL, true);

	exc_register(VECTOR_PF, "page_fault", true, (iroutine_t) page_fault);
	write_cr3((uintptr_t) AS_KERNEL->genarch.page_table);
}

void page_fault(unsigned int n, istate_t *istate)
{
	uintptr_t badvaddr = read_cr2();

	if (istate->error_word & PFERR_CODE_RSVD)
		panic("Reserved bit set in page table entry.");

	pf_access_t access;

	if (istate->error_word & PFERR_CODE_RW)
		access = PF_ACCESS_WRITE;
	else if (istate->error_word & PFERR_CODE_ID)
		access = PF_ACCESS_EXEC;
	else
		access = PF_ACCESS_READ;

	(void) as_page_fault(badvaddr, access, istate);
}

/** @}
 */
