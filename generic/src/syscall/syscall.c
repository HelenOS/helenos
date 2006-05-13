/*
 * Copyright (C) 2005 Martin Decky
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

/**
 * @file	syscall.c
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

static __native sys_io(int fd, const void * buf, size_t count) {
	
	// TODO: buf sanity checks and a lot of other stuff ...

	size_t i;
	
	for (i = 0; i < count; i++)
		putchar(((char *) buf)[i]);
	
	return count;
}

/** Dispatch system call */
__native syscall_handler(__native a1, __native a2, __native a3,
			 __native a4, __native id)
{
	if (id < SYSCALL_END)
		return syscall_table[id](a1,a2,a3,a4);
	else
		panic("Undefined syscall %d", id);
}

syshandler_t syscall_table[SYSCALL_END] = {
	sys_io,
	sys_tls_set,
	
	/* Thread and task related syscalls. */
	sys_thread_create,
	sys_thread_exit,
	sys_task_get_id,
	
	/* Synchronization related syscalls. */
	sys_futex_sleep_timeout,
	sys_futex_wakeup,
	
	/* Address space related syscalls. */
	sys_as_area_create,
	sys_as_area_resize,
	sys_as_area_destroy,
	sys_as_area_accept,
	sys_as_area_send,

	/* IPC related syscalls. */
	sys_ipc_call_sync_fast,
	sys_ipc_call_sync,
	sys_ipc_call_async_fast,
	sys_ipc_call_async,
	sys_ipc_answer_fast,
	sys_ipc_answer,
	sys_ipc_forward_fast,
	sys_ipc_wait_for_call,
	sys_ipc_hangup,
	sys_ipc_register_irq,
	sys_ipc_unregister_irq,

	/* Capabilities related syscalls. */
	sys_cap_grant,
	sys_cap_revoke,

	/* DDI related syscalls. */
	sys_physmem_map,
	sys_iospace_enable,
	sys_preempt_control,
	
	/* Sysinfo syscalls */
	sys_sysinfo_valid,
	sys_sysinfo_value
};
