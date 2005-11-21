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
#include <mm/frame.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memstr.h>

void page_init(void)
{
	page_arch_init();
	page_mapping_insert(0x0, 0x0, PAGE_NOT_PRESENT, 0);
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
		page_mapping_insert(s + i*PAGE_SIZE, s + i*PAGE_SIZE, PAGE_NOT_CACHEABLE, 0);

}

/** Map page to frame
 *
 * Map virtual address 'page' to physical address 'frame'
 * using 'flags'. Allocate and setup any missing page tables.
 *
 * @param page Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 * @param root Explicit PTL0 address.
 */
void page_mapping_insert(__address page, __address frame, int flags, __address root)
{
	pte_t *ptl0, *ptl1, *ptl2, *ptl3;
	__address newpt;

	ptl0 = (pte_t *) PA2KA(root ? root : (__address) GET_PTL0_ADDRESS());

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = frame_alloc(FRAME_KA);
		memsetb(newpt, PAGE_SIZE, 0);
		SET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page), KA2PA(newpt));
		SET_PTL1_FLAGS(ptl0, PTL0_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC);
	}

	ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = frame_alloc(FRAME_KA);
		memsetb(newpt, PAGE_SIZE, 0);
		SET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page), KA2PA(newpt));
		SET_PTL2_FLAGS(ptl1, PTL1_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC);
	}

	ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT) {
		newpt = frame_alloc(FRAME_KA);
		memsetb(newpt, PAGE_SIZE, 0);
		SET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page), KA2PA(newpt));
		SET_PTL3_FLAGS(ptl2, PTL2_INDEX(page), PAGE_PRESENT | PAGE_USER | PAGE_EXEC);
	}

	ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	SET_FRAME_ADDRESS(ptl3, PTL3_INDEX(page), frame);
	SET_FRAME_FLAGS(ptl3, PTL3_INDEX(page), flags);
}

/** Find mapping for virtual page
 *
 * Find mapping for virtual page.
 *
 * @param page Virtual page.
 * @param root PTL0 address if non-zero.
 *
 * @return NULL if there is no such mapping; entry from PTL3 describing the mapping otherwise.
 */
pte_t *page_mapping_find(__address page, __address root)
{
	pte_t *ptl0, *ptl1, *ptl2, *ptl3;

	ptl0 = (pte_t *) PA2KA(root ? root : (__address) GET_PTL0_ADDRESS());

	if (GET_PTL1_FLAGS(ptl0, PTL0_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl1 = (pte_t *) PA2KA(GET_PTL1_ADDRESS(ptl0, PTL0_INDEX(page)));

	if (GET_PTL2_FLAGS(ptl1, PTL1_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl2 = (pte_t *) PA2KA(GET_PTL2_ADDRESS(ptl1, PTL1_INDEX(page)));

	if (GET_PTL3_FLAGS(ptl2, PTL2_INDEX(page)) & PAGE_NOT_PRESENT)
		return NULL;

	ptl3 = (pte_t *) PA2KA(GET_PTL3_ADDRESS(ptl2, PTL2_INDEX(page)));

	return &ptl3[PTL3_INDEX(page)];
}
