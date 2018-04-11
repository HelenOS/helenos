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

/** @addtogroup sync
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
 * user space virtual addresses from all tasks that map to the
 * physical address of the futex variable. A futex object is freed
 * when the last task having accessed the futex exits.
 *
 * Each task keeps track of the futex objects it accessed in a list
 * of pointers (futex_ptr_t, task->futex_list) to the different futex
 * objects.
 *
 * To speed up translation of futex variables' virtual addresses
 * to their physical addresses, futex pointers accessed by the
 * task are furthermore stored in a concurrent hash table (CHT,
 * task->futexes->ht). A single lookup without locks or accesses
 * to the page table translates a futex variable's virtual address
 * into its futex kernel object.
 */

#include <assert.h>
#include <synch/futex.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <synch/rcu.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <adt/cht.h>
#include <adt/hash.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <arch.h>
#include <align.h>
#include <panic.h>
#include <errno.h>

/** Task specific pointer to a global kernel futex object. */
typedef struct futex_ptr {
	/** CHT link. */
	cht_link_t cht_link;
	/** List of all futex pointers used by the task. */
	link_t all_link;
	/** Kernel futex object. */
	futex_t *futex;
	/** User space virtual address of the futex variable in the task. */
	uintptr_t uaddr;
} futex_ptr_t;


static void destroy_task_cache(work_t *work);

static void futex_initialize(futex_t *futex, uintptr_t paddr);
static void futex_add_ref(futex_t *futex);
static void futex_release_ref(futex_t *futex);
static void futex_release_ref_locked(futex_t *futex);

static futex_t *get_futex(uintptr_t uaddr);
static futex_t *find_cached_futex(uintptr_t uaddr);
static futex_t *get_and_cache_futex(uintptr_t phys_addr, uintptr_t uaddr);
static bool find_futex_paddr(uintptr_t uaddr, uintptr_t *phys_addr);

static size_t futex_ht_hash(const ht_link_t *item);
static size_t futex_ht_key_hash(void *key);
static bool futex_ht_key_equal(void *key, const ht_link_t *item);
static void futex_ht_remove_callback(ht_link_t *item);

static size_t task_fut_ht_hash(const cht_link_t *link);
static size_t task_fut_ht_key_hash(void *key);
static bool task_fut_ht_equal(const cht_link_t *item1, const cht_link_t *item2);
static bool task_fut_ht_key_equal(void *key, const cht_link_t *item);


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

/** Task futex cache CHT operations. */
static cht_ops_t task_futex_ht_ops = {
	.hash = task_fut_ht_hash,
	.key_hash = task_fut_ht_key_hash,
	.equal = task_fut_ht_equal,
	.key_equal = task_fut_ht_key_equal,
	.remove_callback = NULL
};

/** Initialize futex subsystem. */
void futex_init(void)
{
	hash_table_create(&futex_ht, 0, 0, &futex_ht_ops);
}

/** Initializes the futex structures for the new task. */
void futex_task_init(struct task *task)
{
	task->futexes = malloc(sizeof(struct futex_cache), 0);

	cht_create(&task->futexes->ht, 0, 0, 0, true, &task_futex_ht_ops);

	list_initialize(&task->futexes->list);
	spinlock_initialize(&task->futexes->list_lock, "futex-list-lock");
}

/** Destroys the futex structures for the dying task. */
void futex_task_deinit(task_t *task)
{
	/* Interrupts are disabled so we must not block (cannot run cht_destroy). */
	if (interrupts_disabled()) {
		/* Invoke the blocking cht_destroy in the background. */
		workq_global_enqueue_noblock(&task->futexes->destroy_work,
		    destroy_task_cache);
	} else {
		/* We can block. Invoke cht_destroy in this thread. */
		destroy_task_cache(&task->futexes->destroy_work);
	}
}

/** Deallocates a task's CHT futex cache (must already be empty). */
static void destroy_task_cache(work_t *work)
{
	struct futex_cache *cache =
	    member_to_inst(work, struct futex_cache, destroy_work);

	/*
	 * Destroy the cache before manually freeing items of the cache in case
	 * table resize is in progress.
	 */
	cht_destroy_unsafe(&cache->ht);

	/* Manually free futex_ptr cache items. */
	list_foreach_safe(cache->list, cur_link, next_link) {
		futex_ptr_t *fut_ptr = member_to_inst(cur_link, futex_ptr_t, all_link);

		list_remove(cur_link);
		free(fut_ptr);
	}

	free(cache);
}

/** Remove references from futexes known to the current task. */
void futex_task_cleanup(void)
{
	struct futex_cache *futexes = TASK->futexes;

	/* All threads of this task have terminated. This is the last thread. */
	spinlock_lock(&futexes->list_lock);

	list_foreach_safe(futexes->list, cur_link, next_link) {
		futex_ptr_t *fut_ptr = member_to_inst(cur_link, futex_ptr_t, all_link);

		/*
		 * The function is free to free the futex. All other threads of this
		 * task have already terminated, so they have also definitely
		 * exited their CHT futex cache protecting rcu reader sections.
		 * Moreover release_ref() only frees the futex if this is the
		 * last task referencing the futex. Therefore, only threads
		 * of this task may have referenced the futex if it is to be freed.
		 */
		futex_release_ref_locked(fut_ptr->futex);
	}

	spinlock_unlock(&futexes->list_lock);
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
	assert(0 < futex->refcount);
	++futex->refcount;
}

/** Decrements the counter of tasks referencing the futex. May free the futex.*/
static void futex_release_ref(futex_t *futex)
{
	assert(spinlock_locked(&futex_ht_lock));
	assert(0 < futex->refcount);

	--futex->refcount;

	if (0 == futex->refcount) {
		hash_table_remove(&futex_ht, &futex->paddr);
	}
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
	futex_t *futex = find_cached_futex(uaddr);

	if (futex)
		return futex;

	uintptr_t paddr;

	if (!find_futex_paddr(uaddr, &paddr)) {
		return 0;
	}

	return get_and_cache_futex(paddr, uaddr);
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

/** Returns the futex cached in this task with the virtual address uaddr. */
static futex_t *find_cached_futex(uintptr_t uaddr)
{
	cht_read_lock();

	futex_t *futex;
	cht_link_t *futex_ptr_link = cht_find_lazy(&TASK->futexes->ht, &uaddr);

	if (futex_ptr_link) {
		futex_ptr_t *futex_ptr =
		    member_to_inst(futex_ptr_link, futex_ptr_t, cht_link);

		futex = futex_ptr->futex;
	} else {
		futex = NULL;
	}

	cht_read_unlock();

	return futex;
}


/**
 * Returns a kernel futex for the physical address @a phys_addr and caches
 * it in this task under the virtual address @a uaddr (if not already cached).
 */
static futex_t *get_and_cache_futex(uintptr_t phys_addr, uintptr_t uaddr)
{
	futex_t *futex = malloc(sizeof(futex_t), 0);

	/*
	 * Find the futex object in the global futex table (or insert it
	 * if it is not present).
	 */
	spinlock_lock(&futex_ht_lock);

	ht_link_t *fut_link = hash_table_find(&futex_ht, &phys_addr);

	if (fut_link) {
		free(futex);
		futex = member_to_inst(fut_link, futex_t, ht_link);
		futex_add_ref(futex);
	} else {
		futex_initialize(futex, phys_addr);
		hash_table_insert(&futex_ht, &futex->ht_link);
	}

	spinlock_unlock(&futex_ht_lock);

	/*
	 * Cache the link to the futex object for this task.
	 */
	futex_ptr_t *fut_ptr = malloc(sizeof(futex_ptr_t), 0);
	cht_link_t *dup_link;

	fut_ptr->futex = futex;
	fut_ptr->uaddr = uaddr;

	cht_read_lock();

	/* Cache the mapping from the virtual address to the futex for this task. */
	if (cht_insert_unique(&TASK->futexes->ht, &fut_ptr->cht_link, &dup_link)) {
		spinlock_lock(&TASK->futexes->list_lock);
		list_append(&fut_ptr->all_link, &TASK->futexes->list);
		spinlock_unlock(&TASK->futexes->list_lock);
	} else {
		/* Another thread of this task beat us to it. Use that mapping instead.*/
		free(fut_ptr);
		futex_release_ref_locked(futex);

		futex_ptr_t *dup = member_to_inst(dup_link, futex_ptr_t, cht_link);
		futex = dup->futex;
	}

	cht_read_unlock();

	return futex;
}

/** Sleep in futex wait queue.
 *
 * @param uaddr		Userspace address of the futex counter.
 *
 * @return		If there is no physical mapping for uaddr ENOENT is
 *			returned. Otherwise returns the return value of
 *                      waitq_sleep_timeout().
 */
sys_errno_t sys_futex_sleep(uintptr_t uaddr)
{
	futex_t *futex = get_futex(uaddr);

	if (!futex)
		return (sys_errno_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif

	errno_t rc = waitq_sleep_timeout(
	    &futex->wq, 0, SYNCH_FLAGS_INTERRUPTIBLE, NULL);

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

/*
 * Operations of a task's CHT that caches mappings of futex user space
 * virtual addresses to kernel futex objects.
 */

static size_t task_fut_ht_hash(const cht_link_t *link)
{
	const futex_ptr_t *fut_ptr = member_to_inst(link, futex_ptr_t, cht_link);
	return fut_ptr->uaddr;
}

static size_t task_fut_ht_key_hash(void *key)
{
	return *(uintptr_t *)key;
}

static bool task_fut_ht_equal(const cht_link_t *item1, const cht_link_t *item2)
{
	const futex_ptr_t *fut_ptr1 = member_to_inst(item1, futex_ptr_t, cht_link);
	const futex_ptr_t *fut_ptr2 = member_to_inst(item2, futex_ptr_t, cht_link);

	return fut_ptr1->uaddr == fut_ptr2->uaddr;
}

static bool task_fut_ht_key_equal(void *key, const cht_link_t *item)
{
	const futex_ptr_t *fut_ptr = member_to_inst(item, futex_ptr_t, cht_link);
	uintptr_t uaddr = *(uintptr_t *)key;

	return fut_ptr->uaddr == uaddr;
}

/** @}
 */
