/*
 * Copyright (c) 2006 Jakub Jermar
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

/**
 * @file cap.c
 * @brief Capabilities control.
 *
 * @see cap.h
 */

#include <security/cap.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>
#include <arch.h>
#include <errno.h>

/** Set capabilities.
 *
 * @param task Task whose capabilities are to be changed.
 * @param caps New set of capabilities.
 *
 */
void cap_set(task_t *task, cap_t caps)
{
	irq_spinlock_lock(&task->lock, true);
	task->capabilities = caps;
	irq_spinlock_unlock(&task->lock, true);
}

/** Get capabilities.
 *
 * @param task Task whose capabilities are to be returned.
 *
 * @return Task's capabilities.
 *
 */
cap_t cap_get(task_t *task)
{
	irq_spinlock_lock(&task->lock, true);
	cap_t caps = task->capabilities;
	irq_spinlock_unlock(&task->lock, true);
	
	return caps;
}

/** Grant capabilities to a task.
 *
 * The calling task must have the CAP_CAP capability.
 *
 * @param taskid Destination task ID.
 * @param caps   Capabilities to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
static sysarg_t cap_grant(task_id_t taskid, cap_t caps)
{
	if (!(cap_get(TASK) & CAP_CAP))
		return (sysarg_t) EPERM;
	
	irq_spinlock_lock(&tasks_lock, true);
	task_t *task = task_find_by_id(taskid);
	
	if ((!task) || (!container_check(CONTAINER, task->container))) {
		irq_spinlock_unlock(&tasks_lock, true);
		return (sysarg_t) ENOENT;
	}
	
	irq_spinlock_lock(&task->lock, false);
	task->capabilities |= caps;
	irq_spinlock_unlock(&task->lock, false);
	
	irq_spinlock_unlock(&tasks_lock, true);
	return 0;
}

/** Revoke capabilities from a task.
 *
 * The calling task must have the CAP_CAP capability or the caller must
 * attempt to revoke capabilities from itself.
 *
 * @param taskid Destination task ID.
 * @param caps   Capabilities to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
static sysarg_t cap_revoke(task_id_t taskid, cap_t caps)
{
	irq_spinlock_lock(&tasks_lock, true);
	
	task_t *task = task_find_by_id(taskid);
	if ((!task) || (!container_check(CONTAINER, task->container))) {
		irq_spinlock_unlock(&tasks_lock, true);
		return (sysarg_t) ENOENT;
	}
	
	/*
	 * Revoking capabilities is different from granting them in that
	 * a task can revoke capabilities from itself even if it
	 * doesn't have CAP_CAP.
	 */
	irq_spinlock_unlock(&TASK->lock, false);
	
	if ((!(TASK->capabilities & CAP_CAP)) || (task != TASK)) {
		irq_spinlock_unlock(&TASK->lock, false);
		irq_spinlock_unlock(&tasks_lock, true);
		return (sysarg_t) EPERM;
	}
	
	task->capabilities &= ~caps;
	irq_spinlock_unlock(&TASK->lock, false);
	
	irq_spinlock_unlock(&tasks_lock, true);
	return 0;
}

#ifdef __32_BITS__

/** Grant capabilities to a task (32 bits)
 *
 * The calling task must have the CAP_CAP capability.
 *
 * @param uspace_taskid User-space pointer to destination task ID.
 * @param caps          Capabilities to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sysarg_t sys_cap_grant(sysarg64_t *uspace_taskid, cap_t caps)
{
	sysarg64_t taskid;
	int rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(sysarg64_t));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return cap_grant((task_id_t) taskid, caps);
}

/** Revoke capabilities from a task (32 bits)
 *
 * The calling task must have the CAP_CAP capability or the caller must
 * attempt to revoke capabilities from itself.
 *
 * @param uspace_taskid User-space pointer to destination task ID.
 * @param caps          Capabilities to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sysarg_t sys_cap_revoke(sysarg64_t *uspace_taskid, cap_t caps)
{
	sysarg64_t taskid;
	int rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(sysarg64_t));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return cap_revoke((task_id_t) taskid, caps);
}

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

/** Grant capabilities to a task (64 bits)
 *
 * The calling task must have the CAP_CAP capability.
 *
 * @param taskid Destination task ID.
 * @param caps   Capabilities to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sysarg_t sys_cap_grant(sysarg_t taskid, cap_t caps)
{
	return cap_grant((task_id_t) taskid, caps);
}

/** Revoke capabilities from a task (64 bits)
 *
 * The calling task must have the CAP_CAP capability or the caller must
 * attempt to revoke capabilities from itself.
 *
 * @param taskid Destination task ID.
 * @param caps   Capabilities to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sysarg_t sys_cap_revoke(sysarg_t taskid, cap_t caps)
{
	return cap_revoke((task_id_t) taskid, caps);
}

#endif  /* __64_BITS__ */

/** @}
 */
