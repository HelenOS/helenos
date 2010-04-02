/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup genericps
 * @{
 */

/**
 * @file
 * @brief	Process listing.
 */

#include <proc/task.h>
#include <proc/thread.h>
#include <ps/ps.h>
#include <ps/taskinfo.h>
#include <adt/avl.h>
#include <synch/waitq.h>
#include <syscall/copy.h>
#include <atomic.h>

static size_t count;
static size_t max_count;
static task_t *selected_task;

#define WRITE_TASK_ID(dst, i, src) copy_to_uspace(dst + i, src, sizeof(task_id_t))
#define WRITE_THREAD_INFO(dst, i, src) copy_to_uspace(dst+i, src, sizeof(thread_info_t))

static bool task_walker(avltree_node_t *node, void *arg)
{
	task_t *t = avltree_get_instance(node, task_t, tasks_tree_node);
	task_id_t *ids = (task_id_t *)arg;

	spinlock_lock(&t->lock);

	++count;
	if (count > max_count) {
		spinlock_unlock(&t->lock);
		return false;
	}

	WRITE_TASK_ID(ids, count - 1, &t->taskid);

	spinlock_unlock(&t->lock);
	return true;
}

size_t sys_ps_get_tasks(task_id_t *uspace_ids, size_t size)
{
	ipl_t ipl;
	
	/* Messing with task structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	count = 0;
	max_count = size / sizeof(task_id_t);
	avltree_walk(&tasks_tree, task_walker, uspace_ids);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
	
	return count;
}

static size_t get_pages_count(as_t *as)
{
	mutex_lock(&as->lock);

	size_t result = 0;
	
	link_t *cur;
	for (cur = as->as_area_btree.leaf_head.next;
	    cur != &as->as_area_btree.leaf_head; cur = cur->next) {
		btree_node_t *node;
		
		node = list_get_instance(cur, btree_node_t, leaf_link);
		
		unsigned int i;
		for (i = 0; i < node->keys; i++) {
			as_area_t *area = node->value[i];
		
			mutex_lock(&area->lock);
			result += area->pages;
			mutex_unlock(&area->lock);
		}
	}
	
	mutex_unlock(&as->lock);
	
	return result;
}

int sys_ps_get_task_info(task_id_t *uspace_id, task_info_t *uspace_info)
{
	ipl_t ipl;
	ipl = interrupts_disable();

	task_id_t id;
	copy_from_uspace(&id, uspace_id, sizeof(task_id_t));

	spinlock_lock(&tasks_lock);
	task_t *t = task_find_by_id(id);
	spinlock_lock(&t->lock);
	spinlock_unlock(&tasks_lock);

	copy_to_uspace(&uspace_info->taskid, &t->taskid, sizeof(task_id_t));
	copy_to_uspace(uspace_info->name, t->name, sizeof(t->name));

	uint64_t ucycles;
	uint64_t kcycles;
	uint64_t cycles = task_get_accounting(t, &ucycles, &kcycles);
	copy_to_uspace(&uspace_info->cycles, &cycles, sizeof(cycles));
	copy_to_uspace(&uspace_info->ucycles, &ucycles, sizeof(cycles));
	copy_to_uspace(&uspace_info->kcycles, &kcycles, sizeof(cycles));

	size_t pages = get_pages_count(t->as);
	copy_to_uspace(&uspace_info->pages, &pages, sizeof(pages));

	int thread_count = atomic_get(&t->refcount);
	copy_to_uspace(&uspace_info->thread_count, &thread_count, sizeof(thread_count));
	
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
	return 0;
}

static bool thread_walker(avltree_node_t *node, void *arg)
{
	thread_t *t = avltree_get_instance(node, thread_t, threads_tree_node);
	thread_info_t *infos = (thread_info_t *)arg;
	thread_info_t result;

	spinlock_lock(&t->lock);

	if (t->task != selected_task) {
		spinlock_unlock(&t->lock);
		return true;
	}

	++count;
	if (count > max_count) {
		spinlock_unlock(&t->lock);
		return false;
	}
	
	result.tid = t->tid;
	result.state = t->state;
	result.priority = t->priority;
	result.cycles = t->cycles;
	result.ucycles = t->ucycles;
	result.kcycles = t->kcycles;

	if (t->cpu)
		result.cpu = t->cpu->id;
	else
		result.cpu = -1;

	WRITE_THREAD_INFO(infos, count - 1, &result);

	spinlock_unlock(&t->lock);
	return true;
}

int sys_ps_get_threads(task_id_t *uspace_id, thread_info_t *uspace_infos, size_t size)
{
	ipl_t ipl;
	ipl = interrupts_disable();

	task_id_t id;
	copy_from_uspace(&id, uspace_id, sizeof(task_id_t));
	spinlock_lock(&tasks_lock);
	selected_task = task_find_by_id(id);
	spinlock_unlock(&tasks_lock);

	if (!selected_task) {
		return 0;
	}

	spinlock_lock(&threads_lock);

	count = 0;
	max_count = size / sizeof(thread_info_t);
	avltree_walk(&threads_tree, thread_walker, uspace_infos);

	spinlock_unlock(&threads_lock);
	interrupts_restore(ipl);
	return count;
}

/** @}
 */
