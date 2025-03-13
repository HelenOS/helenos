/*
 * Copyright (c) 2025 Jiri Svoboda
 * Copyright (c) 2007 Martin Decky
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

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Shutdown procedures.
 */

#include <arch.h>
#include <errno.h>
#include <halt.h>
#include <log.h>
#include <main/main.h>
#include <main/shutdown.h>
#include <proc/task.h>
#include <proc/thread.h>

static thread_t *reboot_thrd = NULL;
SPINLOCK_INITIALIZE(reboot_lock);

void reboot(void)
{
	task_done(kernel_task);

#ifdef CONFIG_DEBUG
	log(LF_OTHER, LVL_DEBUG, "Rebooting the system");
#endif

	arch_reboot();
	halt();
}

/** Thread procedure for rebooting the system.
 *
 * @param arg Argument (unused)
 */
static void reboot_thrd_proc(void *arg)
{
	(void)arg;

	reboot();
}

/** Reboot the system.
 *
 * @return EOK if reboot started successfully. EBUSY if reboot already
 *         started, ENOMEM if out of memory.
 */
sys_errno_t sys_reboot(void)
{
	thread_t *thread;

	thread = thread_create(reboot_thrd_proc, NULL, kernel_task,
	    THREAD_FLAG_NONE, "reboot");
	if (thread == NULL)
		return ENOMEM;

	spinlock_lock(&reboot_lock);

	if (reboot_thrd != NULL) {
		spinlock_unlock(&reboot_lock);
		thread_put(thread);
		return EBUSY;
	}

	reboot_thrd = thread;

	spinlock_unlock(&reboot_lock);

	thread_start(thread);
	thread_detach(thread);

	return EOK;
}

/** @}
 */
