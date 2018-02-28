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
#include <assert.h>
#include <adt/hash.h>
#include <adt/hash_table.h>
#include <align.h>

static size_t ht_hash(const ht_link_t *);
static size_t ht_key_hash(void *);
static bool ht_key_equal(void *, const ht_link_t *);
static void ht_remove_callback(ht_link_t *);

static void ht_mapping_insert(as_t *, uintptr_t, uintptr_t, unsigned int);
static void ht_mapping_remove(as_t *, uintptr_t);
static bool ht_mapping_find(as_t *, uintptr_t, bool, pte_t *);
static void ht_mapping_update(as_t *, uintptr_t, bool, pte_t *);
static void ht_mapping_make_global(uintptr_t, size_t);

slab_cache_t *pte_cache = NULL;

/**
 * This lock protects the page hash table. It must be acquired
 * after address space lock and after any address space area
 * locks.
 *
 */
IRQ_SPINLOCK_STATIC_INITIALIZE(page_ht_lock);

/** Page hash table.
 *
 * The page hash table may be accessed only when page_ht_lock is held.
 *
 */
hash_table_t page_ht;

/** Hash table operations for page hash table. */
hash_table_ops_t ht_ops = {
	.hash = ht_hash,
	.key_hash = ht_key_hash,
	.key_equal = ht_key_equal,
	.remove_callback = ht_remove_callback
};

/** Page mapping operations for page hash table architectures. */
page_mapping_operations_t ht_mapping_operations = {
	.mapping_insert = ht_mapping_insert,
	.mapping_remove = ht_mapping_remove,
	.mapping_find = ht_mapping_find,
	.mapping_update = ht_mapping_update,
	.mapping_make_global = ht_mapping_make_global
};

/** Return the hash of the key stored in the item */
size_t ht_hash(const ht_link_t *item)
{
	pte_t *pte = hash_table_get_inst(item, pte_t, link);
	size_t hash = 0;
	hash = hash_combine(hash, (uintptr_t) pte->as);
	hash = hash_combine(hash, pte->page >> PAGE_WIDTH);
	return hash;
}

/** Return the hash of the key. */
size_t ht_key_hash(void *arg)
{
	uintptr_t *key = (uintptr_t *) arg;
	size_t hash = 0;
	hash = hash_combine(hash, key[KEY_AS]);
	hash = hash_combine(hash, key[KEY_PAGE] >> PAGE_WIDTH);
	return hash;
}

/** Return true if the key is equal to the item's lookup key. */
bool ht_key_equal(void *arg, const ht_link_t *item)
{
	uintptr_t *key = (uintptr_t *) arg;
	pte_t *pte = hash_table_get_inst(item, pte_t, link);
	return (key[KEY_AS] == (uintptr_t) pte->as) &&
	    (key[KEY_PAGE] == pte->page);
}

/** Callback on page hash table item removal.
 *
 * @param item Page hash table item being removed.
 *
 */
void ht_remove_callback(ht_link_t *item)
{
	assert(item);

	pte_t *pte = hash_table_get_inst(item, pte_t, link);
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
	uintptr_t key[2] = {
		[KEY_AS] = (uintptr_t) as,
		[KEY_PAGE] = ALIGN_DOWN(page, PAGE_SIZE)
	};

	assert(page_table_locked(as));

	irq_spinlock_lock(&page_ht_lock, true);

	if (!hash_table_find(&page_ht, key)) {
		pte_t *pte = slab_alloc(pte_cache, FRAME_LOWMEM | FRAME_ATOMIC);
		assert(pte != NULL);

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

		hash_table_insert(&page_ht, &pte->link);
	}

	irq_spinlock_unlock(&page_ht_lock, true);
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
	uintptr_t key[2] = {
		[KEY_AS] = (uintptr_t) as,
		[KEY_PAGE] = ALIGN_DOWN(page, PAGE_SIZE)
	};

	assert(page_table_locked(as));

	irq_spinlock_lock(&page_ht_lock, true);

	/*
	 * Note that removed PTE's will be freed
	 * by remove_callback().
	 */
	hash_table_remove(&page_ht, key);

	irq_spinlock_unlock(&page_ht_lock, true);
}

static pte_t *
ht_mapping_find_internal(as_t *as, uintptr_t page, bool nolock)
{
	uintptr_t key[2] = {
		[KEY_AS] = (uintptr_t) as,
		[KEY_PAGE] = ALIGN_DOWN(page, PAGE_SIZE)
	};

	assert(nolock || page_table_locked(as));

	ht_link_t *cur = hash_table_find(&page_ht, key);
	if (cur)
		return hash_table_get_inst(cur, pte_t, link);

	return NULL;
}

/** Find mapping for virtual page in page hash table.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param[out] pte Structure that will receive a copy of the found PTE.
 *
 * @return True if the mapping was found, false otherwise.
 */
bool ht_mapping_find(as_t *as, uintptr_t page, bool nolock, pte_t *pte)
{
	irq_spinlock_lock(&page_ht_lock, true);

	pte_t *t = ht_mapping_find_internal(as, page, nolock);
	if (t)
		*pte = *t;

	irq_spinlock_unlock(&page_ht_lock, true);

	return t != NULL;
}

/** Update mapping for virtual page in page hash table.
 *
 * @param as       Address space to which page belongs.
 * @param page     Virtual page.
 * @param nolock   True if the page tables need not be locked.
 * @param pte      New PTE.
 */
void ht_mapping_update(as_t *as, uintptr_t page, bool nolock, pte_t *pte)
{
	irq_spinlock_lock(&page_ht_lock, true);

	pte_t *t = ht_mapping_find_internal(as, page, nolock);
	if (!t)
		panic("Updating non-existent PTE");

	assert(pte->as == t->as);
	assert(pte->page == t->page);
	assert(pte->frame == t->frame);
	assert(pte->g == t->g);
	assert(pte->x == t->x);
	assert(pte->w == t->w);
	assert(pte->k == t->k);
	assert(pte->c == t->c);
	assert(pte->p == t->p);

	t->a = pte->a;
	t->d = pte->d;

	irq_spinlock_unlock(&page_ht_lock, true);
}

void ht_mapping_make_global(uintptr_t base, size_t size)
{
	/* nothing to do */
}

/** @}
 */
