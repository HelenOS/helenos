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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#if ((defined(LIBC_ASYNC_H_)) && (!defined(LIBC_ASYNC_C_)))
	#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef LIBC_IPC_H_
#define LIBC_IPC_H_

#include <sys/types.h>
#include <ipc/common.h>
#include <abi/ipc/methods.h>
#include <abi/synch.h>
#include <task.h>

typedef void (*ipc_async_callback_t)(void *, int, ipc_call_t *);

/*
 * User-friendly wrappers for ipc_call_sync_fast() and ipc_call_sync_slow().
 * They are in the form ipc_call_sync_m_n(), where m denotes the number of
 * arguments of payload and n denotes number of return values. Whenever
 * possible, the fast version is used.
 */

#define ipc_call_sync_0_0(phoneid, method) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, 0, 0, 0, 0, 0)
#define ipc_call_sync_0_1(phoneid, method, res1) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, (res1), 0, 0, 0, 0)
#define ipc_call_sync_0_2(phoneid, method, res1, res2) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, (res1), (res2), 0, 0, 0)
#define ipc_call_sync_0_3(phoneid, method, res1, res2, res3) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, (res1), (res2), (res3), \
	    0, 0)
#define ipc_call_sync_0_4(phoneid, method, res1, res2, res3, res4) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, (res1), (res2), (res3), \
	    (res4), 0)
#define ipc_call_sync_0_5(phoneid, method, res1, res2, res3, res4, res5) \
	ipc_call_sync_fast((phoneid), (method), 0, 0, 0, (res1), (res2), (res3), \
	    (res4), (res5))

#define ipc_call_sync_1_0(phoneid, method, arg1) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, 0, 0, 0, 0, 0)
#define ipc_call_sync_1_1(phoneid, method, arg1, res1) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, (res1), 0, 0, 0, 0)
#define ipc_call_sync_1_2(phoneid, method, arg1, res1, res2) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, (res1), (res2), 0, \
	    0, 0)
#define ipc_call_sync_1_3(phoneid, method, arg1, res1, res2, res3) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, (res1), (res2), \
	    (res3), 0, 0)
#define ipc_call_sync_1_4(phoneid, method, arg1, res1, res2, res3, res4) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, (res1), (res2), \
	    (res3), (res4), 0)
#define ipc_call_sync_1_5(phoneid, method, arg1, res1, res2, res3, res4, \
    res5) \
	ipc_call_sync_fast((phoneid), (method), (arg1), 0, 0, (res1), (res2), \
	    (res3), (res4), (res5))

#define ipc_call_sync_2_0(phoneid, method, arg1, arg2) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, 0, 0, 0, \
	    0, 0)
#define ipc_call_sync_2_1(phoneid, method, arg1, arg2, res1) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, (res1), 0, 0, \
	    0, 0)
#define ipc_call_sync_2_2(phoneid, method, arg1, arg2, res1, res2) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, (res1), \
	    (res2), 0, 0, 0)
#define ipc_call_sync_2_3(phoneid, method, arg1, arg2, res1, res2, res3) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, (res1), \
	    (res2), (res3), 0, 0)
#define ipc_call_sync_2_4(phoneid, method, arg1, arg2, res1, res2, res3, \
    res4) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, (res1), \
	    (res2), (res3), (res4), 0)
#define ipc_call_sync_2_5(phoneid, method, arg1, arg2, res1, res2, res3, \
    res4, res5)\
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), 0, (res1), \
	    (res2), (res3), (res4), (res5))

#define ipc_call_sync_3_0(phoneid, method, arg1, arg2, arg3) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, 0, 0, \
	    0, 0)
#define ipc_call_sync_3_1(phoneid, method, arg1, arg2, arg3, res1) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), (res1), \
	    0, 0, 0, 0)
#define ipc_call_sync_3_2(phoneid, method, arg1, arg2, arg3, res1, res2) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), (res1), \
	    (res2), 0, 0, 0)
#define ipc_call_sync_3_3(phoneid, method, arg1, arg2, arg3, res1, res2, \
    res3) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (res1), (res2), (res3), 0, 0)
#define ipc_call_sync_3_4(phoneid, method, arg1, arg2, arg3, res1, res2, \
    res3, res4) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (res1), (res2), (res3), (res4), 0)
#define ipc_call_sync_3_5(phoneid, method, arg1, arg2, arg3, res1, res2, \
    res3, res4, res5) \
	ipc_call_sync_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (res1), (res2), (res3), (res4), (res5))

#define ipc_call_sync_4_0(phoneid, method, arg1, arg2, arg3, arg4) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), 0, \
	    0, 0, 0, 0, 0)
#define ipc_call_sync_4_1(phoneid, method, arg1, arg2, arg3, arg4, res1) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), 0, \
	    (res1), 0, 0, 0, 0)
#define ipc_call_sync_4_2(phoneid, method, arg1, arg2, arg3, arg4, res1, res2) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), 0, \
	    (res1), (res2), 0, 0, 0)
#define ipc_call_sync_4_3(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), 0, (res1), (res2), (res3), 0, 0)
#define ipc_call_sync_4_4(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3, res4) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), 0, (res1), (res2), (res3), (res4), 0)
#define ipc_call_sync_4_5(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3, res4, res5) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), 0, (res1), (res2), (res3), (res4), (res5))

#define ipc_call_sync_5_0(phoneid, method, arg1, arg2, arg3, arg4, arg5) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), 0, 0, 0, 0, 0)
#define ipc_call_sync_5_1(phoneid, method, arg1, arg2, arg3, arg4, arg5, res1) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (res1), 0, 0, 0, 0)
#define ipc_call_sync_5_2(phoneid, method, arg1, arg2, arg3, arg4, arg5, res1, \
    res2) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (res1), (res2), 0, 0, 0)
#define ipc_call_sync_5_3(phoneid, method, arg1, arg2, arg3, arg4, arg5, res1, \
    res2, res3) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (res1), (res2), (res3), 0, 0)
#define ipc_call_sync_5_4(phoneid, method, arg1, arg2, arg3, arg4, arg5, res1, \
    res2, res3, res4) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (res1), (res2), (res3), (res4), 0)
#define ipc_call_sync_5_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, res1, \
    res2, res3, res4, res5) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (res1), (res2), (res3), (res4), (res5))

extern int ipc_call_sync_fast(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);

extern int ipc_call_sync_slow(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);

extern ipc_callid_t ipc_wait_cycle(ipc_call_t *, sysarg_t, unsigned int);
extern void ipc_poke(void);

#define ipc_wait_for_call(data) \
	ipc_wait_for_call_timeout(data, SYNCH_NO_TIMEOUT);

extern ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *, sysarg_t);
extern ipc_callid_t ipc_trywait_for_call(ipc_call_t *);

/*
 * User-friendly wrappers for ipc_answer_fast() and ipc_answer_slow().
 * They are in the form of ipc_answer_m(), where m is the number of return
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_answer_0(callid, retval) \
	ipc_answer_fast((callid), (retval), 0, 0, 0, 0)
#define ipc_answer_1(callid, retval, arg1) \
	ipc_answer_fast((callid), (retval), (arg1), 0, 0, 0)
#define ipc_answer_2(callid, retval, arg1, arg2) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), 0, 0)
#define ipc_answer_3(callid, retval, arg1, arg2, arg3) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), (arg3), 0)
#define ipc_answer_4(callid, retval, arg1, arg2, arg3, arg4) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), (arg3), (arg4))
#define ipc_answer_5(callid, retval, arg1, arg2, arg3, arg4, arg5) \
	ipc_answer_slow((callid), (retval), (arg1), (arg2), (arg3), (arg4), (arg5))

extern sysarg_t ipc_answer_fast(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern sysarg_t ipc_answer_slow(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for ipc_call_async_fast() and ipc_call_async_slow().
 * They are in the form of ipc_call_async_m(), where m is the number of payload
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_call_async_0(phoneid, method, private, callback, can_preempt) \
	ipc_call_async_fast((phoneid), (method), 0, 0, 0, 0, (private), \
	    (callback), (can_preempt))
#define ipc_call_async_1(phoneid, method, arg1, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), 0, 0, 0, (private), \
	    (callback), (can_preempt))
#define ipc_call_async_2(phoneid, method, arg1, arg2, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    (private), (callback), (can_preempt))
#define ipc_call_async_3(phoneid, method, arg1, arg2, arg3, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, \
	    (private), (callback), (can_preempt))
#define ipc_call_async_4(phoneid, method, arg1, arg2, arg3, arg4, private, \
    callback, can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (private), (callback), (can_preempt))
#define ipc_call_async_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, \
    private, callback, can_preempt) \
	ipc_call_async_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (private), (callback), (can_preempt))

extern void ipc_call_async_fast(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, void *, ipc_async_callback_t, bool);
extern void ipc_call_async_slow(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, void *, ipc_async_callback_t, bool);

extern int ipc_clone_establish(int);
extern int ipc_connect_to_me(int, sysarg_t, sysarg_t, sysarg_t, task_id_t *,
    sysarg_t *);
extern int ipc_connect_me_to(int, sysarg_t, sysarg_t, sysarg_t);
extern int ipc_connect_me_to_blocking(int, sysarg_t, sysarg_t, sysarg_t);

extern int ipc_hangup(int);

extern int ipc_forward_fast(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    unsigned int);
extern int ipc_forward_slow(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);

/*
 * User-friendly wrappers for ipc_share_in_start().
 */

#define ipc_share_in_start_0_0(phoneid, size, dst) \
	ipc_share_in_start((phoneid), (size), 0, NULL, (dst))
#define ipc_share_in_start_0_1(phoneid, size, flags, dst) \
	ipc_share_in_start((phoneid), (size), 0, (flags), (dst))
#define ipc_share_in_start_1_0(phoneid, size, arg, dst) \
	ipc_share_in_start((phoneid), (size), (arg), NULL, (dst))
#define ipc_share_in_start_1_1(phoneid, size, arg, flags, dst) \
	ipc_share_in_start((phoneid), (size), (arg), (flags), (dst))

extern int ipc_share_in_start(int, size_t, sysarg_t, unsigned int *, void **);
extern int ipc_share_in_finalize(ipc_callid_t, void *, unsigned int);
extern int ipc_share_out_start(int, void *, unsigned int);
extern int ipc_share_out_finalize(ipc_callid_t, void **);
extern int ipc_data_read_start(int, void *, size_t);
extern int ipc_data_read_finalize(ipc_callid_t, const void *, size_t);
extern int ipc_data_write_start(int, const void *, size_t);
extern int ipc_data_write_finalize(ipc_callid_t, void *, size_t);

extern int ipc_connect_kbox(task_id_t);

#endif

/** @}
 */
