/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief	Kernel backend for futexes.
 *
 * Kernel futex objects are stored in a global hash table futex_ht
 * where the physical address of the futex variable (futex_t.paddr)
 * is used as the lookup key. As a result multiple address spaces
 * may share the same futex variable.
 *
 * A kernel futex object is created the first time a task accesses
 * the futex (having a futex variable at a physical address not
 * encountered before). Futex object's lifetime is governed by
 * a reference count that represents the number of all the different
 * tasks that reference the futex variable. A futex object is freed
 * when the last task having accessed the futex exits.
 *
 * Each task keeps track of the futex objects it accessed in a list
 * of pointers (futex_ptr_t, task->futex_list) to the different futex
 * objects.
 */

#include <assert.h>
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
#include <adt/hash.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <arch.h>
#include <align.h>
#include <panic.h>
#include <errno.h>

/** Task specific pointer to a global kernel futex object. */
typedef struct futex_ptr {
	/** Link for the list of all futex pointers used by a task. */
	link_t task_link;
	/** Kernel futex object. */
	futex_t *futex;
} futex_ptr_t;

static void futex_initialize(futex_t *futex, uintptr_t paddr);
static void futex_add_ref(futex_t *futex);
static void futex_release_ref(futex_t *futex);
static void futex_release_ref_locked(futex_t *futex);

static futex_t *get_futex(uintptr_t uaddr);
static bool find_futex_paddr(uintptr_t uaddr, uintptr_t *phys_addr);

static size_t futex_ht_hash(const ht_link_t *item);
static size_t futex_ht_key_hash(void *key);
static bool futex_ht_key_equal(void *key, const ht_link_t *item);
static void futex_ht_remove_callback(ht_link_t *item);

/** Mutex protecting the global futex hash table.
 *
 * Acquire task specific TASK->futex_list_lock before this mutex.
 */
SPINLOCK_STATIC_INITIALIZE_NAME(futex_ht_lock, "futex-ht-lock");

/** Global kernel futex hash table. Lock futex_ht_lock before accessing.
 *
 * Physical address of the futex variable is the lookup key.
 */
static hash_table_t futex_ht;

/** Global kernel futex hash table operations. */
static hash_table_ops_t futex_ht_ops = {
	.hash = futex_ht_hash,
	.key_hash = futex_ht_key_hash,
	.key_equal = futex_ht_key_equal,
	.remove_callback = futex_ht_remove_callback
};

/** Initialize futex subsystem. */
void futex_init(void)
{
	hash_table_create(&futex_ht, 0, 0, &futex_ht_ops);
}

/** Initializes the futex structures for the new task. */
void futex_task_init(struct task *task)
{
	list_initialize(&task->futex_list);
	spinlock_initialize(&task->futex_list_lock, "futex-list-lock");
}

/** Remove references from futexes known to the current task. */
void futex_task_cleanup(void)
{
	/* All threads of this task have terminated. This is the last thread. */
	spinlock_lock(&TASK->futex_list_lock);

	list_foreach_safe(TASK->futex_list, cur_link, next_link) {
		futex_ptr_t *futex_ptr = member_to_inst(cur_link, futex_ptr_t,
		    task_link);

		futex_release_ref_locked(futex_ptr->futex);
		free(futex_ptr);
	}

	spinlock_unlock(&TASK->futex_list_lock);
}

/** Initialize the kernel futex structure.
 *
 * @param futex	Kernel futex structure.
 * @param paddr Physical address of the futex variable.
 */
static void futex_initialize(futex_t *futex, uintptr_t paddr)
{
	waitq_initialize(&futex->wq);
	futex->paddr = paddr;
	futex->refcount = 1;
}

/** Increments the counter of tasks referencing the futex. */
static void futex_add_ref(futex_t *futex)
{
	assert(spinlock_locked(&futex_ht_lock));
	assert(futex->refcount > 0);
	++futex->refcount;
}

/** Decrements the counter of tasks referencing the futex. May free the futex.*/
static void futex_release_ref(futex_t *futex)
{
	assert(spinlock_locked(&futex_ht_lock));
	assert(futex->refcount > 0);

	--futex->refcount;

	if (futex->refcount == 0)
		hash_table_remove(&futex_ht, &futex->paddr);
}

/** Decrements the counter of tasks referencing the futex. May free the futex.*/
static void futex_release_ref_locked(futex_t *futex)
{
	spinlock_lock(&futex_ht_lock);
	futex_release_ref(futex);
	spinlock_unlock(&futex_ht_lock);
}

/** Returns a futex for the virtual address @a uaddr (or creates one). */
static futex_t *get_futex(uintptr_t uaddr)
{
	uintptr_t paddr;

	if (!find_futex_paddr(uaddr, &paddr))
		return NULL;

	futex_t *futex = malloc(sizeof(futex_t));
	if (!futex)
		return NULL;

	futex_ptr_t *futex_ptr = malloc(sizeof(futex_ptr_t));
	if (!futex_ptr) {
		free(futex);
		return NULL;
	}

	/*
	 * Find the futex object in the global futex table (or insert it
	 * if it is not present).
	 */
	spinlock_lock(&TASK->futex_list_lock);
	spinlock_lock(&futex_ht_lock);

	ht_link_t *fut_link = hash_table_find(&futex_ht, &paddr);

	if (fut_link) {
		free(futex);
		futex = member_to_inst(fut_link, futex_t, ht_link);

		/*
		 * See if the futex is already known to the TASK
		 */
		bool found = false;
		list_foreach(TASK->futex_list, task_link, futex_ptr_t, fp) {
			if (fp->futex->paddr == paddr) {
				found = true;
				break;
			}
		}
		/*
		 * If not, put it on the TASK->futex_list and bump its reference
		 * count
		 */
		if (!found) {
			list_append(&futex_ptr->task_link, &TASK->futex_list);
			futex_add_ref(futex);
		} else
			free(futex_ptr);
	} else {
		futex_initialize(futex, paddr);
		hash_table_insert(&futex_ht, &futex->ht_link);

		/*
		 * This is a new futex, so it is not on the TASK->futex_list yet
		 */
		futex_ptr->futex = futex;
		list_append(&futex_ptr->task_link, &TASK->futex_list);
	}

	spinlock_unlock(&futex_ht_lock);
	spinlock_unlock(&TASK->futex_list_lock);

	return futex;
}

/** Finds the physical address of the futex variable. */
static bool find_futex_paddr(uintptr_t uaddr, uintptr_t *paddr)
{
	page_table_lock(AS, false);
	spinlock_lock(&futex_ht_lock);

	bool success = false;

	pte_t t;
	bool found;

	found = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE), true, &t);
	if (found && PTE_VALID(&t) && PTE_PRESENT(&t)) {
		success = true;
		*paddr = PTE_GET_FRAME(&t) +
		    (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	}

	spinlock_unlock(&futex_ht_lock);
	page_table_unlock(AS, false);

	return success;
}

/** Sleep in futex wait queue with a timeout.
 *
 *  If the sleep times out or is interrupted, the next wakeup is ignored.
 *  The userspace portion of the call must handle this condition.
 *
 * @param uaddr	 	Userspace address of the futex counter.
 * @param timeout	Maximum number of useconds to sleep. 0 means no limit.
 *
 * @return		If there is no physical mapping for uaddr ENOENT is
 *			returned. Otherwise returns the return value of
 *                      waitq_sleep_timeout().
 */
sys_errno_t sys_futex_sleep(uintptr_t uaddr, uintptr_t timeout)
{
	futex_t *futex = get_futex(uaddr);

	if (!futex)
		return (sys_errno_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif

	errno_t rc = waitq_sleep_timeout(&futex->wq, timeout,
	    SYNCH_FLAGS_INTERRUPTIBLE | SYNCH_FLAGS_FUTEX, NULL);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif

	return (sys_errno_t) rc;
}

/** Wakeup one thread waiting in futex wait queue.
 *
 * @param uaddr		Userspace address of the futex counter.
 *
 * @return		ENOENT if there is no physical mapping for uaddr.
 */
sys_errno_t sys_futex_wakeup(uintptr_t uaddr)
{
	futex_t *futex = get_futex(uaddr);

	if (futex) {
		waitq_wakeup(&futex->wq, WAKEUP_FIRST);
		return EOK;
	} else {
		return (sys_errno_t) ENOENT;
	}
}

/** Return the hash of the key stored in the item */
size_t futex_ht_hash(const ht_link_t *item)
{
	futex_t *futex = hash_table_get_inst(item, futex_t, ht_link);
	return hash_mix(futex->paddr);
}

/** Return the hash of the key */
size_t futex_ht_key_hash(void *key)
{
	uintptr_t *paddr = (uintptr_t *) key;
	return hash_mix(*paddr);
}

/** Return true if the key is equal to the item's lookup key. */
bool futex_ht_key_equal(void *key, const ht_link_t *item)
{
	uintptr_t *paddr = (uintptr_t *) key;
	futex_t *futex = hash_table_get_inst(item, futex_t, ht_link);
	return *paddr == futex->paddr;
}

/** Callback for removal items from futex hash table.
 *
 * @param item		Item removed from the hash table.
 */
void futex_ht_remove_callback(ht_link_t *item)
{
	futex_t *futex;

	futex = hash_table_get_inst(item, futex_t, ht_link);
	free(futex);
}

/** @}
 */
