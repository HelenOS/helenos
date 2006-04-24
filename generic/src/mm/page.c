/*
 * Copyright (C) 2001-2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Virtual Address Translation subsystem.
 */

#include <mm/page.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memstr.h>
#include <debug.h>
#include <arch.h>

/** Virtual operations for page subsystem. */
page_mapping_operations_t *page_mapping_operations = NULL;

void page_init(void)
{
	page_arch_init();
}

/** Map memory structure
 *
 * Identity-map memory structure
 * considering possible crossings
 * of page boundaries.
 *
 * @param s Address of the structure.
 * @param size Size of the structure.
 */
void map_structure(__address s, size_t size)
{
	int i, cnt, length;

	length = size + (s - (s & ~(PAGE_SIZE - 1)));
	cnt = length / PAGE_SIZE + (length % PAGE_SIZE > 0);

	for (i = 0; i < cnt; i++)
		page_mapping_insert(AS_KERNEL, s + i * PAGE_SIZE, s + i * PAGE_SIZE, PAGE_NOT_CACHEABLE);

}

/** Insert mapping of page to frame.
 *
 * Map virtual address @page to physical address @frame
 * using @flags. Allocate and setup any missing page tables.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 */
void page_mapping_insert(as_t *as, __address page, __address frame, int flags)
{
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_insert);
	
	page_mapping_operations->mapping_insert(as, page, frame, flags);
}

/** Remove mapping of page.
 *
 * Remove any mapping of @page within address space @as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual address of the page to be demapped.
 */
void page_mapping_remove(as_t *as, __address page)
{
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_remove);
	
	page_mapping_operations->mapping_remove(as, page);
}

/** Find mapping for virtual page
 *
 * Find mapping for virtual page.
 *
 * The page table must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual page.
 *
 * @return NULL if there is no such mapping; requested mapping otherwise.
 */
pte_t *page_mapping_find(as_t *as, __address page)
{
	ASSERT(page_mapping_operations);
	ASSERT(page_mapping_operations->mapping_find);

	return page_mapping_operations->mapping_find(as, page);
}
