/*
 * Copyright (C) 2006 Jakub Jermar
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
 * @file	cap.c
 * @brief	Capabilities control.
 *
 * @see cap.h
 */
 
#include <security/cap.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <syscall/sysarg64.h>
#include <syscall/copy.h>
#include <arch.h>
#include <typedefs.h>
#include <errno.h>

/** Set capabilities.
 *
 * @param t Task whose capabilities are to be changed.
 * @param caps New set of capabilities.
 */
void cap_set(task_t *t, cap_t caps)
{
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&t->lock);
	
	t->capabilities = caps;
	
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
}

/** Get capabilities.
 *
 * @param t Task whose capabilities are to be returned.
 * @return Task's capabilities.
 */
cap_t cap_get(task_t *t)
{
	ipl_t ipl;
	cap_t caps;
	
	ipl = interrupts_disable();
	spinlock_lock(&t->lock);
	
	caps = t->capabilities;
	
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
	
	return caps;
}

/** Grant capabilities to a task.
 *
 * The calling task must have the CAP_CAP capability.
 *
 * @param uspace_taskid_arg Userspace structure holding destination task ID.
 * @param caps Capabilities to grant.
 *
 * @return Zero on success or an error code from @ref errno.h.
 */
unative_t sys_cap_grant(sysarg64_t *uspace_taskid_arg, cap_t caps)
{
	sysarg64_t taskid_arg;
	task_t *t;
	ipl_t ipl;
	int rc;
	
	if (!(cap_get(TASK) & CAP_CAP))
		return (unative_t) EPERM;
	
	rc = copy_from_uspace(&taskid_arg, uspace_taskid_arg, sizeof(sysarg64_t));
	if (rc != 0)
		return (unative_t) rc;
		
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	t = task_find_by_id((task_id_t) taskid_arg.value);
	if (!t) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return (unative_t) ENOENT;
	}
	
	spinlock_lock(&t->lock);
	cap_set(t, cap_get(t) | caps);
	spinlock_unlock(&t->lock);
	
	spinlock_unlock(&tasks_lock);
	

	
	interrupts_restore(ipl);	
	return 0;
}

/** Revoke capabilities from a task.
 *
 * The calling task must have the CAP_CAP capability or the caller must
 * attempt to revoke capabilities from itself.
 *
 * @param uspace_taskid_arg Userspace structure holding destination task ID.
 * @param caps Capabilities to revoke.
 *
 * @return Zero on success or an error code from @ref errno.h.
 */
unative_t sys_cap_revoke(sysarg64_t *uspace_taskid_arg, cap_t caps)
{
	sysarg64_t taskid_arg;
	task_t *t;
	ipl_t ipl;
	int rc;
	
	rc = copy_from_uspace(&taskid_arg, uspace_taskid_arg, sizeof(sysarg64_t));
	if (rc != 0)
		return (unative_t) rc;

	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);	
	t = task_find_by_id((task_id_t) taskid_arg.value);
	if (!t) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return (unative_t) ENOENT;
	}

	/*
	 * Revoking capabilities is different from granting them in that
	 * a task can revoke capabilities from itself even if it
	 * doesn't have CAP_CAP.
	 */
	if (!(cap_get(TASK) & CAP_CAP) || !(t == TASK)) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return (unative_t) EPERM;
	}
	
	spinlock_lock(&t->lock);
	cap_set(t, cap_get(t) & ~caps);
	spinlock_unlock(&t->lock);

	spinlock_unlock(&tasks_lock);

	interrupts_restore(ipl);
	return 0;
}

 /** @}
 */

