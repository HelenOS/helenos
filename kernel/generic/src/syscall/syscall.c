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

/** @addtogroup kernel_generic
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
#include <arch.h>
#include <debug.h>
#include <interrupt.h>
#include <ipc/sysipc.h>
#include <synch/smc.h>
#include <synch/syswaitq.h>
#include <ddi/ddi.h>
#include <ipc/event.h>
#include <security/perm.h>
#include <sysinfo/sysinfo.h>
#include <console/console.h>
#include <udebug/udebug.h>
#include <log.h>

static syshandler_t syscall_table[] = {
	/* System management syscalls. */
	[SYS_KIO] = (syshandler_t) sys_kio,

	/* Thread and task related syscalls. */
	[SYS_THREAD_CREATE] = (syshandler_t) sys_thread_create,
	[SYS_THREAD_EXIT] = (syshandler_t) sys_thread_exit,
	[SYS_THREAD_GET_ID] = (syshandler_t) sys_thread_get_id,
	[SYS_THREAD_USLEEP] = (syshandler_t) sys_thread_usleep,
	[SYS_THREAD_UDELAY] = (syshandler_t) sys_thread_udelay,

	[SYS_TASK_GET_ID] = (syshandler_t) sys_task_get_id,
	[SYS_TASK_SET_NAME] = (syshandler_t) sys_task_set_name,
	[SYS_TASK_KILL] = (syshandler_t) sys_task_kill,
	[SYS_TASK_EXIT] = (syshandler_t) sys_task_exit,
	[SYS_PROGRAM_SPAWN_LOADER] = (syshandler_t) sys_program_spawn_loader,

	/* Synchronization related syscalls. */
	[SYS_WAITQ_CREATE] = (syshandler_t) sys_waitq_create,
	[SYS_WAITQ_SLEEP] = (syshandler_t) sys_waitq_sleep,
	[SYS_WAITQ_WAKEUP] = (syshandler_t) sys_waitq_wakeup,
	[SYS_WAITQ_DESTROY] = (syshandler_t) sys_waitq_destroy,
	[SYS_SMC_COHERENCE] = (syshandler_t) sys_smc_coherence,

	/* Address space related syscalls. */
	[SYS_AS_AREA_CREATE] = (syshandler_t) sys_as_area_create,
	[SYS_AS_AREA_RESIZE] = (syshandler_t) sys_as_area_resize,
	[SYS_AS_AREA_CHANGE_FLAGS] = (syshandler_t) sys_as_area_change_flags,
	[SYS_AS_AREA_GET_INFO] = (syshandler_t) sys_as_area_get_info,
	[SYS_AS_AREA_DESTROY] = (syshandler_t) sys_as_area_destroy,

	/* Page mapping related syscalls. */
	[SYS_PAGE_FIND_MAPPING] = (syshandler_t) sys_page_find_mapping,

	/* IPC related syscalls. */
	[SYS_IPC_CALL_ASYNC_FAST] = (syshandler_t) sys_ipc_call_async_fast,
	[SYS_IPC_CALL_ASYNC_SLOW] = (syshandler_t) sys_ipc_call_async_slow,
	[SYS_IPC_ANSWER_FAST] = (syshandler_t) sys_ipc_answer_fast,
	[SYS_IPC_ANSWER_SLOW] = (syshandler_t) sys_ipc_answer_slow,
	[SYS_IPC_FORWARD_FAST] = (syshandler_t) sys_ipc_forward_fast,
	[SYS_IPC_FORWARD_SLOW] = (syshandler_t) sys_ipc_forward_slow,
	[SYS_IPC_WAIT] = (syshandler_t) sys_ipc_wait_for_call,
	[SYS_IPC_POKE] = (syshandler_t) sys_ipc_poke,
	[SYS_IPC_HANGUP] = (syshandler_t) sys_ipc_hangup,
	[SYS_IPC_CONNECT_KBOX] = (syshandler_t) sys_ipc_connect_kbox,

	/* Event notification syscalls. */
	[SYS_IPC_EVENT_SUBSCRIBE] = (syshandler_t) sys_ipc_event_subscribe,
	[SYS_IPC_EVENT_UNSUBSCRIBE] = (syshandler_t) sys_ipc_event_unsubscribe,
	[SYS_IPC_EVENT_UNMASK] = (syshandler_t) sys_ipc_event_unmask,

	/* Permission related syscalls. */
	[SYS_PERM_GRANT] = (syshandler_t) sys_perm_grant,
	[SYS_PERM_REVOKE] = (syshandler_t) sys_perm_revoke,

	/* DDI related syscalls. */
	[SYS_PHYSMEM_MAP] = (syshandler_t) sys_physmem_map,
	[SYS_PHYSMEM_UNMAP] = (syshandler_t) sys_physmem_unmap,
	[SYS_DMAMEM_MAP] = (syshandler_t) sys_dmamem_map,
	[SYS_DMAMEM_UNMAP] = (syshandler_t) sys_dmamem_unmap,
	[SYS_IOSPACE_ENABLE] = (syshandler_t) sys_iospace_enable,
	[SYS_IOSPACE_DISABLE] = (syshandler_t) sys_iospace_disable,

	[SYS_IPC_IRQ_SUBSCRIBE] = (syshandler_t) sys_ipc_irq_subscribe,
	[SYS_IPC_IRQ_UNSUBSCRIBE] = (syshandler_t) sys_ipc_irq_unsubscribe,

	/* Sysinfo syscalls. */
	[SYS_SYSINFO_GET_KEYS_SIZE] = (syshandler_t) sys_sysinfo_get_keys_size,
	[SYS_SYSINFO_GET_KEYS] = (syshandler_t) sys_sysinfo_get_keys,
	[SYS_SYSINFO_GET_VAL_TYPE] = (syshandler_t) sys_sysinfo_get_val_type,
	[SYS_SYSINFO_GET_VALUE] = (syshandler_t) sys_sysinfo_get_value,
	[SYS_SYSINFO_GET_DATA_SIZE] = (syshandler_t) sys_sysinfo_get_data_size,
	[SYS_SYSINFO_GET_DATA] = (syshandler_t) sys_sysinfo_get_data,

	/* Kernel console syscalls. */
	[SYS_DEBUG_CONSOLE] = (syshandler_t) sys_debug_console,

	[SYS_KLOG] = (syshandler_t) sys_klog,
};

/** Dispatch system call */
sysarg_t syscall_handler(sysarg_t a1, sysarg_t a2, sysarg_t a3,
    sysarg_t a4, sysarg_t a5, sysarg_t a6, sysarg_t id)
{
	/* Do userpace accounting */
	ipl_t ipl = interrupts_disable();
	thread_update_accounting(true);
	interrupts_restore(ipl);

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
	if (id < sizeof_array(syscall_table)) {
		rc = syscall_table[id](a1, a2, a3, a4, a5, a6);
	} else {
		log(LF_OTHER, LVL_ERROR,
		    "Task %" PRIu64 ": Unknown syscall %#" PRIxn, TASK->taskid, id);
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
	ipl = interrupts_disable();
	thread_update_accounting(false);
	interrupts_restore(ipl);

	return rc;
}

/** @}
 */
