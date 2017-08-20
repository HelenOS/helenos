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

#include <kobject/kobject.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <abi/errno.h>
#include <mm/slab.h>

void kobject_initialize(kobject_t *kobj)
{
	kobj->type = KOBJECT_TYPE_INVALID;
	kobj->can_reclaim = NULL;
}

void kobject_task_alloc(task_t *task)
{
	task->kobject = malloc(sizeof(kobject_t) * MAX_KERNEL_OBJECTS, 0);
}

void kobject_task_init(task_t *task)
{
	for (int cap = 0; cap < MAX_KERNEL_OBJECTS; cap++)
		kobject_initialize(&task->kobject[cap]);
}

void kobject_task_free(task_t *task)
{
	free(task->kobject);
}

kobject_t *kobject_get(task_t *task, int cap, kobject_type_t type)
{
	if ((cap < 0) || (cap >= MAX_KERNEL_OBJECTS))
		return NULL;
	if (task->kobject[cap].type != type)
		return NULL;
	return &task->kobject[cap];
}

kobject_t *kobject_get_current(int cap, kobject_type_t type)
{
	return kobject_get(TASK, cap, type);
}

int kobject_alloc(task_t *task)
{
	int cap;

	irq_spinlock_lock(&task->lock, true);
	for (cap = 0; cap < MAX_KERNEL_OBJECTS; cap++) {
		kobject_t *kobj = &task->kobject[cap];
		if (kobj->type > KOBJECT_TYPE_ALLOCATED) {
			if (kobj->can_reclaim && kobj->can_reclaim(kobj))
				kobject_initialize(kobj);
		}
		if (kobj->type == KOBJECT_TYPE_INVALID) {
			kobj->type = KOBJECT_TYPE_ALLOCATED;
			irq_spinlock_unlock(&task->lock, true);
			return cap;
		}
	}
	irq_spinlock_unlock(&task->lock, true);

	return ELIMIT;
}

void kobject_free(task_t *task, int cap)
{
	assert(cap >= 0);
	assert(cap < MAX_KERNEL_OBJECTS);
	assert(task->kobject[cap].type != KOBJECT_TYPE_INVALID);

	irq_spinlock_lock(&task->lock, true);
	kobject_initialize(&task->kobject[cap]);
	irq_spinlock_unlock(&task->lock, true);
}

int kobject_to_cap(task_t *task, kobject_t *kobj)
{
	return kobj - task->kobject;
}

/** @}
 */
