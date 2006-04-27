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

/**
 * Kernel backend for futexes.
 * Deallocation of orphaned kernel-side futex structures is not currently implemented.
 */

#include <synch/futex.h>
#include <synch/rwlock.h>
#include <synch/spinlock.h>
#include <synch/synch.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <proc/thread.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <arch.h>
#include <align.h>
#include <panic.h>
#include <errno.h>
#include <print.h>

#define FUTEX_HT_SIZE	1024	/* keep it a power of 2 */

static void futex_initialize(futex_t *futex);

static futex_t *futex_find(__address paddr);
static index_t futex_ht_hash(__native *key);
static bool futex_ht_compare(__native *key, count_t keys, link_t *item);
static void futex_ht_remove_callback(link_t *item);

/** Read-write lock protecting global futex hash table. */
static rwlock_t futex_ht_lock;

/** Futex hash table. */
static hash_table_t futex_ht;

/** Futex hash table operations. */
static hash_table_operations_t futex_ht_ops = {
	.hash = futex_ht_hash,
	.compare = futex_ht_compare,
	.remove_callback = futex_ht_remove_callback
};

/** Initialize futex subsystem. */
void futex_init(void)
{
	rwlock_initialize(&futex_ht_lock);
	hash_table_create(&futex_ht, FUTEX_HT_SIZE, 1, &futex_ht_ops);
}

/** Initialize kernel futex structure.
 *
 * @param futex Kernel futex structure.
 */
void futex_initialize(futex_t *futex)
{
	waitq_initialize(&futex->wq);
	link_initialize(&futex->ht_link);
	futex->paddr = 0;
}

/** Sleep in futex wait queue.
 *
 * @param uaddr Userspace address of the futex counter.
 * @param usec If non-zero, number of microseconds this thread is willing to sleep.
 * @param trydown If usec is zero and trydown is non-zero, conditional operation will be attempted.
 *
 * @return One of ESYNCH_TIMEOUT, ESYNCH_OK_ATOMIC and ESYNCH_OK_BLOCKED. See synch.h.
 *	   If there is no physical mapping for uaddr ENOENT is returned.
 */
__native sys_futex_sleep_timeout(__address uaddr, __u32 usec, int trydown)
{
	futex_t *futex;
	__address paddr;
	pte_t *t;
	ipl_t ipl;
	
	ipl = interrupts_disable();

	/*
	 * Find physical address of futex counter.
	 */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE));
	if (!t || !PTE_VALID(t) || !PTE_PRESENT(t)) {
		page_table_unlock(AS, true);
		interrupts_restore(ipl);
		return (__native) ENOENT;
	}
	paddr = PFN2ADDR(PTE_GET_FRAME(t)) + (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	page_table_unlock(AS, true);
	
	interrupts_restore(ipl);	

	futex = futex_find(paddr);
	
	return (__native) waitq_sleep_timeout(&futex->wq, usec, trydown);
}

/** Wakeup one thread waiting in futex wait queue.
 *
 * @param uaddr Userspace address of the futex counter.
 *
 * @return ENOENT if there is no futex associated with the address
 *	   or if there is no physical mapping for uaddr.
 */
__native sys_futex_wakeup(__address uaddr)
{
	futex_t *futex;
	__address paddr;
	pte_t *t;
	ipl_t ipl;
	
	ipl = interrupts_disable();
	
	/*
	 * Find physical address of futex counter.
	 */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE));
	if (!t || !PTE_VALID(t) || !PTE_PRESENT(t)) {
		page_table_unlock(AS, true);
		interrupts_restore(ipl);
		return (__native) ENOENT;
	}
	paddr = PFN2ADDR(PTE_GET_FRAME(t)) + (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	page_table_unlock(AS, true);
	
	interrupts_restore(ipl);

	futex = futex_find(paddr);
		
	waitq_wakeup(&futex->wq, WAKEUP_FIRST);
	
	return 0;
}

/** Find kernel address of the futex structure corresponding to paddr.
 *
 * If the structure does not exist alreay, a new one is created.
 *
 * @param paddr Physical address of the userspace futex counter.
 *
 * @return Address of the kernel futex structure.
 */
futex_t *futex_find(__address paddr)
{
	link_t *item;
	futex_t *futex;
	
	/*
	 * Find the respective futex structure
	 * or allocate new one if it does not exist already.
	 */
	rwlock_read_lock(&futex_ht_lock);
	item = hash_table_find(&futex_ht, &paddr);
	if (item) {
		futex = hash_table_get_instance(item, futex_t, ht_link);
		rwlock_read_unlock(&futex_ht_lock);
	} else {
		/*
		 * Upgrade to writer is not currently supported,
		 * Therefore, it is necessary to release the read lock
		 * and reacquire it as a writer.
		 */
		rwlock_read_unlock(&futex_ht_lock);

		rwlock_write_lock(&futex_ht_lock);
		/*
		 * Avoid possible race condition by searching
		 * the hash table once again with write access.
		 */
		item = hash_table_find(&futex_ht, &paddr);
		if (item) {
			futex = hash_table_get_instance(item, futex_t, ht_link);
			rwlock_write_unlock(&futex_ht_lock);
		} else {
			futex = (futex_t *) malloc(sizeof(futex_t), 0);
			futex_initialize(futex);
			futex->paddr = paddr;
			hash_table_insert(&futex_ht, &paddr, &futex->ht_link);
			rwlock_write_unlock(&futex_ht_lock);
		}
	}
	
	return futex;
}

/** Compute hash index into futex hash table.
 *
 * @param key Address where the key (i.e. physical address of futex counter) is stored.
 *
 * @return Index into futex hash table.
 */
index_t futex_ht_hash(__native *key)
{
	return *key & (FUTEX_HT_SIZE-1);
}

/** Compare futex hash table item with a key.
 *
 * @param key Address where the key (i.e. physical address of futex counter) is stored.
 *
 * @return True if the item matches the key. False otherwise.
 */
bool futex_ht_compare(__native *key, count_t keys, link_t *item)
{
	futex_t *futex;

	ASSERT(keys == 1);

	futex = hash_table_get_instance(item, futex_t, ht_link);
	return *key == futex->paddr;
}

/** Callback for removal items from futex hash table.
 *
 * @param item Item removed from the hash table.
 */
void futex_ht_remove_callback(link_t *item)
{
	futex_t *futex;

	futex = hash_table_get_instance(item, futex_t, ht_link);
	free(futex);
}
