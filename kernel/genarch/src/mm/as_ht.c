/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch_mm
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
