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
#include <adt/hash_table.h>
#include <adt/list.h>
#include <arch.h>
#include <align.h>
#include <panic.h>
#include <errno.h>

#define FUTEX_HT_SIZE	1024	/* keep it a power of 2 */

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

static size_t futex_ht_hash(sysarg_t *key);
static bool futex_ht_compare(sysarg_t *key, size_t keys, link_t *item);
static void futex_ht_remove_callback(link_t *item);

static size_t task_fut_ht_hash(const cht_link_t *link);
static size_t task_fut_ht_key_hash(void *key);
static bool task_fut_ht_equal(const cht_link_t *item1, const cht_link_t *item2);
static bool task_fut_ht_key_equal(void *key, const cht_link_t *item);


/** Mutex protecting the global futex hash table.
 * 
 * Acquire task specific TASK->futex_list_lock before this mutex.
 */
static mutex_t futex_ht_lock;

/** Global kernel futex hash table. Lock futex_ht_lock before accessing.
 * 
 * Physical address of the futex variable is the lookup key.
 */
static hash_table_t futex_ht;

/** Global kernel futex hash table operations. */
static hash_table_operations_t futex_ht_ops = {
	.hash = futex_ht_hash,
	.compare = futex_ht_compare,
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
	mutex_initialize(&futex_ht_lock, MUTEX_PASSIVE);
	hash_table_create(&futex_ht, FUTEX_HT_SIZE, 1, &futex_ht_ops);
}

/** Initializes the futex structures for the new task. */
void futex_task_init(struct task *task)
{
	task->futexes = malloc(sizeof(struct futex_cache), 0);
	
	cht_create(&task->futexes->ht, 0, 0, 0, true, &task_futex_ht_ops);
}

/** Destroys the futex structures for the dying task. */
void futex_task_deinit(struct task *task)
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
	
	cht_destroy(&cache->ht);
	free(cache);
}

/** Remove references from futexes known to the current task. */
void futex_task_cleanup(void)
{
	/* All threads of this task have terminated. This is the last thread. */
	mutex_lock(&TASK->futex_list_lock);
	
	list_foreach_safe(TASK->futex_list, cur_link, next_link) {
		futex_ptr_t *fut_ptr = member_to_inst(cur_link, futex_ptr_t, cht_link);
		
		/*
		 * The function is free to free the futex. All other threads of this
		 * task have already terminated, so they have also definitely
		 * exited their CHT futex cache protecting rcu reader sections.
		 * Moreover release_ref() only frees the futex if this is the 
		 * last task referencing the futex. Therefore, only threads
		 * of this task may have referenced the futex if it is to be freed.
		 */
		futex_release_ref_locked(fut_ptr->futex);
		
		/* 
		 * No need to remove the futex_ptr from the CHT cache of this task
		 * because futex_task_deinit() will destroy the CHT shortly.
		 */
		list_remove(cur_link);
		free(fut_ptr);
	}
	
	mutex_unlock(&TASK->futex_list_lock);
}


/** Initialize the kernel futex structure.
 *
 * @param futex	Kernel futex structure.
 * @param paddr Physical address of the futex variable.
 */
static void futex_initialize(futex_t *futex, uintptr_t paddr)
{
	waitq_initialize(&futex->wq);
	link_initialize(&futex->ht_link);
	futex->paddr = paddr;
	futex->refcount = 1;
}

/** Increments the counter of tasks referencing the futex. */
static void futex_add_ref(futex_t *futex)
{
	ASSERT(mutex_locked(&futex_ht_lock));
	ASSERT(0 < futex->refcount);
	++futex->refcount;
}

/** Decrements the counter of tasks referencing the futex. May free the futex.*/
static void futex_release_ref(futex_t *futex)
{
	ASSERT(mutex_locked(&futex_ht_lock));
	ASSERT(0 < futex->refcount);
	
	--futex->refcount;
	
	if (0 == futex->refcount) {
		hash_table_remove(&futex_ht, &futex->paddr, 1);
	}
}

/** Decrements the counter of tasks referencing the futex. May free the futex.*/
static void futex_release_ref_locked(futex_t *futex)
{
	mutex_lock(&futex_ht_lock);
	futex_release_ref(futex);
	mutex_unlock(&futex_ht_lock);
}

/** Returns a futex for the virtual address @a uaddr (or creates one). */
static futex_t *get_futex(uintptr_t uaddr)
{
	futex_t *futex = find_cached_futex(uaddr);
	
	if (futex) 
		return futex;

	uintptr_t paddr;

	if (!find_futex_paddr(uaddr, &paddr)) 
		return 0;

	return get_and_cache_futex(paddr, uaddr);
}


/** Finds the physical address of the futex variable. */
static bool find_futex_paddr(uintptr_t uaddr, uintptr_t *paddr)
{
	page_table_lock(AS, true);

	bool found = false;
	pte_t *t = page_mapping_find(AS, ALIGN_DOWN(uaddr, PAGE_SIZE), false);
	
	if (t && PTE_VALID(t) && PTE_PRESENT(t)) {
		found = true;
		*paddr = PTE_GET_FRAME(t) + (uaddr - ALIGN_DOWN(uaddr, PAGE_SIZE));
	}
	
	page_table_unlock(AS, true);
	
	return found;
}

/** Returns the futex cached in this task with the virtual address uaddr. */
static futex_t *find_cached_futex(uintptr_t uaddr)
{
	cht_read_lock();
	
	futex_t *futex;
	cht_link_t *futex_ptr_link = cht_find_lazy(&TASK->futexes->ht, &uaddr);

	if (futex_ptr_link) {
		futex_ptr_t *futex_ptr 
			= member_to_inst(futex_ptr_link, futex_ptr_t, cht_link);
		
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
	mutex_lock(&futex_ht_lock);
	
	link_t *fut_link = hash_table_find(&futex_ht, &phys_addr);
	
	if (fut_link) {
		free(futex);
		futex = member_to_inst(fut_link, futex_t, ht_link);
		futex_add_ref(futex);
	} else {
		futex_initialize(futex, phys_addr);
		hash_table_insert(&futex_ht, &phys_addr, &futex->ht_link);
	}
	
	mutex_unlock(&futex_ht_lock);
	
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
		mutex_lock(&TASK->futex_list_lock);
		list_append(&fut_ptr->all_link, &TASK->futex_list);
		mutex_unlock(&TASK->futex_list_lock);
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
 *			returned. Otherwise returns a wait result as defined in
 *			synch.h.
 */
sysarg_t sys_futex_sleep(uintptr_t uaddr)
{
	futex_t *futex = get_futex(uaddr);
	
	if (!futex) 
		return (sysarg_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif
	int rc = waitq_sleep_timeout(&futex->wq, 0, SYNCH_FLAGS_INTERRUPTIBLE); 
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
	futex_t *futex = get_futex(uaddr);
	
	if (futex) {
		waitq_wakeup(&futex->wq, WAKEUP_FIRST);
		return 0;
	} else {
		return (sysarg_t) ENOENT;
	}
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
	return *(uintptr_t*)key;
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
	uintptr_t uaddr = *(uintptr_t*)key;
	
	return fut_ptr->uaddr == uaddr;
}


/** @}
 */
