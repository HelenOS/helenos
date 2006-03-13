/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <ipc.h>
#include <libc.h>

int ipc_call_sync(int phoneid, int arg1, int arg2, ipc_data_t *resdata)
{
	return __SYSCALL4(SYS_IPC_CALL_SYNC, phoneid, arg1, arg2, 
			  (sysarg_t)resdata);
}

/*
int ipc_call_async()
{
	
}

int ipc_answer()
{
}
*/

/** Call syscall function sys_ipc_wait_for_call */
static inline ipc_callid_t _ipc_wait_for_call(ipc_data_t *data, int flags)
{
	return __SYSCALL2(SYS_IPC_WAIT, (sysarg_t)data, flags);
}

/** Wait for IPC call and return
 *
 * - dispatch ASYNC reoutines in the background
 */
int ipc_wait_for_call(ipc_data_t *data, int flags)
{
	ipc_callid_t callid;

	callid = _ipc_wait_for_call(data, flags);
	/* TODO: Handle async replies etc.. */
	return callid;
}
