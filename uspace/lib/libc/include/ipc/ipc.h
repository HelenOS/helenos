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

#ifndef LIBIPC_IPC_H_
#define LIBIPC_IPC_H_

#include <kernel/ipc/ipc.h>
#include <kernel/ddi/irq.h>
#include <sys/types.h>
#include <kernel/synch/synch.h>

typedef sysarg_t ipcarg_t;
typedef struct {
	ipcarg_t args[IPC_CALL_LEN];
	ipcarg_t in_phone_hash;
} ipc_call_t;
typedef sysarg_t ipc_callid_t;

typedef void (* ipc_async_callback_t)(void *private, int retval,
    ipc_call_t *data);

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
    ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	0, 0, 0, 0, 0)
#define ipc_call_sync_4_1(phoneid, method, arg1, arg2, arg3, arg4, res1) \
    ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	(res1), 0, 0, 0, 0)
#define ipc_call_sync_4_2(phoneid, method, arg1, arg2, arg3, arg4, res1, res2) \
    ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	(res1), (res2), 0, 0, 0)
#define ipc_call_sync_4_3(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (res1), (res2), (res3), 0, 0)
#define ipc_call_sync_4_4(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3, res4) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (res1), (res2), (res3), (res4), 0)
#define ipc_call_sync_4_5(phoneid, method, arg1, arg2, arg3, arg4, res1, res2, \
    res3, res4, res5) \
	ipc_call_sync_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (res1), (res2), (res3), (res4), (res5))
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

extern int ipc_call_sync_fast(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t *result1, ipcarg_t *result2,
    ipcarg_t *result3, ipcarg_t *result4, ipcarg_t *result5);

extern int ipc_call_sync_slow(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5,
    ipcarg_t *result1, ipcarg_t *result2, ipcarg_t *result3, ipcarg_t *result4,
    ipcarg_t *result5);

extern ipc_callid_t ipc_wait_cycle(ipc_call_t *call, uint32_t usec, int flags);
extern ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *data, uint32_t usec);
static inline ipc_callid_t ipc_wait_for_call(ipc_call_t *data)
{
	return ipc_wait_for_call_timeout(data, SYNCH_NO_TIMEOUT);
}
extern ipc_callid_t ipc_trywait_for_call(ipc_call_t *data);

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

extern ipcarg_t ipc_answer_fast(ipc_callid_t callid, ipcarg_t retval,
    ipcarg_t arg1, ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4);
extern ipcarg_t ipc_answer_slow(ipc_callid_t callid, ipcarg_t retval,
    ipcarg_t arg1, ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5);

/*
 * User-friendly wrappers for ipc_call_async_fast() and ipc_call_async_slow().
 * They are in the form of ipc_call_async_m(), where m is the number of payload
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */
#define ipc_call_async_0(phoneid, method, private, callback, \
    can_preempt) \
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

extern void ipc_call_async_fast(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, void *private,
    ipc_async_callback_t callback, int can_preempt);
extern void ipc_call_async_slow(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5, void *private,
    ipc_async_callback_t callback, int can_preempt);

extern int ipc_connect_to_me(int phoneid, int arg1, int arg2, int arg3,
    ipcarg_t *phone);
extern int ipc_connect_me_to(int phoneid, int arg1, int arg2, int arg3);
extern int ipc_hangup(int phoneid);
extern int ipc_register_irq(int inr, int devno, int method, irq_code_t *code);
extern int ipc_unregister_irq(int inr, int devno);
extern int ipc_forward_fast(ipc_callid_t callid, int phoneid, int method,
    ipcarg_t arg1, ipcarg_t arg2, int mode);
extern int ipc_data_write_send(int phoneid, void *src, size_t size);
extern int ipc_data_write_receive(ipc_callid_t *callid, void **dst,
    size_t *size);
extern ipcarg_t ipc_data_write_deliver(ipc_callid_t callid, void *dst,
    size_t size);

#endif

/** @}
 */
