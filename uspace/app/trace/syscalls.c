/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#include <abi/syscall.h>
#include "syscalls.h"
#include "trace.h"

const sc_desc_t syscall_desc[] = {
	/* System management syscalls. */
	[SYS_KIO] = { "kio", 3, V_INT_ERRNO },

	/* Thread and task related syscalls. */
	[SYS_THREAD_CREATE] = { "thread_create", 3, V_ERRNO },
	[SYS_THREAD_EXIT] = { "thread_exit", 1, V_ERRNO },
	[SYS_THREAD_GET_ID] = { "thread_get_id", 1, V_ERRNO },
	[SYS_THREAD_USLEEP] = { "thread_usleep", 1, V_ERRNO },
	[SYS_THREAD_UDELAY] = { "thread_udelay", 1, V_ERRNO },

	[SYS_TASK_GET_ID] = { "task_get_id", 1, V_ERRNO },
	[SYS_TASK_SET_NAME] = { "task_set_name", 2, V_ERRNO },
	[SYS_TASK_KILL] = { "task_kill", 1, V_ERRNO },
	[SYS_TASK_EXIT] = { "task_exit", 1, V_ERRNO },
	[SYS_PROGRAM_SPAWN_LOADER] = { "program_spawn_loader", 2, V_ERRNO },

	/* Synchronization related syscalls. */
	[SYS_WAITQ_CREATE] = { "waitq_create", 1, V_ERRNO },
	[SYS_WAITQ_SLEEP] = { "waitq_sleep", 3, V_ERRNO },
	[SYS_WAITQ_WAKEUP] = { "waitq_wakeup", 1, V_ERRNO },
	[SYS_WAITQ_DESTROY] = { "waitq_destroy", 1, V_ERRNO },
	[SYS_SMC_COHERENCE] = { "smc_coherence", 2, V_ERRNO },

	/* Address space related syscalls. */
	[SYS_AS_AREA_CREATE] = { "as_area_create", 5, V_ERRNO },
	[SYS_AS_AREA_RESIZE] = { "as_area_resize", 3, V_ERRNO },
	[SYS_AS_AREA_CHANGE_FLAGS] = { "as_area_change_flags", 2, V_ERRNO },
	[SYS_AS_AREA_GET_INFO] = { "as_area_get_info", 2, V_ERRNO },
	[SYS_AS_AREA_DESTROY] = { "as_area_destroy", 1, V_ERRNO },

	/* Page mapping related syscalls. */
	[SYS_PAGE_FIND_MAPPING] = { "page_find_mapping", 2, V_ERRNO },

	/* IPC related syscalls. */
	[SYS_IPC_CALL_ASYNC_FAST] = { "ipc_call_async_fast", 6, V_HASH },
	[SYS_IPC_CALL_ASYNC_SLOW] = { "ipc_call_async_slow", 3, V_HASH },
	[SYS_IPC_ANSWER_FAST] = { "ipc_answer_fast", 6, V_ERRNO },
	[SYS_IPC_ANSWER_SLOW] = { "ipc_answer_slow", 2, V_ERRNO },
	[SYS_IPC_FORWARD_FAST] = { "ipc_forward_fast", 6, V_ERRNO },
	[SYS_IPC_FORWARD_SLOW] = { "ipc_forward_slow", 3, V_ERRNO },
	[SYS_IPC_WAIT] = { "ipc_wait_for_call", 3, V_HASH },
	[SYS_IPC_POKE] = { "ipc_poke", 0, V_ERRNO },
	[SYS_IPC_HANGUP] = { "ipc_hangup", 1, V_ERRNO },
	[SYS_IPC_CONNECT_KBOX] = { "ipc_connect_kbox", 2, V_ERRNO },

	/* Event notification syscalls. */
	[SYS_IPC_EVENT_SUBSCRIBE] = { "ipc_event_subscribe", 2, V_ERRNO },
	[SYS_IPC_EVENT_UNSUBSCRIBE] = { "ipc_event_unsubscribe", 1, V_ERRNO },
	[SYS_IPC_EVENT_UNMASK] = { "ipc_event_unmask", 1, V_ERRNO },

	/* Permission related syscalls. */
	[SYS_PERM_GRANT] = { "perm_grant", 2, V_ERRNO },
	[SYS_PERM_REVOKE] = { "perm_revoke", 2, V_ERRNO },

	/* DDI related syscalls. */
	[SYS_PHYSMEM_MAP] = { "physmem_map", 4, V_ERRNO },
	[SYS_PHYSMEM_UNMAP] = { "physmem_unmap", 1, V_ERRNO },
	[SYS_DMAMEM_MAP] = { "dmamem_map", 6, V_ERRNO },
	[SYS_DMAMEM_UNMAP] = { "dmamem_unmap", 3, V_ERRNO },
	[SYS_IOSPACE_ENABLE] = { "iospace_enable", 1, V_ERRNO },
	[SYS_IOSPACE_DISABLE] = { "iospace_disable", 1, V_ERRNO },

	[SYS_IPC_IRQ_SUBSCRIBE] = { "ipc_irq_subscribe", 4, V_ERRNO },
	[SYS_IPC_IRQ_UNSUBSCRIBE] = { "ipc_irq_unsubscribe", 2, V_ERRNO },

	/* Sysinfo syscalls. */
	[SYS_SYSINFO_GET_KEYS_SIZE] = { "sysinfo_get_keys_size", 3, V_ERRNO },
	[SYS_SYSINFO_GET_KEYS] = { "sysinfo_get_keys", 5, V_ERRNO },
	[SYS_SYSINFO_GET_VAL_TYPE] = { "sysinfo_get_val_type", 2, V_INTEGER },
	[SYS_SYSINFO_GET_VALUE] = { "sysinfo_get_value", 3, V_ERRNO },
	[SYS_SYSINFO_GET_DATA_SIZE] = { "sysinfo_get_data_size", 3, V_ERRNO },
	[SYS_SYSINFO_GET_DATA] = { "sysinfo_get_data", 5, V_ERRNO },

	/* Kernel console syscalls. */
	[SYS_DEBUG_CONSOLE] = { "debug_console", 0, V_ERRNO },

	[SYS_KLOG] = { "klog", 5, V_ERRNO }
};

const size_t syscall_desc_len = (sizeof(syscall_desc) / sizeof(sc_desc_t));

/** @}
 */
