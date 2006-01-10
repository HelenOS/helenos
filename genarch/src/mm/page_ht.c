/*
 * Copyright (C) 2006 Jakub Jermar
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

#include <genarch/mm/page_ht.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <debug.h>

static void ht_mapping_insert(__address page, asid_t asid, __address frame, int flags, __address root);
static pte_t *ht_mapping_find(__address page, asid_t asid, __address root);

page_operations_t page_ht_operations = {
	.mapping_insert = ht_mapping_insert,
	.mapping_find = ht_mapping_find
};

/** Map page to frame using page hash table.
 *
 * Map virtual address 'page' to physical address 'frame'
 * using 'flags'.
 *
 * @param page Virtual address of the page to be mapped.
 * @param asid Address space to which page belongs.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 * @param root Ignored.
 */
void ht_mapping_insert(__address page, asid_t asid, __address frame, int flags, __address root)
{
	pte_t *t, *u = NULL;
	
	t = HT_HASH(page, asid);
	if (!HT_SLOT_EMPTY(t)) {
		u = (pte_t *) malloc(sizeof(pte_t));	/* FIXME: use slab allocator for this */
		if (!u)
			panic("could not allocate memory for hash table\n");
		*u = *t;
	}
	HT_SET_NEXT(t, u);
	HT_SET_RECORD(t, page, asid, frame, flags);
}

/** Find mapping for virtual page in page hash table.
 *
 * Find mapping for virtual page.
 *
 * @param page Virtual page.
 * @param asid Address space to wich page belongs.
 * @param root Ignored.
 *
 * @return NULL if there is no such mapping; requested mapping otherwise.
 */
pte_t *ht_mapping_find(__address page, asid_t asid, __address root)
{
	pte_t *t;
	
	t = HT_HASH(page, asid);
	while (!HT_COMPARE(page, asid, t) && HT_GET_NEXT(t))
		t = HT_GET_NEXT(t);
	
	return HT_COMPARE(page, asid, t) ? t : NULL;
}
