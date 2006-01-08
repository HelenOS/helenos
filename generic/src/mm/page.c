/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <mm/page.h>
#include <arch/mm/page.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>
#include <mm/frame.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memstr.h>
#include <debug.h>
#include <arch.h>

/** Virtual operations for page subsystem. */
page_operations_t *page_operations = NULL;

void page_init(void)
{
	page_arch_init();
	page_mapping_insert(0x0, 0, 0x0, PAGE_NOT_PRESENT, 0);
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

	length = size + (s - (s & ~(PAGE_SIZE-1)));
	cnt = length/PAGE_SIZE + (length%PAGE_SIZE>0);

	for (i = 0; i < cnt; i++)
		page_mapping_insert(s + i*PAGE_SIZE, ASID_KERNEL, s + i*PAGE_SIZE, PAGE_NOT_CACHEABLE, 0);

}

/** Map page to frame
 *
 * Map virtual address 'page' to physical address 'frame'
 * using 'flags'. Allocate and setup any missing page tables.
 *
 * @param page Virtual address of the page to be mapped.
 * @param asid Address space to wich page belongs.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 * @param root Explicit PTL0 address.
 */
void page_mapping_insert(__address page, asid_t asid, __address frame, int flags, __address root)
{
	ASSERT(page_operations);
	ASSERT(page_operations->mapping_insert);
	
	page_operations->mapping_insert(page, asid, frame, flags, root);
}

/** Find mapping for virtual page
 *
 * Find mapping for virtual page.
 *
 * @param page Virtual page.
 * @param asid Address space to wich page belongs.
 * @param root PTL0 address if non-zero.
 *
 * @return NULL if there is no such mapping; entry from PTL3 describing the mapping otherwise.
 */
pte_t *page_mapping_find(__address page,  asid_t asid, __address root)
{
	ASSERT(page_operations);
	ASSERT(page_operations->mapping_find);

	return page_operations->mapping_find(page, asid, root);
}
