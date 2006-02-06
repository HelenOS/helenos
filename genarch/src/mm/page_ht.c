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
#include <arch/mm/page.h>
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
#include <adt/hash_table.h>

static index_t hash(__native key[]);
static bool compare(__native key[], count_t keys, link_t *item);
static void remove_callback(link_t *item);

static void ht_mapping_insert(as_t *as, __address page, __address frame, int flags);
static pte_t *ht_mapping_find(as_t *as, __address page);

/**
 * This lock protects the page hash table.
 */
SPINLOCK_INITIALIZE(page_ht_lock);

/**
 * Page hash table.
 * The page hash table may be accessed only when page_ht_lock is held.
 */
hash_table_t page_ht;

/** Hash table operations for page hash table. */
hash_table_operations_t ht_operations = {
	.hash = hash,
	.compare = compare,
	.remove_callback = remove_callback
};

page_operations_t page_ht_operations = {
	.mapping_insert = ht_mapping_insert,
	.mapping_find = ht_mapping_find
};

/** Compute page hash table index.
 *
 * @param key Array of two keys (i.e. page and address space).
 *
 * @return Index into page hash table.
 */
index_t hash(__native key[])
{
	as_t *as = (as_t *) key[KEY_AS];
	__address page = (__address) key[KEY_PAGE];
	index_t index;
	
	/*
	 * Virtual page addresses have roughly the same probability
	 * of occurring. Least significant bits of VPN compose the
	 * hash index.
	 */
	index = ((page >> PAGE_WIDTH) & (PAGE_HT_ENTRIES-1));
	
	/*
	 * Address space structures are likely to be allocated from
	 * similar addresses. Least significant bits compose the
	 * hash index.
	 */
	index |= ((__native) as) & (PAGE_HT_ENTRIES-1);
	
	return index;
}

/** Compare page hash table item with page and/or address space.
 *
 * @param key Array of one or two keys (i.e. page and/or address space).
 * @param keys Number of keys passed.
 * @param item Item to compare the keys with.
 *
 * @return true on match, false otherwise.
 */
bool compare(__native key[], count_t keys, link_t *item)
{
	pte_t *t;

	ASSERT(item);
	ASSERT((keys > 0) && (keys <= PAGE_HT_KEYS));

	/*
	 * Convert item to PTE.
	 */
	t = list_get_instance(item, pte_t, link);

	if (keys == PAGE_HT_KEYS) {
		return (key[KEY_AS] == (__address) t->as) && (key[KEY_PAGE] == t->page);
	} else {
		return (key[KEY_AS] == (__address) t->as);
	}
}

/** Callback on page hash table item removal.
 *
 * @param item Page hash table item being removed.
 */
void remove_callback(link_t *item)
{
	pte_t *t;

	ASSERT(item);

	/*
	 * Convert item to PTE.
	 */
	t = list_get_instance(item, pte_t, link);

	free(t);
}

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
	pte_t *t;
	ipl_t ipl;
	__native key[2] = { (__address) as, page };
	
	ipl = interrupts_disable();
	spinlock_lock(&page_ht_lock);

	if (!hash_table_find(&page_ht, key)) {
		t = (pte_t *) malloc(sizeof(pte_t));
		ASSERT(t != NULL);
	
		hash_table_insert(&page_ht, key, &t->link);
	}
	
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
	link_t *hlp;
	pte_t *t = NULL;
	__native key[2] = { (__address) as, page };
	
	spinlock_lock(&page_ht_lock);

	hlp = hash_table_find(&page_ht, key);
	if (hlp)
		t = list_get_instance(hlp, pte_t, link);

	spinlock_unlock(&page_ht_lock);
	return t;
}
