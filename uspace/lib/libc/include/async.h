/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#ifndef LIBC_ASYNC_H_
#define LIBC_ASYNC_H_

#include <ipc/ipc.h>
#include <fibril.h>
#include <sys/time.h>
#include <atomic.h>

typedef ipc_callid_t aid_t;
typedef void (*async_client_conn_t)(ipc_callid_t callid, ipc_call_t *call);

static inline void async_manager(void)
{
	fibril_switch(FIBRIL_TO_MANAGER);
}

ipc_callid_t async_get_call_timeout(ipc_call_t *call, suseconds_t usecs);
static inline ipc_callid_t async_get_call(ipc_call_t *data)
{
	return async_get_call_timeout(data, 0);
}

aid_t async_send_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipc_call_t *dataptr);
aid_t async_send_3(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipcarg_t arg3, ipc_call_t *dataptr);
void async_wait_for(aid_t amsgid, ipcarg_t *result);
int async_wait_timeout(aid_t amsgid, ipcarg_t *retval, suseconds_t timeout);

/** Pseudo-synchronous message sending
 *
 * Send message through IPC, wait in the event loop, until it is received
 *
 * @return Return code of message
 */
static inline ipcarg_t async_req_2(int phoneid, ipcarg_t method, ipcarg_t arg1, 
    ipcarg_t arg2, ipcarg_t *r1, ipcarg_t *r2)
{
	ipc_call_t result;
	ipcarg_t rc;

	aid_t eid = async_send_2(phoneid, method, arg1, arg2, &result);
	async_wait_for(eid, &rc);
	if (r1) 
		*r1 = IPC_GET_ARG1(result);
	if (r2)
		*r2 = IPC_GET_ARG2(result);
	return rc;
}
#define async_req(phoneid, method, arg1, r1) \
    async_req_2(phoneid, method, arg1, 0, r1, 0)

static inline ipcarg_t async_req_3(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t *r1, ipcarg_t *r2, ipcarg_t *r3)
{
	ipc_call_t result;
	ipcarg_t rc;

	aid_t eid = async_send_3(phoneid, method, arg1, arg2, arg3, &result);
	async_wait_for(eid, &rc);
	if (r1) 
		*r1 = IPC_GET_ARG1(result);
	if (r2)
		*r2 = IPC_GET_ARG2(result);
	if (r3)
		*r3 = IPC_GET_ARG3(result);
	return rc;
}


fid_t async_new_connection(ipcarg_t in_phone_hash,ipc_callid_t callid, 
    ipc_call_t *call, void (*cthread)(ipc_callid_t,ipc_call_t *));
void async_usleep(suseconds_t timeout);
void async_create_manager(void);
void async_destroy_manager(void);
void async_set_client_connection(async_client_conn_t conn);
void async_set_interrupt_received(async_client_conn_t conn);
int _async_init(void);


/* Primitve functions for IPC communication */
void async_msg_3(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2, 
    ipcarg_t arg3);
void async_msg_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2);
#define async_msg(ph, m, a1) async_msg_2(ph, m, a1, 0)

static inline void async_serialize_start(void)
{
	fibril_inc_sercount();
}

static inline void async_serialize_end(void)
{
	fibril_dec_sercount();
}

extern atomic_t async_futex;
#endif

/** @}
 */
