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

#define LIBC_ASYNC_C_
#define LIBC_ASYNC_OBSOLETE_C_
#include <ipc/ipc.h>
#include <async.h>
#include <async_obsolete.h>
#undef LIBC_ASYNC_C_
#undef LIBC_ASYNC_OBSOLETE_C_

#include <fibril.h>
#include <malloc.h>
#include <errno.h>
#include "private/async.h"

/** Send message and return id of the sent message.
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param phoneid Handle of the phone that will be used for the send.
 * @param method  Service-defined method.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param dataptr If non-NULL, storage where the reply data will be
 *                stored.
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
aid_t async_obsolete_send_fast(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr)
{
	amsg_t *msg = malloc(sizeof(amsg_t));
	
	if (!msg)
		return 0;
	
	msg->done = false;
	msg->dataptr = dataptr;
	
	msg->wdata.to_event.inlist = false;
	
	/*
	 * We may sleep in the next method,
	 * but it will use its own means
	 */
	msg->wdata.active = true;
	
	ipc_call_async_4(phoneid, method, arg1, arg2, arg3, arg4, msg,
	    reply_received, true);
	
	return (aid_t) msg;
}

/** Pseudo-synchronous message sending - fast version.
 *
 * Send message asynchronously and return only after the reply arrives.
 *
 * This function can only transfer 4 register payload arguments. For
 * transferring more arguments, see the slower async_req_slow().
 *
 * @param phoneid Hash of the phone through which to make the call.
 * @param method  Method of the call.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param r1      If non-NULL, storage for the 1st reply argument.
 * @param r2      If non-NULL, storage for the 2nd reply argument.
 * @param r3      If non-NULL, storage for the 3rd reply argument.
 * @param r4      If non-NULL, storage for the 4th reply argument.
 * @param r5      If non-NULL, storage for the 5th reply argument.
 *
 * @return Return code of the reply or a negative error code.
 *
 */
sysarg_t async_obsolete_req_fast(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	ipc_call_t result;
	aid_t eid = async_obsolete_send_4(phoneid, method, arg1, arg2, arg3, arg4,
	    &result);
	
	sysarg_t rc;
	async_wait_for(eid, &rc);
	
	if (r1)
		*r1 = IPC_GET_ARG1(result);
	
	if (r2)
		*r2 = IPC_GET_ARG2(result);
	
	if (r3)
		*r3 = IPC_GET_ARG3(result);
	
	if (r4)
		*r4 = IPC_GET_ARG4(result);
	
	if (r5)
		*r5 = IPC_GET_ARG5(result);
	
	return rc;
}

/** Wrapper for IPC_M_SHARE_OUT calls using the async framework.
 *
 * @param phoneid Phone that will be used to contact the receiving side.
 * @param src     Source address space area base address.
 * @param flags   Flags to be used for sharing. Bits can be only cleared.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_obsolete_share_out_start(int phoneid, void *src, unsigned int flags)
{
	return async_obsolete_req_3_0(phoneid, IPC_M_SHARE_OUT, (sysarg_t) src, 0,
	    (sysarg_t) flags);
}

/** Wrapper for ipc_hangup.
 *
 * @param phone Phone handle to hung up.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_obsolete_hangup(int phone)
{
	return ipc_hangup(phone);
}

/** Wrapper for IPC_M_DATA_WRITE calls using the async framework.
 *
 * @param phoneid Phone that will be used to contact the receiving side.
 * @param src     Address of the beginning of the source buffer.
 * @param size    Size of the source buffer.
 * @param flags   Flags to control the data transfer.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_obsolete_data_write_start_generic(int phoneid, const void *src, size_t size,
    int flags)
{
	return async_obsolete_req_3_0(phoneid, IPC_M_DATA_WRITE, (sysarg_t) src,
	    (sysarg_t) size, (sysarg_t) flags);
}

/** Start IPC_M_DATA_READ using the async framework.
 *
 * @param phoneid Phone that will be used to contact the receiving side.
 * @param dst     Address of the beginning of the destination buffer.
 * @param size    Size of the destination buffer (in bytes).
 * @param dataptr Storage of call data (arg 2 holds actual data size).
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
aid_t async_obsolete_data_read(int phone, void *dst, size_t size,
    ipc_call_t *dataptr)
{
	return async_obsolete_send_2(phone, IPC_M_DATA_READ, (sysarg_t) dst,
	    (sysarg_t) size, dataptr);
}

/** Wrapper for IPC_M_DATA_READ calls using the async framework.
 *
 * @param phoneid Phone that will be used to contact the receiving side.
 * @param dst     Address of the beginning of the destination buffer.
 * @param size    Size of the destination buffer.
 * @param flags   Flags to control the data transfer.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_obsolete_data_read_start_generic(int phoneid, void *dst, size_t size, int flags)
{
	return async_obsolete_req_3_0(phoneid, IPC_M_DATA_READ, (sysarg_t) dst,
	    (sysarg_t) size, (sysarg_t) flags);
}

/** Wrapper for making IPC_M_CONNECT_TO_ME calls using the async framework.
 *
 * Ask through phone for a new connection to some service.
 *
 * @param phone           Phone handle used for contacting the other side.
 * @param arg1            User defined argument.
 * @param arg2            User defined argument.
 * @param arg3            User defined argument.
 * @param client_receiver Connection handing routine.
 *
 * @return New phone handle on success or a negative error code.
 *
 */
int async_obsolete_connect_to_me(int phone, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_client_conn_t client_receiver, void *carg)
{
	sysarg_t task_hash;
	sysarg_t phone_hash;
	int rc = async_obsolete_req_3_5(phone, IPC_M_CONNECT_TO_ME, arg1, arg2, arg3,
	    NULL, NULL, NULL, &task_hash, &phone_hash);
	if (rc != EOK)
		return rc;
	
	if (client_receiver != NULL)
		async_new_connection(task_hash, phone_hash, 0, NULL,
		    client_receiver, carg);
	
	return EOK;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through phone for a new connection to some service.
 *
 * @param phone Phone handle used for contacting the other side.
 * @param arg1  User defined argument.
 * @param arg2  User defined argument.
 * @param arg3  User defined argument.
 *
 * @return New phone handle on success or a negative error code.
 *
 */
int async_obsolete_connect_me_to(int phone, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3)
{
	sysarg_t newphid;
	int rc = async_obsolete_req_3_5(phone, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3,
	    NULL, NULL, NULL, NULL, &newphid);
	
	if (rc != EOK)
		return rc;
	
	return newphid;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through phone for a new connection to some service and block until
 * success.
 *
 * @param phoneid Phone handle used for contacting the other side.
 * @param arg1    User defined argument.
 * @param arg2    User defined argument.
 * @param arg3    User defined argument.
 *
 * @return New phone handle on success or a negative error code.
 *
 */
int async_obsolete_connect_me_to_blocking(int phoneid, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3)
{
	sysarg_t newphid;
	int rc = async_obsolete_req_4_5(phoneid, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3,
	    IPC_FLAG_BLOCKING, NULL, NULL, NULL, NULL, &newphid);
	
	if (rc != EOK)
		return rc;
	
	return newphid;
}

/** Send message and return id of the sent message
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param phoneid Handle of the phone that will be used for the send.
 * @param method  Service-defined method.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param arg5    Service-defined payload argument.
 * @param dataptr If non-NULL, storage where the reply data will be
 *                stored.
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
aid_t async_obsolete_send_slow(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    ipc_call_t *dataptr)
{
	amsg_t *msg = malloc(sizeof(amsg_t));
	
	if (!msg)
		return 0;
	
	msg->done = false;
	msg->dataptr = dataptr;
	
	msg->wdata.to_event.inlist = false;
	
	/*
	 * We may sleep in the next method,
	 * but it will use its own means
	 */
	msg->wdata.active = true;
	
	ipc_call_async_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, msg,
	    reply_received, true);
	
	return (aid_t) msg;
}

void async_obsolete_msg_0(int phone, sysarg_t imethod)
{
	ipc_call_async_0(phone, imethod, NULL, NULL, true);
}

void async_obsolete_msg_1(int phone, sysarg_t imethod, sysarg_t arg1)
{
	ipc_call_async_1(phone, imethod, arg1, NULL, NULL, true);
}

void async_obsolete_msg_2(int phone, sysarg_t imethod, sysarg_t arg1, sysarg_t arg2)
{
	ipc_call_async_2(phone, imethod, arg1, arg2, NULL, NULL, true);
}

void async_obsolete_msg_3(int phone, sysarg_t imethod, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3)
{
	ipc_call_async_3(phone, imethod, arg1, arg2, arg3, NULL, NULL, true);
}

void async_obsolete_msg_4(int phone, sysarg_t imethod, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, sysarg_t arg4)
{
	ipc_call_async_4(phone, imethod, arg1, arg2, arg3, arg4, NULL, NULL,
	    true);
}

void async_obsolete_msg_5(int phone, sysarg_t imethod, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	ipc_call_async_5(phone, imethod, arg1, arg2, arg3, arg4, arg5, NULL,
	    NULL, true);
}

/** Wrapper for IPC_M_SHARE_IN calls using the async framework.
 *
 * @param phoneid Phone that will be used to contact the receiving side.
 * @param dst     Destination address space area base.
 * @param size    Size of the destination address space area.
 * @param arg     User defined argument.
 * @param flags   Storage for the received flags. Can be NULL.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_obsolete_share_in_start(int phoneid, void *dst, size_t size, sysarg_t arg,
    unsigned int *flags)
{
	sysarg_t tmp_flags;
	int res = async_obsolete_req_3_2(phoneid, IPC_M_SHARE_IN, (sysarg_t) dst,
	    (sysarg_t) size, arg, NULL, &tmp_flags);
	
	if (flags)
		*flags = (unsigned int) tmp_flags;
	
	return res;
}

int async_obsolete_forward_fast(ipc_callid_t callid, int phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	return ipc_forward_fast(callid, phoneid, imethod, arg1, arg2, mode);
}

int async_obsolete_forward_slow(ipc_callid_t callid, int phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    unsigned int mode)
{
	return ipc_forward_slow(callid, phoneid, imethod, arg1, arg2, arg3, arg4,
	    arg5, mode);
}

/** Wrapper for forwarding any data that is about to be received
 *
 */
int async_obsolete_data_write_forward_fast(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr)
{
	ipc_callid_t callid;
	if (!async_data_write_receive(&callid, NULL)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	aid_t msg = async_obsolete_send_fast(phoneid, method, arg1, arg2, arg3, arg4,
	    dataptr);
	if (msg == 0) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	int retval = ipc_forward_fast(callid, phoneid, 0, 0, 0,
	    IPC_FF_ROUTE_FROM_ME);
	if (retval != EOK) {
		async_wait_for(msg, NULL);
		ipc_answer_0(callid, retval);
		return retval;
	}
	
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	return (int) rc;
}

/** @}
 */
