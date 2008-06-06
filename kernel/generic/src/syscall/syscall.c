/*
 * Copyright (c) 2005 Martin Decky
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
 * @file
 * @brief	Syscall table and syscall wrappers.
 */
 
#include <syscall/syscall.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <print.h>
#include <putchar.h>
#include <errno.h>
#include <arch.h>
#include <debug.h>
#include <ipc/sysipc.h>
#include <synch/futex.h>
#include <ddi/ddi.h>
#include <security/cap.h>
#include <syscall/copy.h>
#include <sysinfo/sysinfo.h>
#include <console/console.h>

/** Print using kernel facility
 *
 * Print to kernel log.
 *
 */
static unative_t sys_klog(int fd, const void * buf, size_t count) 
{
	size_t i;
	char *data;
	int rc;

	if (count > PAGE_SIZE)
		return ELIMIT;
	
	if (count > 0) {
		data = (char *) malloc(count, 0);
		if (!data)
			return ENOMEM;
		
		rc = copy_from_uspace(data, buf, count);
		if (rc) {
			free(data);
			return rc;
		}
	
		for (i = 0; i < count; i++)
			putchar(data[i]);
		free(data);
	} else
		klog_update();
	
	return count;
}

/** Tell kernel to get keyboard/console access again */
static unative_t sys_debug_enable_console(void)
{
	arch_grab_console();
	return 0;
}

/** Dispatch system call */
unative_t syscall_handler(unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5, unative_t a6, unative_t id)
{
	unative_t rc;

	if (id < SYSCALL_END)
		rc = syscall_table[id](a1, a2, a3, a4, a5, a6);
	else {
		printf("Task %" PRIu64": Unknown syscall %#" PRIxn, TASK->taskid, id);
		task_kill(TASK->taskid);
		thread_exit();
	}
		
	if (THREAD->interrupted)
		thread_exit();
	
	return rc;
}

syshandler_t syscall_table[SYSCALL_END] = {
	(syshandler_t) sys_klog,
	(syshandler_t) sys_tls_set,
	
	/* Thread and task related syscalls. */
	(syshandler_t) sys_thread_create,
	(syshandler_t) sys_thread_exit,
	(syshandler_t) sys_thread_get_id,
	
	(syshandler_t) sys_task_get_id,
	(syshandler_t) sys_task_spawn,
	
	/* Synchronization related syscalls. */
	(syshandler_t) sys_futex_sleep_timeout,
	(syshandler_t) sys_futex_wakeup,
	
	/* Address space related syscalls. */
	(syshandler_t) sys_as_area_create,
	(syshandler_t) sys_as_area_resize,
	(syshandler_t) sys_as_area_destroy,
	
	/* IPC related syscalls. */
	(syshandler_t) sys_ipc_call_sync_fast,
	(syshandler_t) sys_ipc_call_sync_slow,
	(syshandler_t) sys_ipc_call_async_fast,
	(syshandler_t) sys_ipc_call_async_slow,
	(syshandler_t) sys_ipc_answer_fast,
	(syshandler_t) sys_ipc_answer_slow,
	(syshandler_t) sys_ipc_forward_fast,
	(syshandler_t) sys_ipc_wait_for_call,
	(syshandler_t) sys_ipc_hangup,
	(syshandler_t) sys_ipc_register_irq,
	(syshandler_t) sys_ipc_unregister_irq,
	
	/* Capabilities related syscalls. */
	(syshandler_t) sys_cap_grant,
	(syshandler_t) sys_cap_revoke,
	
	/* DDI related syscalls. */
	(syshandler_t) sys_physmem_map,
	(syshandler_t) sys_iospace_enable,
	(syshandler_t) sys_preempt_control,
	
	/* Sysinfo syscalls */
	(syshandler_t) sys_sysinfo_valid,
	(syshandler_t) sys_sysinfo_value,
	
	/* Debug calls */
	(syshandler_t) sys_debug_enable_console
};

/** @}
 */
