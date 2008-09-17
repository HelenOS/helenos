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

/** @addtogroup libc
 * @{
 */
/** @file
 */ 

#include <udebug.h>
#include <sys/types.h>
#include <syscall.h>
#include <ipc/ipc.h>
#include <async.h>

int udebug_begin(int phoneid)
{
	return async_req_1_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_BEGIN);
}

int udebug_end(int phoneid)
{
	return async_req_1_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_END);
}

int udebug_set_evmask(int phoneid, udebug_evmask_t mask)
{
	return async_req_2_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_SET_EVMASK,
		mask);
}

int udebug_thread_read(int phoneid, void *buffer, size_t n,
	size_t *copied, size_t *needed)
{
	unsigned dest_addr;

	return async_req_3_3(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_THREAD_READ,
		(sysarg_t)buffer, n, &dest_addr, copied, needed);
}

int udebug_mem_read(int phoneid, void *buffer, uintptr_t addr, size_t n)
{
	return async_req_4_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_MEM_READ,
	    (sysarg_t)buffer, addr, n);
}

int udebug_args_read(int phoneid, thash_t tid, sysarg_t *buffer)
{
	return async_req_3_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_ARGS_READ,
	    tid, (sysarg_t)buffer);
}

int udebug_go(int phoneid, thash_t tid, udebug_event_t *ev_type,
    sysarg_t *val0, sysarg_t *val1)
{
	return async_req_2_3(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_GO,
	    tid, (sysarg_t)ev_type, (sysarg_t)val0, (sysarg_t)val1);
}

int udebug_stop(int phoneid, thash_t tid)
{
	return async_req_2_0(phoneid, IPC_M_DEBUG_ALL, UDEBUG_M_STOP,
	    tid);
}

/** @}
 */
