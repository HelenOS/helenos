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

#include <genarch/mm/as_ht.h>
#include <genarch/mm/page_ht.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <arch/types.h>
#include <typedefs.h>
#include <memstr.h>
#include <adt/hash_table.h>
#include <synch/spinlock.h>

static pte_t *ht_create(int flags);

static void ht_lock(as_t *as, bool lock);
static void ht_unlock(as_t *as, bool unlock);

as_operations_t as_ht_operations = {
	.page_table_create = ht_create,
	.page_table_lock = ht_lock,
	.page_table_unlock = ht_unlock,
};


/** Page hash table create.
 *
 * The page hash table will be created only once
 * and will be shared by all address spaces.
 *
 * @param flags Ignored.
 *
 * @return Returns NULL.
 */
pte_t *ht_create(int flags)
{
	if (flags & FLAG_AS_KERNEL) {
		hash_table_create(&page_ht, PAGE_HT_ENTRIES, 2, &ht_operations);
	}
	return NULL;
}

/** Lock page table.
 *
 * Lock address space and page hash table.
 * Interrupts must be disabled.
 *
 * @param as Address space.
 * @param lock If false, do not attempt to lock the address space.
 */
void ht_lock(as_t *as, bool lock)
{
	if (lock)
		spinlock_lock(&as->lock);
	spinlock_lock(&page_ht_lock);
}

/** Unlock page table.
 *
 * Unlock address space and page hash table.
 * Interrupts must be disabled.
 *
 * @param as Address space.
 * @param unlock If false, do not attempt to lock the address space.
 */
void ht_unlock(as_t *as, bool unlock)
{
	spinlock_unlock(&page_ht_lock);
	if (unlock)
		spinlock_unlock(&as->lock);
}
