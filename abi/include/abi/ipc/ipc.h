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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#ifndef ABI_IPC_IPC_H_
#define ABI_IPC_IPC_H_

/** Length of data being transferred with IPC call
 *
 * The uspace may not be able to utilize the full length
 *
 */
#define IPC_CALL_LEN  6

/** Maximum active async calls per phone */
#define IPC_MAX_ASYNC_CALLS  64

/* Flags for calls */

/** This is answer to a call */
#define IPC_CALL_ANSWERED        (1 << 0)

/** Answer will not be passed to userspace, will be discarded */
#define IPC_CALL_DISCARD_ANSWER  (1 << 1)

/** Call was forwarded */
#define IPC_CALL_FORWARDED       (1 << 2)

/** Interrupt notification */
#define IPC_CALL_NOTIF           (1 << 3)

/** The call was automatically answered by the kernel due to error */
#define IPC_CALL_AUTO_REPLY      (1 << 4)

/**
 * Maximum buffer size allowed for IPC_M_DATA_WRITE and
 * IPC_M_DATA_READ requests.
 */
#define DATA_XFER_LIMIT  (64 * 1024)

/* Macros for manipulating calling data */
#define IPC_SET_RETVAL(data, retval)  ((data).args[0] = (sysarg_t) (retval))
#define IPC_SET_IMETHOD(data, val)    ((data).args[0] = (val))
#define IPC_SET_ARG1(data, val)       ((data).args[1] = (val))
#define IPC_SET_ARG2(data, val)       ((data).args[2] = (val))
#define IPC_SET_ARG3(data, val)       ((data).args[3] = (val))
#define IPC_SET_ARG4(data, val)       ((data).args[4] = (val))
#define IPC_SET_ARG5(data, val)       ((data).args[5] = (val))

#define IPC_GET_IMETHOD(data)  ((data).args[0])
#define IPC_GET_RETVAL(data)   ((errno_t) (data).args[0])

#define IPC_GET_ARG1(data)  ((data).args[1])
#define IPC_GET_ARG2(data)  ((data).args[2])
#define IPC_GET_ARG3(data)  ((data).args[3])
#define IPC_GET_ARG4(data)  ((data).args[4])
#define IPC_GET_ARG5(data)  ((data).args[5])

/* Forwarding flags. */
#define IPC_FF_NONE  0

/**
 * The call will be routed as though it was initially sent via the phone used to
 * forward it. This feature is intended to support the situation in which the
 * forwarded call needs to be handled by the same connection fibril as any other
 * calls that were initially sent by the forwarder to the same destination. This
 * flag has no imapct on routing replies.
 */
#define IPC_FF_ROUTE_FROM_ME  (1 << 0)

/* Data transfer flags. */
#define IPC_XF_NONE  0

/** Restrict the transfer size if necessary. */
#define IPC_XF_RESTRICT  (1 << 0)

/** User-defined IPC methods */
#define IPC_FIRST_USER_METHOD  1024

#endif

/** @}
 */
