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

#include <syscall/syscall.h>
#include <proc/thread.h>
#include <mm/as.h>
#include <print.h>
#include <putchar.h>
#include <errno.h>
#include <proc/task.h>
#include <arch.h>
#include <debug.h>
#include <ipc/sysipc.h>

static __native sys_io(int fd, const void * buf, size_t count) {
	
	// TODO: buf sanity checks and a lot of other stuff ...

	size_t i;
	
	for (i = 0; i < count; i++)
		putchar(((char *) buf)[i]);
	
	return count;
}

static __native sys_mmap(void *address, size_t size, int flags)
{
	if (as_area_create(AS, flags, size, (__address) address))
		return (__native) address;
	else
		return (__native) -1;
}

static __native sys_mremap(void *address, size_t size, int flags)
{
	return as_remap(AS, (__address) address, size, 0);
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
	sys_thread_create,
	sys_thread_exit,
	sys_mmap,
	sys_mremap,
	sys_ipc_call_sync_fast,
	sys_ipc_call_sync,
	sys_ipc_call_async_fast,
	sys_ipc_call_async,
	sys_ipc_answer_fast,
	sys_ipc_answer,
	sys_ipc_forward_fast,
	sys_ipc_wait_for_call,
	sys_ipc_hangup
};
