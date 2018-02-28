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
 * @file perm.c
 * @brief Task permissions control.
 *
 * @see perm.h
 */

#include <security/perm.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>
#include <arch.h>
#include <errno.h>

/** Set permissions.
 *
 * @param task  Task whose permissions are to be changed.
 * @param perms New set of permissions.
 *
 */
void perm_set(task_t *task, perm_t perms)
{
	irq_spinlock_lock(&task->lock, true);
	task->perms = perms;
	irq_spinlock_unlock(&task->lock, true);
}

/** Get permissions.
 *
 * @param task Task whose permissions are to be returned.
 *
 * @return Task's permissions.
 *
 */
perm_t perm_get(task_t *task)
{
	irq_spinlock_lock(&task->lock, true);
	perm_t perms = task->perms;
	irq_spinlock_unlock(&task->lock, true);

	return perms;
}

/** Grant permissions to a task.
 *
 * The calling task must have the PERM_PERM permission.
 *
 * @param taskid Destination task ID.
 * @param perms   Permissions to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
static errno_t perm_grant(task_id_t taskid, perm_t perms)
{
	if (!(perm_get(TASK) & PERM_PERM))
		return EPERM;

	irq_spinlock_lock(&tasks_lock, true);
	task_t *task = task_find_by_id(taskid);

	if ((!task) || (!container_check(CONTAINER, task->container))) {
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}

	irq_spinlock_lock(&task->lock, false);
	task->perms |= perms;
	irq_spinlock_unlock(&task->lock, false);

	irq_spinlock_unlock(&tasks_lock, true);
	return EOK;
}

/** Revoke permissions from a task.
 *
 * The calling task must have the PERM_PERM permission or the caller must
 * attempt to revoke permissions from itself.
 *
 * @param taskid Destination task ID.
 * @param perms   Permissions to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
static errno_t perm_revoke(task_id_t taskid, perm_t perms)
{
	irq_spinlock_lock(&tasks_lock, true);

	task_t *task = task_find_by_id(taskid);
	if ((!task) || (!container_check(CONTAINER, task->container))) {
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}

	/*
	 * Revoking permissions is different from granting them in that
	 * a task can revoke permissions from itself even if it
	 * doesn't have PERM_PERM.
	 */
	irq_spinlock_unlock(&TASK->lock, false);

	if ((!(TASK->perms & PERM_PERM)) || (task != TASK)) {
		irq_spinlock_unlock(&TASK->lock, false);
		irq_spinlock_unlock(&tasks_lock, true);
		return EPERM;
	}

	task->perms &= ~perms;
	irq_spinlock_unlock(&TASK->lock, false);

	irq_spinlock_unlock(&tasks_lock, true);
	return EOK;
}

#ifdef __32_BITS__

/** Grant permissions to a task (32 bits)
 *
 * The calling task must have the PERM_PERM permission.
 *
 * @param uspace_taskid User-space pointer to destination task ID.
 * @param perms         Permissions to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_perm_grant(sysarg64_t *uspace_taskid, perm_t perms)
{
	sysarg64_t taskid;
	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(sysarg64_t));
	if (rc != EOK)
		return (sys_errno_t) rc;

	return perm_grant((task_id_t) taskid, perms);
}

/** Revoke permissions from a task (32 bits)
 *
 * The calling task must have the PERM_PERM permission or the caller must
 * attempt to revoke permissions from itself.
 *
 * @param uspace_taskid User-space pointer to destination task ID.
 * @param perms         Perms to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_perm_revoke(sysarg64_t *uspace_taskid, perm_t perms)
{
	sysarg64_t taskid;
	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(sysarg64_t));
	if (rc != EOK)
		return (sys_errno_t) rc;

	return perm_revoke((task_id_t) taskid, perms);
}

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

/** Grant permissions to a task (64 bits)
 *
 * The calling task must have the PERM_PERM permission.
 *
 * @param taskid Destination task ID.
 * @param perms  Permissions to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_perm_grant(sysarg_t taskid, perm_t perms)
{
	return perm_grant((task_id_t) taskid, perms);
}

/** Revoke permissions from a task (64 bits)
 *
 * The calling task must have the PERM_PERM permission or the caller must
 * attempt to revoke permissions from itself.
 *
 * @param taskid Destination task ID.
 * @param perms  Permissions to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_perm_revoke(sysarg_t taskid, perm_t perms)
{
	return perm_revoke((task_id_t) taskid, perms);
}

#endif  /* __64_BITS__ */

/** @}
 */
