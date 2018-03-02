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
 * @brief Address space functions for global page hash table.
 */

#include <arch/mm/as.h>
#include <genarch/mm/as_ht.h>
#include <genarch/mm/page_ht.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <adt/hash_table.h>
#include <synch/mutex.h>

static pte_t *ht_create(unsigned int);
static void ht_destroy(pte_t *);

static void ht_lock(as_t *, bool);
static void ht_unlock(as_t *, bool);
static bool ht_locked(as_t *);

as_operations_t as_ht_operations = {
	.page_table_create = ht_create,
	.page_table_destroy = ht_destroy,
	.page_table_lock = ht_lock,
	.page_table_unlock = ht_unlock,
	.page_table_locked = ht_locked,
};


/** Page hash table create.
 *
 * The page hash table will be created only once
 * and will be shared by all address spaces.
 *
 * @param flags Ignored.
 *
 * @return Returns NULL.
 *
 */
pte_t *ht_create(unsigned int flags)
{
	if (flags & FLAG_AS_KERNEL) {
		hash_table_create(&page_ht, 0, 0, &ht_ops);
		pte_cache = slab_cache_create("pte_t", sizeof(pte_t), 0,
		    NULL, NULL, SLAB_CACHE_MAGDEFERRED);
	}

	return NULL;
}

/** Destroy page table.
 *
 * Actually do nothing as the global page hash table is used.
 *
 * @param page_table This parameter is ignored.
 *
 */
void ht_destroy(pte_t *page_table)
{
	/* No-op. */
}

/** Lock page table.
 *
 * Lock address space.
 * Interrupts must be disabled.
 *
 * @param as   Address space.
 * @param lock If false, do not attempt to lock the address space.
 *
 */
void ht_lock(as_t *as, bool lock)
{
	if (lock)
		mutex_lock(&as->lock);
}

/** Unlock page table.
 *
 * Unlock address space.
 * Interrupts must be disabled.
 *
 * @param as     Address space.
 * @param unlock If false, do not attempt to lock the address space.
 *
 */
void ht_unlock(as_t *as, bool unlock)
{
	if (unlock)
		mutex_unlock(&as->lock);
}

/** Test whether page tables are locked.
 *
 * @param as		Address space where the page tables belong.
 *
 * @return		True if the page tables belonging to the address soace
 *			are locked, otherwise false.
 */
bool ht_locked(as_t *as)
{
	return mutex_locked(&as->lock);
}

/** @}
 */
