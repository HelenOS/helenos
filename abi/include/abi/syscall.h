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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_SYSCALL_H_
#define _ABI_SYSCALL_H_

/** System calls.
 *
 * If you are adding or removing syscalls, or changing the number of
 * their arguments, please update syscall description table
 * in @c uspace/app/trace/syscalls.c
 */
typedef enum {
	SYS_KIO = 0,

	SYS_THREAD_CREATE,
	SYS_THREAD_EXIT,
	SYS_THREAD_GET_ID,
	SYS_THREAD_USLEEP,
	SYS_THREAD_UDELAY,

	SYS_TASK_GET_ID,
	SYS_TASK_SET_NAME,
	SYS_TASK_KILL,
	SYS_TASK_EXIT,
	SYS_PROGRAM_SPAWN_LOADER,

	SYS_WAITQ_CREATE,
	SYS_WAITQ_SLEEP,
	SYS_WAITQ_WAKEUP,
	SYS_WAITQ_DESTROY,
	SYS_SMC_COHERENCE,

	SYS_AS_AREA_CREATE,
	SYS_AS_AREA_RESIZE,
	SYS_AS_AREA_CHANGE_FLAGS,
	SYS_AS_AREA_GET_INFO,
	SYS_AS_AREA_DESTROY,

	SYS_PAGE_FIND_MAPPING,

	SYS_IPC_CALL_ASYNC_FAST,
	SYS_IPC_CALL_ASYNC_SLOW,
	SYS_IPC_ANSWER_FAST,
	SYS_IPC_ANSWER_SLOW,
	SYS_IPC_FORWARD_FAST,
	SYS_IPC_FORWARD_SLOW,
	SYS_IPC_WAIT,
	SYS_IPC_POKE,
	SYS_IPC_HANGUP,
	SYS_IPC_CONNECT_KBOX,

	SYS_IPC_EVENT_SUBSCRIBE,
	SYS_IPC_EVENT_UNSUBSCRIBE,
	SYS_IPC_EVENT_UNMASK,

	SYS_PERM_GRANT,
	SYS_PERM_REVOKE,

	SYS_PHYSMEM_MAP,
	SYS_PHYSMEM_UNMAP,
	SYS_DMAMEM_MAP,
	SYS_DMAMEM_UNMAP,
	SYS_IOSPACE_ENABLE,
	SYS_IOSPACE_DISABLE,

	SYS_IPC_IRQ_SUBSCRIBE,
	SYS_IPC_IRQ_UNSUBSCRIBE,

	SYS_SYSINFO_GET_KEYS_SIZE,
	SYS_SYSINFO_GET_KEYS,
	SYS_SYSINFO_GET_VAL_TYPE,
	SYS_SYSINFO_GET_VALUE,
	SYS_SYSINFO_GET_DATA_SIZE,
	SYS_SYSINFO_GET_DATA,

	SYS_DEBUG_CONSOLE,

	SYS_KLOG
} syscall_t;

#endif

/** @}
 */
