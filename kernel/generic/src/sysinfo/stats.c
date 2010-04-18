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

#include <typedefs.h>
#include <sysinfo/abi.h>
#include <sysinfo/stats.h>
#include <sysinfo/sysinfo.h>
#include <time/clock.h>
#include <mm/frame.h>
#include <proc/thread.h>
#include <str.h>
#include <errno.h>
#include <cpu.h>
#include <arch.h>

/** Bits of fixed-point precision for load */
#define LOAD_FIXED_SHIFT  11

/** 1.0 as fixed-point for load */
#define LOAD_FIXED_1  (1 << LOAD_FIXED_SHIFT)

/** Compute load in 5 second intervals */
#define LOAD_INTERVAL  5

/**
 * Fixed-point representation of:
 *
 * 1 / exp(5 sec / 1 min)
 * 1 / exp(5 sec / 5 min)
 * 1 / exp(5 sec / 15 min)
 *
 */
static load_t load_exp[LOAD_STEPS] = {1884, 2014, 2037};
static load_t avenrdy[LOAD_STEPS] = {0, 0, 0};
SPINLOCK_STATIC_INITIALIZE_NAME(load_lock, "load_lock");

static unative_t get_stats_uptime(struct sysinfo_item *item)
{
	/* This doesn't have to be very accurate */
	return uptime->seconds1;
}

static void *get_stats_cpus(struct sysinfo_item *item, size_t *size)
{
	stats_cpu_t *stats_cpus =
	    (stats_cpu_t *) malloc(sizeof(stats_cpu_t) * config.cpu_count,
	    FRAME_ATOMIC);
	if (stats_cpus == NULL) {
		*size = 0;
		return NULL;
	}
	
	ipl_t ipl = interrupts_disable();
	
	size_t i;
	for (i = 0; i < config.cpu_count; i++) {
		spinlock_lock(&cpus[i].lock);
		
		stats_cpus[i].id = cpus[i].id;
		stats_cpus[i].frequency_mhz = cpus[i].frequency_mhz;
		stats_cpus[i].busy_ticks = cpus[i].busy_ticks;
		stats_cpus[i].idle_ticks = cpus[i].idle_ticks;
		
		spinlock_unlock(&cpus[i].lock);
	}
	
	interrupts_restore(ipl);
	
	*size = sizeof(stats_cpu_t) * config.cpu_count;
	return ((void *) stats_cpus);
}

static bool task_count_walker(avltree_node_t *node, void *arg)
{
	size_t *count = (size_t *) arg;
	(*count)++;
	
	return true;
}

static bool task_serialize_walker(avltree_node_t *node, void *arg)
{
	task_id_t **ids = (task_id_t **) arg;
	task_t *task = avltree_get_instance(node, task_t, tasks_tree_node);
	
	spinlock_lock(&(task->lock));
	
	**ids = task->taskid;
	(*ids)++;
	
	spinlock_unlock(&(task->lock));
	
	return true;
}

static void *get_stats_tasks(struct sysinfo_item *item, size_t *size)
{
	/* Messing with task structures, avoid deadlock */
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	size_t count = 0;
	avltree_walk(&tasks_tree, task_count_walker, (void *) &count);
	
	if (count == 0) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		
		*size = 0;
		return NULL;
	}
	
	task_id_t *task_ids =
	    (task_id_t *) malloc(sizeof(task_id_t) * count, FRAME_ATOMIC);
	if (task_ids == NULL) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		
		*size = 0;
		return NULL;
	}
	
	task_id_t *iterator = task_ids;
	avltree_walk(&tasks_tree, task_serialize_walker, (void *) &iterator);
	
	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
	
	*size = sizeof(task_id_t) * count;
	return ((void *) task_ids);
}

static size_t get_task_virtmem(as_t *as)
{
	mutex_lock(&as->lock);
	
	size_t result = 0;
	
	link_t *cur;
	for (cur = as->as_area_btree.leaf_head.next;
	    cur != &as->as_area_btree.leaf_head; cur = cur->next) {
		btree_node_t *node =
		    list_get_instance(cur, btree_node_t, leaf_link);
		
		unsigned int i;
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];
			
			mutex_lock(&area->lock);
			result += area->pages;
			mutex_unlock(&area->lock);
		}
	}
	
	mutex_unlock(&as->lock);
	
	return result * PAGE_SIZE;
}

static sysinfo_return_t get_stats_task(const char *name)
{
	
	sysinfo_return_t ret;
	ret.tag = SYSINFO_VAL_UNDEFINED;
	
	task_id_t task_id;
	if (str_uint64(name, NULL, 0, true, &task_id) != EOK)
		return ret;
	
	stats_task_t *stats_task =
	    (stats_task_t *) malloc(sizeof(stats_task_t), FRAME_ATOMIC);
	if (stats_task == NULL)
		return ret;
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	
	task_t *task = task_find_by_id(task_id);
	if (task == NULL) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		free(stats_task);
		return ret;
	}
	
	spinlock_lock(&task->lock);
	spinlock_unlock(&tasks_lock);
	
	str_cpy(stats_task->name, TASK_NAME_BUFLEN, task->name);
	stats_task->virtmem = get_task_virtmem(task->as);
	stats_task->threads = atomic_get(&task->refcount);
	task_get_accounting(task, &(stats_task->ucycles),
	    &(stats_task->kcycles));
	stats_task->ipc_info = task->ipc_info;
	
	spinlock_unlock(&task->lock);
	interrupts_restore(ipl);
	
	ret.tag = SYSINFO_VAL_FUNCTION_DATA;
	ret.data.data = (void *) stats_task;
	ret.data.size = sizeof(stats_task_t);
	
	return ret;
}

static void *get_stats_physmem(struct sysinfo_item *item, size_t *size)
{
	stats_physmem_t *stats_physmem =
	    (stats_physmem_t *) malloc(sizeof(stats_physmem_t), FRAME_ATOMIC);
	if (stats_physmem == NULL) {
		*size = 0;
		return NULL;
	}
	
	zones_stats(&(stats_physmem->total), &(stats_physmem->unavail),
	    &(stats_physmem->used), &(stats_physmem->free));
	
	*size = sizeof(stats_physmem_t);
	return ((void *) stats_physmem);
}

static void *get_stats_load(struct sysinfo_item *item, size_t *size)
{
	load_t *stats_load =
	    (load_t *) malloc(sizeof(load_t) * LOAD_STEPS, FRAME_ATOMIC);
	if (stats_load == NULL) {
		*size = 0;
		return NULL;
	}
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&load_lock);
	
	unsigned int i;
	for (i = 0; i < LOAD_STEPS; i++)
		stats_load[i] = avenrdy[i] << LOAD_FIXED_SHIFT;
	
	spinlock_unlock(&load_lock);
	interrupts_restore(ipl);
	
	*size = sizeof(load_t) * LOAD_STEPS;
	return ((void *) stats_load);
}

/** Calculate load
 *
 */
static inline load_t load_calc(load_t load, load_t exp, size_t ready)
{
	load *= exp;
	load += ready * (LOAD_FIXED_1 - exp);
	
	return (load >> LOAD_FIXED_SHIFT);
}

/** Count threads in ready queues
 *
 * Should be called with interrupts disabled.
 *
 */
static inline size_t get_ready_count(void)
{
	size_t i;
	size_t count = 0;
	
	for (i = 0; i < config.cpu_count; i++) {
		spinlock_lock(&cpus[i].lock);
		
		size_t j;
		for (j = 0; j < RQ_COUNT; j++) {
			spinlock_lock(&cpus[i].rq[j].lock);
			count += cpus[i].rq[j].n;
			spinlock_unlock(&cpus[i].rq[j].lock);
		}
		
		spinlock_unlock(&cpus[i].lock);
	}
	
	return count;
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
		ipl_t ipl = interrupts_disable();
		spinlock_lock(&load_lock);
		
		size_t ready = get_ready_count() * LOAD_FIXED_1;
		
		unsigned int i;
		for (i = 0; i < LOAD_STEPS; i++)
			avenrdy[i] = load_calc(avenrdy[i], load_exp[i], ready);
		
		spinlock_unlock(&load_lock);
		interrupts_restore(ipl);
		
		thread_sleep(LOAD_INTERVAL);
	}
}

void stats_init(void)
{
	sysinfo_set_item_fn_val("system.uptime", NULL, get_stats_uptime);
	sysinfo_set_item_fn_data("system.cpus", NULL, get_stats_cpus);
	sysinfo_set_item_fn_data("system.physmem", NULL, get_stats_physmem);
	sysinfo_set_item_fn_data("system.load", NULL, get_stats_load);
	sysinfo_set_item_fn_data("system.tasks", NULL, get_stats_tasks);
	sysinfo_set_subtree_fn("system.tasks", NULL, get_stats_task);
}

/** @}
 */
