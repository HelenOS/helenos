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
#include <mm/as.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <debug.h>
#include <memstr.h>

/**
 * This lock protects the page hash table.
 */
SPINLOCK_INITIALIZE(page_ht_lock);

/**
 * Page hash table pointer.
 * The page hash table may be accessed only when page_ht_lock is held.
 */
pte_t *page_ht = NULL;

static void ht_mapping_insert(as_t *as, __address page, __address frame, int flags);
static pte_t *ht_mapping_find(as_t *as, __address page);

page_operations_t page_ht_operations = {
	.mapping_insert = ht_mapping_insert,
	.mapping_find = ht_mapping_find,
};

/** Map page to frame using page hash table.
 *
 * Map virtual address 'page' to physical address 'frame'
 * using 'flags'. 
 *
 * The address space must be locked and interruptsmust be disabled.
 *
 * @param as Address space to which page belongs.
 * @param page Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 */
void ht_mapping_insert(as_t *as, __address page, __address frame, int flags)
{
	pte_t *t, *u;
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&page_ht_lock);

	t = HT_HASH(page, as->asid);
	if (!HT_SLOT_EMPTY(t)) {
	
		/*
		 * The slot is occupied.
		 * Walk through the collision chain and append the mapping to its end.
		 */
		 
		do {
			u = t;
			if (HT_COMPARE(page, as->asid, t)) {
				/*
				 * Nothing to do,
				 * the record is already there.
				 */
				spinlock_unlock(&page_ht_lock);
				interrupts_restore(ipl);
				return;
			}
		} while ((t = HT_GET_NEXT(t)));
	
		t = (pte_t *) malloc(sizeof(pte_t));	/* FIXME: use slab allocator for this */
		if (!t)
			panic("could not allocate memory\n");

		HT_SET_NEXT(u, t);
	}
	
	HT_SET_RECORD(t, page, as->asid, frame, flags);
	HT_SET_NEXT(t, NULL);
	
	spinlock_unlock(&page_ht_lock);
	interrupts_restore(ipl);
}

/** Find mapping for virtual page in page hash table.
 *
 * Find mapping for virtual page.
 *
 * The address space must be locked and interrupts must be disabled.
 *
 * @param as Address space to wich page belongs.
 * @param page Virtual page.
 *
 * @return NULL if there is no such mapping; requested mapping otherwise.
 */
pte_t *ht_mapping_find(as_t *as, __address page)
{
	pte_t *t;
	
	spinlock_lock(&page_ht_lock);
	t = HT_HASH(page, as->asid);
	if (!HT_SLOT_EMPTY(t)) {
		while (!HT_COMPARE(page, as->asid, t) && HT_GET_NEXT(t))
			t = HT_GET_NEXT(t);
		t = HT_COMPARE(page, as->asid, t) ? t : NULL;
	} else {
		t = NULL;
	}
	spinlock_unlock(&page_ht_lock);
	return t;
}

/** Invalidate page hash table.
 *
 * Interrupts must be disabled.
 */
void ht_invalidate_all(void)
{
	pte_t *t, *u;
	int i;
	
	spinlock_lock(&page_ht_lock);
	for (i = 0; i < HT_ENTRIES; i++) {
		if (!HT_SLOT_EMPTY(&page_ht[i])) {
			t = HT_GET_NEXT(&page_ht[i]);
			while (t) {
				u = t;
				t = HT_GET_NEXT(t);
				free(u);		/* FIXME: use slab allocator for this */
			}
			HT_INVALIDATE_SLOT(&page_ht[i]);
		}
	}
	spinlock_unlock(&page_ht_lock);
}
