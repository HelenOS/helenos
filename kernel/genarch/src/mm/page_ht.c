/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genarchmm
 * @{
 */

/**
 * @file
 * @brief Virtual Address Translation (VAT) for global page hash table.
 */

#include <genarch/mm/page_ht.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <mm/as.h>
#include <arch/mm/asid.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <debug.h>
#include <memstr.h>
#include <adt/hash_table.h>
#include <align.h>

static size_t hash(sysarg_t[]);
static bool compare(sysarg_t[], size_t, link_t *);
static void remove_callback(link_t *);

static void ht_mapping_insert(as_t *, uintptr_t, uintptr_t, unsigned int);
static void ht_mapping_remove(as_t *, uintptr_t);
static pte_t *ht_mapping_find(as_t *, uintptr_t, bool);
static void ht_mapping_make_global(uintptr_t, size_t);

slab_cache_t *pte_cache = NULL;

/**
 * This lock protects the page hash table. It must be acquired
 * after address space lock and after any address space area
 * locks.
 *
 */
mutex_t page_ht_lock;

/** Page hash table.
 *
 * The page hash table may be accessed only when page_ht_lock is held.
 *
 */
hash_table_t page_ht;

/** Hash table operations for page hash table. */
hash_table_operations_t ht_operations = {
	.hash = hash,
	.compare = compare,
	.remove_callback = remove_callback
};

/** Page mapping operations for page hash table architectures. */
page_mapping_operations_t ht_mapping_operations = {
	.mapping_insert = ht_mapping_insert,
	.mapping_remove = ht_mapping_remove,
	.mapping_find = ht_mapping_find,
	.mapping_make_global = ht_mapping_make_global
};

/** Compute page hash table index.
 *
 * @param key Array of two keys (i.e. page and address space).
 *
 * @return Index into page hash table.
 *
 */
size_t hash(sysarg_t key[])
{
	as_t *as = (as_t *) key[KEY_AS];
	uintptr_t page = (uintptr_t) key[KEY_PAGE];
	
	/*
	 * Virtual page addresses have roughly the same probability
	 * of occurring. Least significant bits of VPN compose the
	 * hash index.
	 *
	 */
	size_t index = ((page >> PAGE_WIDTH) & (PAGE_HT_ENTRIES - 1));
	
	/*
	 * Address space structures are likely to be allocated from
	 * similar addresses. Least significant bits compose the
	 * hash index.
	 *
	 */
	index |= ((sysarg_t) as) & (PAGE_HT_ENTRIES - 1);
	
	return index;
}

/** Compare page hash table item with page and/or address space.
 *
 * @param key  Array of one or two keys (i.e. page and/or address space).
 * @param keys Number of keys passed.
 * @param item Item to compare the keys with.
 *
 * @return true on match, false otherwise.
 *
 */
bool compare(sysarg_t key[], size_t keys, link_t *item)
{
	ASSERT(item);
	ASSERT(keys > 0);
	ASSERT(keys <= PAGE_HT_KEYS);
	
	/*
	 * Convert item to PTE.
	 *
	 */
	pte_t *pte = hash_table_get_instance(item, pte_t, link);
	
	if (keys == PAGE_HT_KEYS)
		return (key[KEY_AS] == (uintptr_t) pte->as) &&
		    (key[KEY_PAGE] == pte->page);
	
	return (key[KEY_AS] == (uintptr_t) pte->as);
}

/** Callback on page hash table item removal.
 *
 * @param item Page hash table item being removed.
 *
 */
void remove_callback(link_t *item)
{
	ASSERT(item);
	
	/*
	 * Convert item to PTE.
	 *
	 */
	pte_t *pte = hash_table_get_instance(item, pte_t, link);
	
	slab_free(pte_cache, pte);
}

/** Map page to frame using page hash table.
 *
 * Map virtual address page to physical address frame
 * using flags.
 *
 * @param as    Address space to which page belongs.
 * @param page  Virtual address of the page to be mapped.
 * @param frame Physical address of memory frame to which the mapping is done.
 * @param flags Flags to be used for mapping.
 *
 */
void ht_mapping_insert(as_t *as, uintptr_t page, uintptr_t frame,
    unsigned int flags)
{
	sysarg_t key[2] = {
		(uintptr_t) as,
		page = ALIGN_DOWN(page, PAGE_SIZE)
	};

	ASSERT(page_table_locked(as));
	
	if (!hash_table_find(&page_ht, key)) {
		pte_t *pte = slab_alloc(pte_cache, FRAME_LOWMEM | FRAME_ATOMIC);
		ASSERT(pte != NULL);
		
		pte->g = (flags & PAGE_GLOBAL) != 0;
		pte->x = (flags & PAGE_EXEC) != 0;
		pte->w = (flags & PAGE_WRITE) != 0;
		pte->k = !(flags & PAGE_USER);
		pte->c = (flags & PAGE_CACHEABLE) != 0;
		pte->p = !(flags & PAGE_NOT_PRESENT);
		pte->a = false;
		pte->d = false;
		
		pte->as = as;
		pte->page = ALIGN_DOWN(page, PAGE_SIZE);
		pte->frame = ALIGN_DOWN(frame, FRAME_SIZE);

		/*
		 * Make sure that a concurrent ht_mapping_find() will see the
		 * new entry only after it is fully initialized.
		 */
		write_barrier();
		
		hash_table_insert(&page_ht, key, &pte->link);
	}
}

/** Remove mapping of page from page hash table.
 *
 * Remove any mapping of page within address space as.
 * TLB shootdown should follow in order to make effects of
 * this call visible.
 *
 * @param as   Address space to which page belongs.
 * @param page Virtual address of the page to be demapped.
 *
 */
void ht_mapping_remove(as_t *as, uintptr_t page)
{
	sysarg_t key[2] = {
		(uintptr_t) as,
		page = ALIGN_DOWN(page, PAGE_SIZE)
	};

	ASSERT(page_table_locked(as));
	
	/*
	 * Note that removed PTE's will be freed
	 * by remove_callback().
	 */
	hash_table_remove(&page_ht, key, 2);
}


/** Find mapping for virtual page in page hash table.
 *
 * @param as     Address space to which page belongs.
 * @param page   Virtual page.
 * @param nolock True if the page tables need not be locked.
 *
 * @return NULL if there is no such mapping; requested mapping otherwise.
 *
 */
pte_t *ht_mapping_find(as_t *as, uintptr_t page, bool nolock)
{
	sysarg_t key[2] = {
		(uintptr_t) as,
		page = ALIGN_DOWN(page, PAGE_SIZE)
	};

	ASSERT(nolock || page_table_locked(as));
	
	link_t *cur = hash_table_find(&page_ht, key);
	if (cur)
		return hash_table_get_instance(cur, pte_t, link);
	
	return NULL;
}

void ht_mapping_make_global(uintptr_t base, size_t size)
{
	/* nothing to do */
}

/** @}
 */
