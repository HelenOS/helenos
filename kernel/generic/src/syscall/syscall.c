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
 * @brief Syscall table and syscall wrappers.
 */

#include <syscall/syscall.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <proc/program.h>
#include <mm/as.h>
#include <print.h>
#include <arch.h>
#include <debug.h>
#include <ddi/device.h>
#include <ipc/sysipc.h>
#include <synch/futex.h>
#include <synch/smc.h>
#include <ddi/ddi.h>
#include <ipc/event.h>
#include <security/cap.h>
#include <sysinfo/sysinfo.h>
#include <console/console.h>
#include <udebug/udebug.h>
#include <ps/ps.h>
#include <ps/load.h>
#include <ps/uptime.h>

/** Dispatch system call */
unative_t syscall_handler(unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5, unative_t a6, unative_t id)
{
	unative_t rc;

	/* Do userpace accounting */
	thread_update_accounting(true);

#ifdef CONFIG_UDEBUG
	/*
	 * Early check for undebugged tasks. We do not lock anything as this
	 * test need not be precise in either direction.
	 */
	if (THREAD->udebug.active) {
		udebug_syscall_event(a1, a2, a3, a4, a5, a6, id, 0, false);
	}
#endif
	
	if (id < SYSCALL_END) {
		rc = syscall_table[id](a1, a2, a3, a4, a5, a6);
	} else {
		printf("Task %" PRIu64": Unknown syscall %#" PRIxn, TASK->taskid, id);
		task_kill(TASK->taskid);
		thread_exit();
	}
	
	if (THREAD->interrupted)
		thread_exit();
	
#ifdef CONFIG_UDEBUG
	if (THREAD->udebug.active) {
		udebug_syscall_event(a1, a2, a3, a4, a5, a6, id, rc, true);
	
		/*
		 * Stopping point needed for tasks that only invoke
		 * non-blocking system calls. Not needed if the task
		 * is not being debugged (it cannot block here).
		 */
		udebug_stoppable_begin();
		udebug_stoppable_end();
	}
#endif

	/* Do kernel accounting */
	thread_update_accounting(false);
	
	return rc;
}

syshandler_t syscall_table[SYSCALL_END] = {
	(syshandler_t) sys_klog,
	(syshandler_t) sys_tls_set,
	
	/* Thread and task related syscalls. */
	(syshandler_t) sys_thread_create,
	(syshandler_t) sys_thread_exit,
	(syshandler_t) sys_thread_get_id,
	(syshandler_t) sys_thread_usleep,
	
	(syshandler_t) sys_task_get_id,
	(syshandler_t) sys_task_set_name,
	(syshandler_t) sys_program_spawn_loader,
	
	/* Synchronization related syscalls. */
	(syshandler_t) sys_futex_sleep,
	(syshandler_t) sys_futex_wakeup,
	(syshandler_t) sys_smc_coherence,
	
	/* Address space related syscalls. */
	(syshandler_t) sys_as_area_create,
	(syshandler_t) sys_as_area_resize,
	(syshandler_t) sys_as_area_change_flags,
	(syshandler_t) sys_as_area_destroy,
	
	/* IPC related syscalls. */
	(syshandler_t) sys_ipc_call_sync_fast,
	(syshandler_t) sys_ipc_call_sync_slow,
	(syshandler_t) sys_ipc_call_async_fast,
	(syshandler_t) sys_ipc_call_async_slow,
	(syshandler_t) sys_ipc_answer_fast,
	(syshandler_t) sys_ipc_answer_slow,
	(syshandler_t) sys_ipc_forward_fast,
	(syshandler_t) sys_ipc_forward_slow,
	(syshandler_t) sys_ipc_wait_for_call,
	(syshandler_t) sys_ipc_poke,
	(syshandler_t) sys_ipc_hangup,
	(syshandler_t) sys_ipc_register_irq,
	(syshandler_t) sys_ipc_unregister_irq,

	/* Event notification syscalls. */
	(syshandler_t) sys_event_subscribe,
	
	/* Capabilities related syscalls. */
	(syshandler_t) sys_cap_grant,
	(syshandler_t) sys_cap_revoke,
	
	/* DDI related syscalls. */
	(syshandler_t) sys_device_assign_devno,
	(syshandler_t) sys_physmem_map,
	(syshandler_t) sys_iospace_enable,
	(syshandler_t) sys_preempt_control,
	
	/* Sysinfo syscalls */
	(syshandler_t) sys_sysinfo_get_tag,
	(syshandler_t) sys_sysinfo_get_value,
	(syshandler_t) sys_sysinfo_get_data_size,
	(syshandler_t) sys_sysinfo_get_data,
	
	/* Debug calls */
	(syshandler_t) sys_debug_enable_console,
	(syshandler_t) sys_debug_disable_console,

	/* Ps calls */
	(syshandler_t) sys_ps_get_cpu_info,
	(syshandler_t) sys_ps_get_mem_info,
	(syshandler_t) sys_ps_get_tasks,
	(syshandler_t) sys_ps_get_task_info,
	(syshandler_t) sys_ps_get_threads,
	(syshandler_t) sys_ps_get_uptime,
	(syshandler_t) sys_ps_get_load,
	
	(syshandler_t) sys_ipc_connect_kbox
};

/** @}
 */
