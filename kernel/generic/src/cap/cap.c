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
#include <synch/spinlock.h>
#include <abi/errno.h>
#include <mm/slab.h>

void cap_initialize(cap_t *cap, int handle)
{
	cap->type = CAP_TYPE_INVALID;
	cap->handle = handle;
	cap->can_reclaim = NULL;
}

void caps_task_alloc(task_t *task)
{
	task->caps = malloc(sizeof(cap_t) * MAX_CAPS, 0);
}

void caps_task_init(task_t *task)
{
	for (int i = 0; i < MAX_CAPS; i++)
		cap_initialize(&task->caps[i], i);
}

void caps_task_free(task_t *task)
{
	free(task->caps);
}

cap_t *cap_get(task_t *task, int handle, cap_type_t type)
{
	if ((handle < 0) || (handle >= MAX_CAPS))
		return NULL;
	if (task->caps[handle].type != type)
		return NULL;
	return &task->caps[handle];
}

cap_t *cap_get_current(int handle, cap_type_t type)
{
	return cap_get(TASK, handle, type);
}

int cap_alloc(task_t *task)
{
	int handle;

	irq_spinlock_lock(&task->lock, true);
	for (handle = 0; handle < MAX_CAPS; handle++) {
		cap_t *cap = &task->caps[handle];
		if (cap->type > CAP_TYPE_ALLOCATED) {
			if (cap->can_reclaim && cap->can_reclaim(cap))
				cap_initialize(cap, handle);
		}
		if (cap->type == CAP_TYPE_INVALID) {
			cap->type = CAP_TYPE_ALLOCATED;
			irq_spinlock_unlock(&task->lock, true);
			return handle;
		}
	}
	irq_spinlock_unlock(&task->lock, true);

	return ELIMIT;
}

void cap_free(task_t *task, int handle)
{
	assert(handle >= 0);
	assert(handle < MAX_CAPS);
	assert(task->caps[handle].type != CAP_TYPE_INVALID);

	irq_spinlock_lock(&task->lock, true);
	cap_initialize(&task->caps[handle], handle);
	irq_spinlock_unlock(&task->lock, true);
}

/** @}
 */
