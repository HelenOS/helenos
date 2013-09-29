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

/** @addtogroup sync
 * @{
 */

/**
 * @file
 * @brief	Kernel backend for futexes.
 */

#include <synch/futex.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <proc/thread.h>
#include <proc/task.h>
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

static futex_t *futex_find(uintptr_t paddr);
static size_t futex_ht_hash(sysarg_t *key);
static bool futex_ht_compare(sysarg_t *key, size_t keys, link_t *item);
static void futex_ht_remove_callback(link_t *item);

/**
 * Mutex protecting global futex hash table.
 * It is also used to serialize access to all futex_t structures.
 * Must be acquired before the task futex B+tree lock.
 */
static mutex_t futex_ht_lock;

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
	mutex_initialize(&futex_ht_lock, MUTEX_PASSIVE);
	hash_table_create(&futex_ht, FUTEX_HT_SIZE, 1, &futex_ht_ops);
}

/** Initialize kernel futex structure.
 *
 * @param futex		Kernel futex structure.
 */
void futex_initialize(futex_t *futex)
{
	waitq_initialize(&futex->wq);
	link_initialize(&futex->ht_link);
	futex->paddr = 0;
	futex->refcount = 1;
}

/** Sleep in futex wait queue.
 *
 * @param uaddr		Userspace address of the futex counter.
 *
 * @return		If there is no physical mapping for uaddr ENOENT is
 *			returned. Otherwise returns a wait result as defined in
 *			synch.h.
 */
sysarg_t sys_futex_sleep(uintptr_t uaddr)
{
	futex_t *futex;
	uintptr_t paddr;
	pte_t *t;
	int rc;
	
	/*
	 * Find physical address of futex counter.
	 */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE), false);
	if (!t || !PTE_VALID(t) || !PTE_PRESENT(t)) {
		page_table_unlock(AS, true);
		return (sysarg_t) ENOENT;
	}
	paddr = PTE_GET_FRAME(t) + (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	page_table_unlock(AS, true);
	
	futex = futex_find(paddr);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif
	rc = waitq_sleep_timeout(&futex->wq, 0, SYNCH_FLAGS_INTERRUPTIBLE); 
#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif
	return (sysarg_t) rc;
}

/** Wakeup one thread waiting in futex wait queue.
 *
 * @param uaddr		Userspace address of the futex counter.
 *
 * @return		ENOENT if there is no physical mapping for uaddr.
 */
sysarg_t sys_futex_wakeup(uintptr_t uaddr)
{
	futex_t *futex;
	uintptr_t paddr;
	pte_t *t;
	
	/*
	 * Find physical address of futex counter.
	 */
	page_table_lock(AS, true);
	t = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE), false);
	if (!t || !PTE_VALID(t) || !PTE_PRESENT(t)) {
		page_table_unlock(AS, true);
		return (sysarg_t) ENOENT;
	}
	paddr = PTE_GET_FRAME(t) + (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	page_table_unlock(AS, true);
	
	futex = futex_find(paddr);
		
	waitq_wakeup(&futex->wq, WAKEUP_FIRST);
	
	return 0;
}

/** Find kernel address of the futex structure corresponding to paddr.
 *
 * If the structure does not exist already, a new one is created.
 *
 * @param paddr		Physical address of the userspace futex counter.
 *
 * @return		Address of the kernel futex structure.
 */
futex_t *futex_find(uintptr_t paddr)
{
	link_t *item;
	futex_t *futex;
	btree_node_t *leaf;
	
	/*
	 * Find the respective futex structure
	 * or allocate new one if it does not exist already.
	 */
	mutex_lock(&futex_ht_lock);
	item = hash_table_find(&futex_ht, &paddr);
	if (item) {
		futex = hash_table_get_instance(item, futex_t, ht_link);

		/*
		 * See if the current task knows this futex.
		 */
		mutex_lock(&TASK->futexes_lock);
		if (!btree_search(&TASK->futexes, paddr, &leaf)) {
			/*
			 * The futex is new to the current task.
			 * Upgrade its reference count and put it to the
			 * current task's B+tree of known futexes.
			 */
			futex->refcount++;
			btree_insert(&TASK->futexes, paddr, futex, leaf);
		}
		mutex_unlock(&TASK->futexes_lock);
	} else {
		futex = (futex_t *) malloc(sizeof(futex_t), 0);
		futex_initialize(futex);
		futex->paddr = paddr;
		hash_table_insert(&futex_ht, &paddr, &futex->ht_link);
			
		/*
		 * This is the first task referencing the futex.
		 * It can be directly inserted into its
		 * B+tree of known futexes.
		 */
		mutex_lock(&TASK->futexes_lock);
		btree_insert(&TASK->futexes, paddr, futex, NULL);
		mutex_unlock(&TASK->futexes_lock);
		
	}
	mutex_unlock(&futex_ht_lock);
	
	return futex;
}

/** Compute hash index into futex hash table.
 *
 * @param key		Address where the key (i.e. physical address of futex
 *			counter) is stored.
 *
 * @return		Index into futex hash table.
 */
size_t futex_ht_hash(sysarg_t *key)
{
	return (*key & (FUTEX_HT_SIZE - 1));
}

/** Compare futex hash table item with a key.
 *
 * @param key		Address where the key (i.e. physical address of futex
 *			counter) is stored.
 *
 * @return		True if the item matches the key. False otherwise.
 */
bool futex_ht_compare(sysarg_t *key, size_t keys, link_t *item)
{
	futex_t *futex;

	ASSERT(keys == 1);

	futex = hash_table_get_instance(item, futex_t, ht_link);
	return *key == futex->paddr;
}

/** Callback for removal items from futex hash table.
 *
 * @param item		Item removed from the hash table.
 */
void futex_ht_remove_callback(link_t *item)
{
	futex_t *futex;

	futex = hash_table_get_instance(item, futex_t, ht_link);
	free(futex);
}

/** Remove references from futexes known to the current task. */
void futex_cleanup(void)
{
	mutex_lock(&futex_ht_lock);
	mutex_lock(&TASK->futexes_lock);

	list_foreach(TASK->futexes.leaf_list, leaf_link, btree_node_t, node) {
		unsigned int i;
		
		for (i = 0; i < node->keys; i++) {
			futex_t *ftx;
			uintptr_t paddr = node->key[i];
			
			ftx = (futex_t *) node->value[i];
			if (--ftx->refcount == 0)
				hash_table_remove(&futex_ht, &paddr, 1);
		}
	}
	
	mutex_unlock(&TASK->futexes_lock);
	mutex_unlock(&futex_ht_lock);
}

/** @}
 */
