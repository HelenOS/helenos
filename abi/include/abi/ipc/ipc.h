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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_IPC_IPC_H_
#define _ABI_IPC_IPC_H_

#include <stdint.h>
#include <abi/proc/task.h>
#include <abi/cap.h>
#include <_bits/errno.h>

/* Miscellaneous constants */
enum {
	/** Length of data being transferred with IPC call
	 *
	 * The uspace may not be able to utilize the full length
	 *
	 */
	IPC_CALL_LEN = 6,

	/** Maximum active async calls per phone */
	IPC_MAX_ASYNC_CALLS = 64,

	/**
	 * Maximum buffer size allowed for IPC_M_DATA_WRITE and
	 * IPC_M_DATA_READ requests.
	 */
	DATA_XFER_LIMIT = 64 * 1024,
};

/* Flags for calls */
enum {
	/** This is answer to a call */
	IPC_CALL_ANSWERED       = 1 << 0,

	/** Answer will not be passed to userspace, will be discarded */
	IPC_CALL_DISCARD_ANSWER = 1 << 1,

	/** Call was forwarded */
	IPC_CALL_FORWARDED      = 1 << 2,

	/** Interrupt notification */
	IPC_CALL_NOTIF          = 1 << 3,

	/** The call was automatically answered by the kernel due to error */
	IPC_CALL_AUTO_REPLY     = 1 << 4,
};

/* Forwarding flags. */
enum {
	IPC_FF_NONE = 0,

	/**
	 * The call will be routed as though it was initially sent via the phone
	 * used to forward it. This feature is intended to support the situation
	 * in which the forwarded call needs to be handled by the same
	 * connection fibril as any other calls that were initially sent by
	 * the forwarder to the same destination.
	 * This flag has no imapct on routing replies.
	 */
	IPC_FF_ROUTE_FROM_ME = 1 << 0,
};

/* Data transfer flags. */
enum {
	IPC_XF_NONE = 0,

	/** Restrict the transfer size if necessary. */
	IPC_XF_RESTRICT = 1 << 0,
};

/** User-defined IPC methods */
enum {
	IPC_FIRST_USER_METHOD = 1024,
};

typedef struct {
	sysarg_t args[IPC_CALL_LEN];
	/**
	 * Task which made or forwarded the call with IPC_FF_ROUTE_FROM_ME,
	 * or the task which answered the call.
	 */
	task_id_t task_id;
	/** Flags */
	unsigned flags;
	/** User-defined label associated with requests */
	sysarg_t request_label;
	/** User-defined label associated with answers */
	sysarg_t answer_label;
	/** Capability handle */
	cap_call_handle_t cap_handle;
} ipc_data_t;

/* Functions for manipulating calling data */

static inline void ipc_set_retval(ipc_data_t *data, errno_t retval)
{
	data->args[0] = (sysarg_t) retval;
}

static inline void ipc_set_imethod(ipc_data_t *data, sysarg_t val)
{
	data->args[0] = val;
}

static inline void ipc_set_arg1(ipc_data_t *data, sysarg_t val)
{
	data->args[1] = val;
}

static inline void ipc_set_arg2(ipc_data_t *data, sysarg_t val)
{
	data->args[2] = val;
}

static inline void ipc_set_arg3(ipc_data_t *data, sysarg_t val)
{
	data->args[3] = val;
}

static inline void ipc_set_arg4(ipc_data_t *data, sysarg_t val)
{
	data->args[4] = val;
}

static inline void ipc_set_arg5(ipc_data_t *data, sysarg_t val)
{
	data->args[5] = val;
}

static inline sysarg_t ipc_get_imethod(ipc_data_t *data)
{
	return data->args[0];
}
static inline errno_t ipc_get_retval(ipc_data_t *data)
{
	return (errno_t) data->args[0];
}

static inline sysarg_t ipc_get_arg1(ipc_data_t *data)
{
	return data->args[1];
}

static inline sysarg_t ipc_get_arg2(ipc_data_t *data)
{
	return data->args[2];
}

static inline sysarg_t ipc_get_arg3(ipc_data_t *data)
{
	return data->args[3];
}

static inline sysarg_t ipc_get_arg4(ipc_data_t *data)
{
	return data->args[4];
}

static inline sysarg_t ipc_get_arg5(ipc_data_t *data)
{
	return data->args[5];
}

#endif

/** @}
 */
