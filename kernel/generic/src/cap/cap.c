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

static kobject_t *cap_unpublish_locked(task_t *, cap_handle_t, kobject_type_t);

void cap_initialize(cap_t *cap, cap_handle_t handle)
{
	cap->state = CAP_STATE_FREE;
	cap->handle = handle;
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

	for (kobject_type_t t = 0; t < KOBJECT_TYPE_MAX; t++)
		list_initialize(&task->cap_info->type_list[t]);

	for (cap_handle_t h = 0; h < MAX_CAPS; h++)
		cap_initialize(&task->cap_info->caps[h], h);
}

void caps_task_free(task_t *task)
{
	free(task->cap_info->caps);
	free(task->cap_info);
}

bool caps_apply_to_kobject_type(task_t *task, kobject_type_t type,
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

static cap_t *cap_get(task_t *task, cap_handle_t handle, cap_state_t state)
{
	assert(mutex_locked(&task->cap_info->lock));

	if ((handle < 0) || (handle >= MAX_CAPS))
		return NULL;
	if (task->cap_info->caps[handle].state != state)
		return NULL;
	return &task->cap_info->caps[handle];
}

cap_handle_t cap_alloc(task_t *task)
{
	mutex_lock(&task->cap_info->lock);
	for (cap_handle_t handle = 0; handle < MAX_CAPS; handle++) {
		cap_t *cap = &task->cap_info->caps[handle];
		/* See if the capability should be garbage-collected */
		if (cap->state == CAP_STATE_PUBLISHED &&
		    cap->kobject->ops->reclaim &&
		    cap->kobject->ops->reclaim(cap->kobject)) {
			kobject_t *kobj = cap_unpublish_locked(task, handle,
			    cap->kobject->type);
			kobject_put(kobj);
			cap_initialize(&task->cap_info->caps[handle], handle);
		}
		if (cap->state == CAP_STATE_FREE) {
			cap->state = CAP_STATE_ALLOCATED;
			mutex_unlock(&task->cap_info->lock);
			return handle;
		}
	}
	mutex_unlock(&task->cap_info->lock);

	return ELIMIT;
}

void
cap_publish(task_t *task, cap_handle_t handle, kobject_t *kobj)
{
	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_ALLOCATED);
	assert(cap);
	cap->state = CAP_STATE_PUBLISHED;
	/* Hand over kobj's reference to cap */
	cap->kobject = kobj;
	list_append(&cap->link, &task->cap_info->type_list[kobj->type]);
	mutex_unlock(&task->cap_info->lock);
}

static kobject_t *
cap_unpublish_locked(task_t *task, cap_handle_t handle, kobject_type_t type)
{
	kobject_t *kobj = NULL;

	cap_t *cap = cap_get(task, handle, CAP_STATE_PUBLISHED);
	if (cap) {
		if (cap->kobject->type == type) {
			/* Hand over cap's reference to kobj */
			kobj = cap->kobject;
			cap->kobject = NULL;
			list_remove(&cap->link);
			cap->state = CAP_STATE_ALLOCATED;
		}
	}

	return kobj;
}
kobject_t *cap_unpublish(task_t *task, cap_handle_t handle, kobject_type_t type)
{

	mutex_lock(&task->cap_info->lock);
	kobject_t *kobj = cap_unpublish_locked(task, handle, type);
	mutex_unlock(&task->cap_info->lock);

	return kobj;
}

void cap_free(task_t *task, cap_handle_t handle)
{
	assert(handle >= 0);
	assert(handle < MAX_CAPS);
	assert(task->cap_info->caps[handle].state == CAP_STATE_ALLOCATED);

	mutex_lock(&task->cap_info->lock);
	cap_initialize(&task->cap_info->caps[handle], handle);
	mutex_unlock(&task->cap_info->lock);
}

void kobject_initialize(kobject_t *kobj, kobject_type_t type, void *raw,
    kobject_ops_t *ops)
{
	atomic_set(&kobj->refcnt, 1);
	kobj->type = type;
	kobj->raw = raw;
	kobj->ops = ops;
}

kobject_t *
kobject_get(struct task *task, cap_handle_t handle, kobject_type_t type)
{
	kobject_t *kobj = NULL;

	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_PUBLISHED);
	if (cap) {
		if (cap->kobject->type == type) {
			kobj = cap->kobject;
			atomic_inc(&kobj->refcnt);
		}
	}
	mutex_unlock(&task->cap_info->lock);

	return kobj;
}

void kobject_put(kobject_t *kobj)
{
	if (atomic_postdec(&kobj->refcnt) == 1) {
		kobj->ops->destroy(kobj->raw);
		free(kobj);
	}
}

/** @}
 */
