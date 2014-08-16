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
#include <mm/page.h>
#include <print.h>
#include <arch.h>
#include <debug.h>
#include <ddi/device.h>
#include <interrupt.h>
#include <ipc/sysipc.h>
#include <synch/futex.h>
#include <synch/smc.h>
#include <ddi/ddi.h>
#include <ipc/event.h>
#include <security/cap.h>
#include <sysinfo/sysinfo.h>
#include <console/console.h>
#include <udebug/udebug.h>
#include <log.h>

/** Dispatch system call */
sysarg_t syscall_handler(sysarg_t a1, sysarg_t a2, sysarg_t a3,
    sysarg_t a4, sysarg_t a5, sysarg_t a6, sysarg_t id)
{
	/* Do userpace accounting */
	irq_spinlock_lock(&THREAD->lock, true);
	thread_update_accounting(true);
	irq_spinlock_unlock(&THREAD->lock, true);
	
#ifdef CONFIG_UDEBUG
	/*
	 * An istate_t-compatible record was created on the stack by the
	 * low-level syscall handler. This is the userspace space state
	 * structure.
	 */
	THREAD->udebug.uspace_state = istate_get(THREAD);

	/*
	 * Early check for undebugged tasks. We do not lock anything as this
	 * test need not be precise in either direction.
	 */
	if (THREAD->udebug.active)
		udebug_syscall_event(a1, a2, a3, a4, a5, a6, id, 0, false);
#endif
	
	sysarg_t rc;
	if (id < SYSCALL_END) {
		rc = syscall_table[id](a1, a2, a3, a4, a5, a6);
	} else {
		log(LF_OTHER, LVL_ERROR,
		    "Task %" PRIu64": Unknown syscall %#" PRIxn, TASK->taskid, id);
		task_kill_self(true);
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

	/* Clear userspace state pointer */
	THREAD->udebug.uspace_state = NULL;
#endif
	
	/* Do kernel accounting */
	irq_spinlock_lock(&THREAD->lock, true);
	thread_update_accounting(false);
	irq_spinlock_unlock(&THREAD->lock, true);
	
	return rc;
}

syshandler_t syscall_table[SYSCALL_END] = {
	/* System management syscalls. */
	(syshandler_t) sys_kio,
	(syshandler_t) sys_tls_set,
	
	/* Thread and task related syscalls. */
	(syshandler_t) sys_thread_create,
	(syshandler_t) sys_thread_exit,
	(syshandler_t) sys_thread_get_id,
	(syshandler_t) sys_thread_usleep,
	(syshandler_t) sys_thread_udelay,
	
	(syshandler_t) sys_task_get_id,
	(syshandler_t) sys_task_set_name,
	(syshandler_t) sys_task_kill,
	(syshandler_t) sys_task_exit,
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
	
	/* Page mapping related syscalls. */
	(syshandler_t) sys_page_find_mapping,
	
	/* IPC related syscalls. */
	(syshandler_t) sys_ipc_call_async_fast,
	(syshandler_t) sys_ipc_call_async_slow,
	(syshandler_t) sys_ipc_answer_fast,
	(syshandler_t) sys_ipc_answer_slow,
	(syshandler_t) sys_ipc_forward_fast,
	(syshandler_t) sys_ipc_forward_slow,
	(syshandler_t) sys_ipc_wait_for_call,
	(syshandler_t) sys_ipc_poke,
	(syshandler_t) sys_ipc_hangup,
	(syshandler_t) sys_ipc_connect_kbox,
	
	/* Event notification syscalls. */
	(syshandler_t) sys_ipc_event_subscribe,
	(syshandler_t) sys_ipc_event_unsubscribe,
	(syshandler_t) sys_ipc_event_unmask,
	
	/* Capabilities related syscalls. */
	(syshandler_t) sys_cap_grant,
	(syshandler_t) sys_cap_revoke,
	
	/* DDI related syscalls. */
	(syshandler_t) sys_device_assign_devno,
	(syshandler_t) sys_physmem_map,
	(syshandler_t) sys_physmem_unmap,
	(syshandler_t) sys_dmamem_map,
	(syshandler_t) sys_dmamem_unmap,
	(syshandler_t) sys_iospace_enable,
	(syshandler_t) sys_iospace_disable,
	
	(syshandler_t) sys_ipc_irq_subscribe,
	(syshandler_t) sys_ipc_irq_unsubscribe,
	
	/* Sysinfo syscalls. */
	(syshandler_t) sys_sysinfo_get_keys_size,
	(syshandler_t) sys_sysinfo_get_keys,
	(syshandler_t) sys_sysinfo_get_val_type,
	(syshandler_t) sys_sysinfo_get_value,
	(syshandler_t) sys_sysinfo_get_data_size,
	(syshandler_t) sys_sysinfo_get_data,
	
	/* Kernel console syscalls. */
	(syshandler_t) sys_debug_console,
	
	(syshandler_t) sys_klog,
};

/** @}
 */
