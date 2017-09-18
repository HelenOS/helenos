/*
 * Copyright (c) 2017 Jakub Jermar
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

#include <cap/cap.h>
#include <proc/task.h>
#include <synch/mutex.h>
#include <abi/errno.h>
#include <mm/slab.h>
#include <adt/list.h>

void cap_initialize(cap_t *cap, int handle)
{
	cap->type = CAP_TYPE_INVALID;
	cap->handle = handle;
	cap->can_reclaim = NULL;
	link_initialize(&cap->link);
}

void caps_task_alloc(task_t *task)
{
	task->cap_info = (cap_info_t *) malloc(sizeof(cap_info_t), 0);
	task->cap_info->caps = malloc(sizeof(cap_t) * MAX_CAPS, 0);
}

void caps_task_init(task_t *task)
{
	mutex_initialize(&task->cap_info->lock, MUTEX_PASSIVE);

	for (int i = 0; i < CAP_TYPE_MAX; i++)
		list_initialize(&task->cap_info->type_list[i]);

	for (int i = 0; i < MAX_CAPS; i++)
		cap_initialize(&task->cap_info->caps[i], i);
}

void caps_task_free(task_t *task)
{
	free(task->cap_info->caps);
	free(task->cap_info);
}

bool caps_apply_to_all(task_t *task, cap_type_t type,
    bool (*cb)(cap_t *, void *), void *arg)
{
	bool done = true;

	mutex_lock(&task->cap_info->lock);
	list_foreach_safe(task->cap_info->type_list[type], cur, next) {
		cap_t *cap = list_get_instance(cur, cap_t, link);
		done = cb(cap, arg);
		if (!done)
			break;
	}
	mutex_unlock(&task->cap_info->lock);

	return done;
}

void caps_lock(task_t *task)
{
	mutex_lock(&task->cap_info->lock);
}

void caps_unlock(task_t *task)
{
	mutex_unlock(&task->cap_info->lock);
}

cap_t *cap_get(task_t *task, int handle, cap_type_t type)
{
	assert(mutex_locked(&task->cap_info->lock));

	if ((handle < 0) || (handle >= MAX_CAPS))
		return NULL;
	if (task->cap_info->caps[handle].type != type)
		return NULL;
	return &task->cap_info->caps[handle];
}

int cap_alloc(task_t *task)
{
	int handle;

	mutex_lock(&task->cap_info->lock);
	for (handle = 0; handle < MAX_CAPS; handle++) {
		cap_t *cap = &task->cap_info->caps[handle];
		if (cap->type > CAP_TYPE_ALLOCATED) {
			if (cap->can_reclaim && cap->can_reclaim(cap))
				cap_initialize(cap, handle);
		}
		if (cap->type == CAP_TYPE_INVALID) {
			cap->type = CAP_TYPE_ALLOCATED;
			mutex_unlock(&task->cap_info->lock);
			return handle;
		}
	}
	mutex_unlock(&task->cap_info->lock);

	return ELIMIT;
}

void cap_publish(task_t *task, int handle, cap_type_t type, void *kobject)
{
	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_TYPE_ALLOCATED);
	assert(cap);
	cap->type = type;
	cap->kobject = kobject;
	list_append(&cap->link, &task->cap_info->type_list[type]);
	mutex_unlock(&task->cap_info->lock);
}

cap_t *cap_unpublish(task_t *task, int handle, cap_type_t type)
{
	cap_t *cap;

	mutex_lock(&task->cap_info->lock);
	cap = cap_get(task, handle, type);
	if (cap) {
		list_remove(&cap->link);
		cap->type = CAP_TYPE_ALLOCATED;
	}
	mutex_unlock(&task->cap_info->lock);

	return cap;
}

void cap_free(task_t *task, int handle)
{
	assert(handle >= 0);
	assert(handle < MAX_CAPS);
	assert(task->cap_info->caps[handle].type == CAP_TYPE_ALLOCATED);

	mutex_lock(&task->cap_info->lock);
	cap_initialize(&task->cap_info->caps[handle], handle);
	mutex_unlock(&task->cap_info->lock);
}

/** @}
 */
