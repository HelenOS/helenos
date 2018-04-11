/*
 * Copyright (c) 2010 Stanislav Kozina
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#include <assert.h>
#include <typedefs.h>
#include <abi/sysinfo.h>
#include <sysinfo/stats.h>
#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <time/clock.h>
#include <mm/frame.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <interrupt.h>
#include <stdbool.h>
#include <str.h>
#include <errno.h>
#include <cpu.h>
#include <arch.h>

/** Bits of fixed-point precision for load */
#define LOAD_FIXED_SHIFT  11

/** Uspace load fixed-point precision */
#define LOAD_USPACE_SHIFT  6

/** Kernel load shift */
#define LOAD_KERNEL_SHIFT  (LOAD_FIXED_SHIFT - LOAD_USPACE_SHIFT)

/** 1.0 as fixed-point for load */
#define LOAD_FIXED_1  (1 << LOAD_FIXED_SHIFT)

/** Compute load in 5 second intervals */
#define LOAD_INTERVAL  5

/** Fixed-point representation of
 *
 * 1 / exp(5 sec / 1 min)
 * 1 / exp(5 sec / 5 min)
 * 1 / exp(5 sec / 15 min)
 *
 */
static load_t load_exp[LOAD_STEPS] = { 1884, 2014, 2037 };

/** Running average of the number of ready threads */
static load_t avenrdy[LOAD_STEPS] = { 0, 0, 0 };

/** Load calculation lock */
static mutex_t load_lock;

/** Get statistics of all CPUs
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data containing several stats_cpu_t structures.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_cpus(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	*size = sizeof(stats_cpu_t) * config.cpu_count;
	if (dry_run)
		return NULL;

	/* Assumption: config.cpu_count is constant */
	stats_cpu_t *stats_cpus = (stats_cpu_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_cpus == NULL) {
		*size = 0;
		return NULL;
	}

	size_t i;
	for (i = 0; i < config.cpu_count; i++) {
		irq_spinlock_lock(&cpus[i].lock, true);

		stats_cpus[i].id = cpus[i].id;
		stats_cpus[i].active = cpus[i].active;
		stats_cpus[i].frequency_mhz = cpus[i].frequency_mhz;
		stats_cpus[i].busy_cycles = cpus[i].busy_cycles;
		stats_cpus[i].idle_cycles = cpus[i].idle_cycles;

		irq_spinlock_unlock(&cpus[i].lock, true);
	}

	return ((void *) stats_cpus);
}

/** Count number of nodes in an AVL tree
 *
 * AVL tree walker for counting nodes.
 *
 * @param node AVL tree node (unused).
 * @param arg  Pointer to the counter (size_t).
 *
 * @param Always true (continue the walk).
 *
 */
static bool avl_count_walker(avltree_node_t *node, void *arg)
{
	size_t *count = (size_t *) arg;
	(*count)++;

	return true;
}

/** Get the size of a virtual address space
 *
 * @param as Address space.
 *
 * @return Size of the mapped virtual address space (bytes).
 *
 */
static size_t get_task_virtmem(as_t *as)
{
	/*
	 * We are holding spinlocks here and therefore are not allowed to
	 * block. Only attempt to lock the address space and address space
	 * area mutexes conditionally. If it is not possible to lock either
	 * object, return inexact statistics by skipping the respective object.
	 */

	if (mutex_trylock(&as->lock) != EOK)
		return 0;

	size_t pages = 0;

	/* Walk the B+ tree and count pages */
	list_foreach(as->as_area_btree.leaf_list, leaf_link, btree_node_t,
	    node) {
		unsigned int i;
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];

			if (mutex_trylock(&area->lock) != EOK)
				continue;

			pages += area->pages;
			mutex_unlock(&area->lock);
		}
	}

	mutex_unlock(&as->lock);

	return (pages << PAGE_WIDTH);
}

/** Get the resident (used) size of a virtual address space
 *
 * @param as Address space.
 *
 * @return Size of the resident (used) virtual address space (bytes).
 *
 */
static size_t get_task_resmem(as_t *as)
{
	/*
	 * We are holding spinlocks here and therefore are not allowed to
	 * block. Only attempt to lock the address space and address space
	 * area mutexes conditionally. If it is not possible to lock either
	 * object, return inexact statistics by skipping the respective object.
	 */

	if (mutex_trylock(&as->lock) != EOK)
		return 0;

	size_t pages = 0;

	/* Walk the B+ tree and count pages */
	list_foreach(as->as_area_btree.leaf_list, leaf_link, btree_node_t, node) {
		unsigned int i;
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];

			if (mutex_trylock(&area->lock) != EOK)
				continue;

			pages += area->resident;
			mutex_unlock(&area->lock);
		}
	}

	mutex_unlock(&as->lock);

	return (pages << PAGE_WIDTH);
}

/* Produce task statistics
 *
 * Summarize task information into task statistics.
 *
 * @param task       Task.
 * @param stats_task Task statistics.
 *
 */
static void produce_stats_task(task_t *task, stats_task_t *stats_task)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&task->lock));

	stats_task->task_id = task->taskid;
	str_cpy(stats_task->name, TASK_NAME_BUFLEN, task->name);
	stats_task->virtmem = get_task_virtmem(task->as);
	stats_task->resmem = get_task_resmem(task->as);
	stats_task->threads = atomic_get(&task->refcount);
	task_get_accounting(task, &(stats_task->ucycles),
	    &(stats_task->kcycles));
	stats_task->ipc_info = task->ipc_info;
}

/** Gather statistics of all tasks
 *
 * AVL task tree walker for gathering task statistics. Interrupts should
 * be already disabled while walking the tree.
 *
 * @param node AVL task tree node.
 * @param arg  Pointer to the iterator into the array of stats_task_t.
 *
 * @param Always true (continue the walk).
 *
 */
static bool task_serialize_walker(avltree_node_t *node, void *arg)
{
	stats_task_t **iterator = (stats_task_t **) arg;
	task_t *task = avltree_get_instance(node, task_t, tasks_tree_node);

	/* Interrupts are already disabled */
	irq_spinlock_lock(&(task->lock), false);

	/* Record the statistics and increment the iterator */
	produce_stats_task(task, *iterator);
	(*iterator)++;

	irq_spinlock_unlock(&(task->lock), false);

	return true;
}

/** Get task statistics
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data containing several stats_task_t structures.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_tasks(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	/* Messing with task structures, avoid deadlock */
	irq_spinlock_lock(&tasks_lock, true);

	/* First walk the task tree to count the tasks */
	size_t count = 0;
	avltree_walk(&tasks_tree, avl_count_walker, (void *) &count);

	if (count == 0) {
		/* No tasks found (strange) */
		irq_spinlock_unlock(&tasks_lock, true);
		*size = 0;
		return NULL;
	}

	*size = sizeof(stats_task_t) * count;
	if (dry_run) {
		irq_spinlock_unlock(&tasks_lock, true);
		return NULL;
	}

	stats_task_t *stats_tasks = (stats_task_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_tasks == NULL) {
		/* No free space for allocation */
		irq_spinlock_unlock(&tasks_lock, true);
		*size = 0;
		return NULL;
	}

	/* Walk tha task tree again to gather the statistics */
	stats_task_t *iterator = stats_tasks;
	avltree_walk(&tasks_tree, task_serialize_walker, (void *) &iterator);

	irq_spinlock_unlock(&tasks_lock, true);

	return ((void *) stats_tasks);
}

/* Produce thread statistics
 *
 * Summarize thread information into thread statistics.
 *
 * @param thread       Thread.
 * @param stats_thread Thread statistics.
 *
 */
static void produce_stats_thread(thread_t *thread, stats_thread_t *stats_thread)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&thread->lock));

	stats_thread->thread_id = thread->tid;
	stats_thread->task_id = thread->task->taskid;
	stats_thread->state = thread->state;
	stats_thread->priority = thread->priority;
	stats_thread->ucycles = thread->ucycles;
	stats_thread->kcycles = thread->kcycles;

	if (thread->cpu != NULL) {
		stats_thread->on_cpu = true;
		stats_thread->cpu = thread->cpu->id;
	} else
		stats_thread->on_cpu = false;
}

/** Gather statistics of all threads
 *
 * AVL three tree walker for gathering thread statistics. Interrupts should
 * be already disabled while walking the tree.
 *
 * @param node AVL thread tree node.
 * @param arg  Pointer to the iterator into the array of thread statistics.
 *
 * @param Always true (continue the walk).
 *
 */
static bool thread_serialize_walker(avltree_node_t *node, void *arg)
{
	stats_thread_t **iterator = (stats_thread_t **) arg;
	thread_t *thread = avltree_get_instance(node, thread_t, threads_tree_node);

	/* Interrupts are already disabled */
	irq_spinlock_lock(&thread->lock, false);

	/* Record the statistics and increment the iterator */
	produce_stats_thread(thread, *iterator);
	(*iterator)++;

	irq_spinlock_unlock(&thread->lock, false);

	return true;
}

/** Get thread statistics
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data containing several stats_task_t structures.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_threads(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	/* Messing with threads structures, avoid deadlock */
	irq_spinlock_lock(&threads_lock, true);

	/* First walk the thread tree to count the threads */
	size_t count = 0;
	avltree_walk(&threads_tree, avl_count_walker, (void *) &count);

	if (count == 0) {
		/* No threads found (strange) */
		irq_spinlock_unlock(&threads_lock, true);
		*size = 0;
		return NULL;
	}

	*size = sizeof(stats_thread_t) * count;
	if (dry_run) {
		irq_spinlock_unlock(&threads_lock, true);
		return NULL;
	}

	stats_thread_t *stats_threads = (stats_thread_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_threads == NULL) {
		/* No free space for allocation */
		irq_spinlock_unlock(&threads_lock, true);
		*size = 0;
		return NULL;
	}

	/* Walk tha thread tree again to gather the statistics */
	stats_thread_t *iterator = stats_threads;
	avltree_walk(&threads_tree, thread_serialize_walker, (void *) &iterator);

	irq_spinlock_unlock(&threads_lock, true);

	return ((void *) stats_threads);
}

/** Get a single task statistics
 *
 * Get statistics of a given task. The task ID is passed
 * as a string (current limitation of the sysinfo interface,
 * but it is still reasonable for the given purpose).
 *
 * @param name    Task ID (string-encoded number).
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Sysinfo return holder. The type of the returned
 *         data is either SYSINFO_VAL_UNDEFINED (unknown
 *         task ID or memory allocation error) or
 *         SYSINFO_VAL_FUNCTION_DATA (in that case the
 *         generated data should be freed within the
 *         sysinfo request context).
 *
 */
static sysinfo_return_t get_stats_task(const char *name, bool dry_run,
    void *data)
{
	/* Initially no return value */
	sysinfo_return_t ret;
	ret.tag = SYSINFO_VAL_UNDEFINED;

	/* Parse the task ID */
	task_id_t task_id;
	if (str_uint64_t(name, NULL, 0, true, &task_id) != EOK)
		return ret;

	/* Messing with task structures, avoid deadlock */
	irq_spinlock_lock(&tasks_lock, true);

	task_t *task = task_find_by_id(task_id);
	if (task == NULL) {
		/* No task with this ID */
		irq_spinlock_unlock(&tasks_lock, true);
		return ret;
	}

	if (dry_run) {
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = NULL;
		ret.data.size = sizeof(stats_task_t);

		irq_spinlock_unlock(&tasks_lock, true);
	} else {
		/* Allocate stats_task_t structure */
		stats_task_t *stats_task =
		    (stats_task_t *) malloc(sizeof(stats_task_t), FRAME_ATOMIC);
		if (stats_task == NULL) {
			irq_spinlock_unlock(&tasks_lock, true);
			return ret;
		}

		/* Correct return value */
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = (void *) stats_task;
		ret.data.size = sizeof(stats_task_t);

		/* Hand-over-hand locking */
		irq_spinlock_exchange(&tasks_lock, &task->lock);

		produce_stats_task(task, stats_task);

		irq_spinlock_unlock(&task->lock, true);
	}

	return ret;
}

/** Get thread statistics
 *
 * Get statistics of a given thread. The thread ID is passed
 * as a string (current limitation of the sysinfo interface,
 * but it is still reasonable for the given purpose).
 *
 * @param name    Thread ID (string-encoded number).
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Sysinfo return holder. The type of the returned
 *         data is either SYSINFO_VAL_UNDEFINED (unknown
 *         thread ID or memory allocation error) or
 *         SYSINFO_VAL_FUNCTION_DATA (in that case the
 *         generated data should be freed within the
 *         sysinfo request context).
 *
 */
static sysinfo_return_t get_stats_thread(const char *name, bool dry_run,
    void *data)
{
	/* Initially no return value */
	sysinfo_return_t ret;
	ret.tag = SYSINFO_VAL_UNDEFINED;

	/* Parse the thread ID */
	thread_id_t thread_id;
	if (str_uint64_t(name, NULL, 0, true, &thread_id) != EOK)
		return ret;

	/* Messing with threads structures, avoid deadlock */
	irq_spinlock_lock(&threads_lock, true);

	thread_t *thread = thread_find_by_id(thread_id);
	if (thread == NULL) {
		/* No thread with this ID */
		irq_spinlock_unlock(&threads_lock, true);
		return ret;
	}

	if (dry_run) {
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = NULL;
		ret.data.size = sizeof(stats_thread_t);

		irq_spinlock_unlock(&threads_lock, true);
	} else {
		/* Allocate stats_thread_t structure */
		stats_thread_t *stats_thread =
		    (stats_thread_t *) malloc(sizeof(stats_thread_t), FRAME_ATOMIC);
		if (stats_thread == NULL) {
			irq_spinlock_unlock(&threads_lock, true);
			return ret;
		}

		/* Correct return value */
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = (void *) stats_thread;
		ret.data.size = sizeof(stats_thread_t);

		/* Hand-over-hand locking */
		irq_spinlock_exchange(&threads_lock, &thread->lock);

		produce_stats_thread(thread, stats_thread);

		irq_spinlock_unlock(&thread->lock, true);
	}

	return ret;
}

/** Get exceptions statistics
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data containing several stats_exc_t structures.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_exceptions(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	*size = sizeof(stats_exc_t) * IVT_ITEMS;

	if ((dry_run) || (IVT_ITEMS == 0))
		return NULL;

	stats_exc_t *stats_exceptions =
	    (stats_exc_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_exceptions == NULL) {
		/* No free space for allocation */
		*size = 0;
		return NULL;
	}

#if (IVT_ITEMS > 0)
	/* Messing with exception table, avoid deadlock */
	irq_spinlock_lock(&exctbl_lock, true);

	unsigned int i;
	for (i = 0; i < IVT_ITEMS; i++) {
		stats_exceptions[i].id = i + IVT_FIRST;
		str_cpy(stats_exceptions[i].desc, EXC_NAME_BUFLEN, exc_table[i].name);
		stats_exceptions[i].hot = exc_table[i].hot;
		stats_exceptions[i].cycles = exc_table[i].cycles;
		stats_exceptions[i].count = exc_table[i].count;
	}

	irq_spinlock_unlock(&exctbl_lock, true);
#endif

	return ((void *) stats_exceptions);
}

/** Get exception statistics
 *
 * Get statistics of a given exception. The exception number
 * is passed as a string (current limitation of the sysinfo
 * interface, but it is still reasonable for the given purpose).
 *
 * @param name    Exception number (string-encoded number).
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Sysinfo return holder. The type of the returned
 *         data is either SYSINFO_VAL_UNDEFINED (unknown
 *         exception number or memory allocation error) or
 *         SYSINFO_VAL_FUNCTION_DATA (in that case the
 *         generated data should be freed within the
 *         sysinfo request context).
 *
 */
static sysinfo_return_t get_stats_exception(const char *name, bool dry_run,
    void *data)
{
	/* Initially no return value */
	sysinfo_return_t ret;
	ret.tag = SYSINFO_VAL_UNDEFINED;

	/* Parse the exception number */
	uint64_t excn;
	if (str_uint64_t(name, NULL, 0, true, &excn) != EOK)
		return ret;

#if (IVT_FIRST > 0)
	if (excn < IVT_FIRST)
		return ret;
#endif

#if (IVT_ITEMS + IVT_FIRST == 0)
	return ret;
#else
	if (excn >= IVT_ITEMS + IVT_FIRST)
		return ret;
#endif

	if (dry_run) {
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = NULL;
		ret.data.size = sizeof(stats_thread_t);
	} else {
		/* Update excn index for accessing exc_table */
		excn -= IVT_FIRST;

		/* Allocate stats_exc_t structure */
		stats_exc_t *stats_exception =
		    (stats_exc_t *) malloc(sizeof(stats_exc_t), FRAME_ATOMIC);
		if (stats_exception == NULL)
			return ret;

		/* Messing with exception table, avoid deadlock */
		irq_spinlock_lock(&exctbl_lock, true);

		/* Correct return value */
		ret.tag = SYSINFO_VAL_FUNCTION_DATA;
		ret.data.data = (void *) stats_exception;
		ret.data.size = sizeof(stats_exc_t);

		stats_exception->id = excn;
		str_cpy(stats_exception->desc, EXC_NAME_BUFLEN, exc_table[excn].name);
		stats_exception->hot = exc_table[excn].hot;
		stats_exception->cycles = exc_table[excn].cycles;
		stats_exception->count = exc_table[excn].count;

		irq_spinlock_unlock(&exctbl_lock, true);
	}

	return ret;
}

/** Get physical memory statistics
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data containing stats_physmem_t.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_physmem(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	*size = sizeof(stats_physmem_t);
	if (dry_run)
		return NULL;

	stats_physmem_t *stats_physmem =
	    (stats_physmem_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_physmem == NULL) {
		*size = 0;
		return NULL;
	}

	zones_stats(&(stats_physmem->total), &(stats_physmem->unavail),
	    &(stats_physmem->used), &(stats_physmem->free));

	return ((void *) stats_physmem);
}

/** Get system load
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    Unused.
 *
 * @return Data several load_t values.
 *         If the return value is not NULL, it should be freed
 *         in the context of the sysinfo request.
 */
static void *get_stats_load(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	*size = sizeof(load_t) * LOAD_STEPS;
	if (dry_run)
		return NULL;

	load_t *stats_load = (load_t *) malloc(*size, FRAME_ATOMIC);
	if (stats_load == NULL) {
		*size = 0;
		return NULL;
	}

	/* To always get consistent values acquire the mutex */
	mutex_lock(&load_lock);

	unsigned int i;
	for (i = 0; i < LOAD_STEPS; i++)
		stats_load[i] = avenrdy[i] << LOAD_KERNEL_SHIFT;

	mutex_unlock(&load_lock);

	return ((void *) stats_load);
}

/** Calculate load
 *
 */
static inline load_t load_calc(load_t load, load_t exp, atomic_count_t ready)
{
	load *= exp;
	load += (ready << LOAD_FIXED_SHIFT) * (LOAD_FIXED_1 - exp);

	return (load >> LOAD_FIXED_SHIFT);
}

/** Load computation thread.
 *
 * Compute system load every few seconds.
 *
 * @param arg Unused.
 *
 */
void kload(void *arg)
{
	thread_detach(THREAD);

	while (true) {
		atomic_count_t ready = atomic_get(&nrdy);

		/* Mutually exclude with get_stats_load() */
		mutex_lock(&load_lock);

		unsigned int i;
		for (i = 0; i < LOAD_STEPS; i++)
			avenrdy[i] = load_calc(avenrdy[i], load_exp[i], ready);

		mutex_unlock(&load_lock);

		thread_sleep(LOAD_INTERVAL);
	}
}

/** Register sysinfo statistical items
 *
 */
void stats_init(void)
{
	mutex_initialize(&load_lock, MUTEX_PASSIVE);

	sysinfo_set_item_gen_data("system.cpus", NULL, get_stats_cpus, NULL);
	sysinfo_set_item_gen_data("system.physmem", NULL, get_stats_physmem, NULL);
	sysinfo_set_item_gen_data("system.load", NULL, get_stats_load, NULL);
	sysinfo_set_item_gen_data("system.tasks", NULL, get_stats_tasks, NULL);
	sysinfo_set_item_gen_data("system.threads", NULL, get_stats_threads, NULL);
	sysinfo_set_item_gen_data("system.exceptions", NULL, get_stats_exceptions, NULL);
	sysinfo_set_subtree_fn("system.tasks", NULL, get_stats_task, NULL);
	sysinfo_set_subtree_fn("system.threads", NULL, get_stats_thread, NULL);
	sysinfo_set_subtree_fn("system.exceptions", NULL, get_stats_exception, NULL);
}

/** @}
 */
