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

#if ((defined(LIBC_IPC_H_)) && (!defined(LIBC_ASYNC_C_)))
	#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef LIBC_ASYNC_H_
#define LIBC_ASYNC_H_

#include <ipc/common.h>
#include <async_sess.h>
#include <fibril.h>
#include <sys/time.h>
#include <atomic.h>
#include <bool.h>
#include <task.h>

typedef ipc_callid_t aid_t;

typedef void *(*async_client_data_ctor_t)(void);
typedef void (*async_client_data_dtor_t)(void *);

typedef void (*async_client_conn_t)(ipc_callid_t, ipc_call_t *);

extern atomic_t threads_in_ipc_wait;

#define async_manager() \
	fibril_switch(FIBRIL_TO_MANAGER)

#define async_get_call(data) \
	async_get_call_timeout(data, 0)

extern ipc_callid_t async_get_call_timeout(ipc_call_t *, suseconds_t);

/*
 * User-friendly wrappers for async_send_fast() and async_send_slow(). The
 * macros are in the form async_send_m(), where m denotes the number of payload
 * arguments.  Each macros chooses between the fast and the slow version based
 * on m.
 */

#define async_send_0(phoneid, method, dataptr) \
	async_send_fast((phoneid), (method), 0, 0, 0, 0, (dataptr))
#define async_send_1(phoneid, method, arg1, dataptr) \
	async_send_fast((phoneid), (method), (arg1), 0, 0, 0, (dataptr))
#define async_send_2(phoneid, method, arg1, arg2, dataptr) \
	async_send_fast((phoneid), (method), (arg1), (arg2), 0, 0, (dataptr))
#define async_send_3(phoneid, method, arg1, arg2, arg3, dataptr) \
	async_send_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (dataptr))
#define async_send_4(phoneid, method, arg1, arg2, arg3, arg4, dataptr) \
	async_send_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (dataptr))
#define async_send_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, dataptr) \
	async_send_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (dataptr))

extern aid_t async_send_fast(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, ipc_call_t *);
extern aid_t async_send_slow(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);
extern void async_wait_for(aid_t, sysarg_t *);
extern int async_wait_timeout(aid_t, sysarg_t *, suseconds_t);

extern fid_t async_new_connection(sysarg_t, sysarg_t, ipc_callid_t,
    ipc_call_t *, void (*)(ipc_callid_t, ipc_call_t *));
extern void async_usleep(suseconds_t);
extern void async_create_manager(void);
extern void async_destroy_manager(void);

extern void async_set_client_data_constructor(async_client_data_ctor_t);
extern void async_set_client_data_destructor(async_client_data_dtor_t);

extern void *async_client_data_get(void);

extern void async_set_client_connection(async_client_conn_t);
extern void async_set_interrupt_received(async_client_conn_t);

/*
 * Wrappers for simple communication.
 */

extern void async_msg_0(int, sysarg_t);
extern void async_msg_1(int, sysarg_t, sysarg_t);
extern void async_msg_2(int, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_3(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_4(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_5(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

/*
 * Wrappers for answer routines.
 */

extern sysarg_t async_answer_0(ipc_callid_t, sysarg_t);
extern sysarg_t async_answer_1(ipc_callid_t, sysarg_t, sysarg_t);
extern sysarg_t async_answer_2(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t);
extern sysarg_t async_answer_3(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern sysarg_t async_answer_4(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern sysarg_t async_answer_5(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * Wrappers for forwarding routines.
 */

extern int async_forward_fast(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    unsigned int);
extern int async_forward_slow(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);

/*
 * User-friendly wrappers for async_req_fast() and async_req_slow(). The macros
 * are in the form async_req_m_n(), where m is the number of payload arguments
 * and n is the number of return arguments. The macros decide between the fast
 * and slow verion based on m.
 */

#define async_req_0_0(phoneid, method) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, NULL, NULL, NULL, NULL, \
	    NULL)
#define async_req_0_1(phoneid, method, r1) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), NULL, NULL, NULL, \
	    NULL)
#define async_req_0_2(phoneid, method, r1, r2) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), NULL, NULL, \
	    NULL)
#define async_req_0_3(phoneid, method, r1, r2, r3) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), NULL, \
	    NULL)
#define async_req_0_4(phoneid, method, r1, r2, r3, r4) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), (r4), \
	    NULL)
#define async_req_0_5(phoneid, method, r1, r2, r3, r4, r5) \
	async_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), (r4), \
	    (r5))
#define async_req_1_0(phoneid, method, arg1) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, NULL, NULL, NULL, \
	    NULL, NULL)
#define async_req_1_1(phoneid, method, arg1, rc1) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), NULL, NULL, \
	    NULL, NULL)
#define async_req_1_2(phoneid, method, arg1, rc1, rc2) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), NULL, \
	    NULL, NULL)
#define async_req_1_3(phoneid, method, arg1, rc1, rc2, rc3) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    NULL, NULL)
#define async_req_1_4(phoneid, method, arg1, rc1, rc2, rc3, rc4) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    (rc4), NULL)
#define async_req_1_5(phoneid, method, arg1, rc1, rc2, rc3, rc4, rc5) \
	async_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    (rc4), (rc5))
#define async_req_2_0(phoneid, method, arg1, arg2) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, NULL, NULL, \
	    NULL, NULL, NULL)
#define async_req_2_1(phoneid, method, arg1, arg2, rc1) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), NULL, \
	    NULL, NULL, NULL)
#define async_req_2_2(phoneid, method, arg1, arg2, rc1, rc2) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    NULL, NULL, NULL)
#define async_req_2_3(phoneid, method, arg1, arg2, rc1, rc2, rc3) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), NULL, NULL)
#define async_req_2_4(phoneid, method, arg1, arg2, rc1, rc2, rc3, rc4) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), (rc4), NULL)
#define async_req_2_5(phoneid, method, arg1, arg2, rc1, rc2, rc3, rc4, rc5) \
	async_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), (rc4), (rc5))
#define async_req_3_0(phoneid, method, arg1, arg2, arg3) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, NULL, NULL, \
	    NULL, NULL, NULL)
#define async_req_3_1(phoneid, method, arg1, arg2, arg3, rc1) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    NULL, NULL, NULL, NULL)
#define async_req_3_2(phoneid, method, arg1, arg2, arg3, rc1, rc2) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), NULL, NULL, NULL)
#define async_req_3_3(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), NULL, NULL)
#define async_req_3_4(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), (rc4), NULL)
#define async_req_3_5(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4, \
    rc5) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), (rc4), (rc5))
#define async_req_4_0(phoneid, method, arg1, arg2, arg3, arg4) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), NULL, \
	    NULL, NULL, NULL, NULL)
#define async_req_4_1(phoneid, method, arg1, arg2, arg3, arg4, rc1) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    NULL, NULL, NULL, NULL)
#define async_req_4_2(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    (rc2), NULL, NULL, NULL)
#define async_req_4_3(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    (rc2), (rc3), NULL, NULL)
#define async_req_4_4(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (rc1), (rc2), (rc3), (rc4), NULL)
#define async_req_4_5(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4, rc5) \
	async_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (rc1), (rc2), (rc3), (rc4), (rc5))
#define async_req_5_0(phoneid, method, arg1, arg2, arg3, arg4, arg5) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), NULL, NULL, NULL, NULL, NULL)
#define async_req_5_1(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), NULL, NULL, NULL, NULL)
#define async_req_5_2(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), NULL, NULL, NULL)
#define async_req_5_3(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), NULL, NULL)
#define async_req_5_4(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), (rc4), NULL)
#define async_req_5_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4, rc5) \
	async_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), (rc4), (rc5))

extern sysarg_t async_req_fast(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern sysarg_t async_req_slow(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);

static inline void async_serialize_start(void)
{
	fibril_inc_sercount();
}

static inline void async_serialize_end(void)
{
	fibril_dec_sercount();
}

extern int async_connect_to_me(int, sysarg_t, sysarg_t, sysarg_t,
    async_client_conn_t);
extern int async_connect_me_to(int, sysarg_t, sysarg_t, sysarg_t);
extern int async_connect_me_to_blocking(int, sysarg_t, sysarg_t, sysarg_t);
extern int async_connect_kbox(task_id_t);
extern int async_hangup(int);
extern void async_poke(void);

/*
 * User-friendly wrappers for async_share_in_start().
 */

#define async_share_in_start_0_0(phoneid, dst, size) \
	async_share_in_start((phoneid), (dst), (size), 0, NULL)
#define async_share_in_start_0_1(phoneid, dst, size, flags) \
	async_share_in_start((phoneid), (dst), (size), 0, (flags))
#define async_share_in_start_1_0(phoneid, dst, size, arg) \
	async_share_in_start((phoneid), (dst), (size), (arg), NULL)
#define async_share_in_start_1_1(phoneid, dst, size, arg, flags) \
	async_share_in_start((phoneid), (dst), (size), (arg), (flags))

extern int async_share_in_start(int, void *, size_t, sysarg_t, unsigned int *);
extern bool async_share_in_receive(ipc_callid_t *, size_t *);
extern int async_share_in_finalize(ipc_callid_t, void *, unsigned int);

extern int async_share_out_start(int, void *, unsigned int);
extern bool async_share_out_receive(ipc_callid_t *, size_t *, unsigned int *);
extern int async_share_out_finalize(ipc_callid_t, void *);

/*
 * User-friendly wrappers for async_data_read_forward_fast().
 */

#define async_data_read_forward_0_0(phoneid, method, answer) \
	async_data_read_forward_fast((phoneid), (method), 0, 0, 0, 0, NULL)
#define async_data_read_forward_0_1(phoneid, method, answer) \
	async_data_read_forward_fast((phoneid), (method), 0, 0, 0, 0, (answer))
#define async_data_read_forward_1_0(phoneid, method, arg1, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), 0, 0, 0, NULL)
#define async_data_read_forward_1_1(phoneid, method, arg1, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), 0, 0, 0, (answer))
#define async_data_read_forward_2_0(phoneid, method, arg1, arg2, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, NULL)
#define async_data_read_forward_2_1(phoneid, method, arg1, arg2, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    (answer))
#define async_data_read_forward_3_0(phoneid, method, arg1, arg2, arg3, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, \
	    NULL)
#define async_data_read_forward_3_1(phoneid, method, arg1, arg2, arg3, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, \
	    (answer))
#define async_data_read_forward_4_0(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), NULL)
#define async_data_read_forward_4_1(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_data_read_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (answer))

extern aid_t async_data_read(int, void *, size_t, ipc_call_t *);
extern int async_data_read_start(int, void *, size_t);
extern bool async_data_read_receive(ipc_callid_t *, size_t *);
extern int async_data_read_finalize(ipc_callid_t, const void *, size_t);

extern int async_data_read_forward_fast(int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);

/*
 * User-friendly wrappers for async_data_write_forward_fast().
 */

#define async_data_write_forward_0_0(phoneid, method, answer) \
	async_data_write_forward_fast((phoneid), (method), 0, 0, 0, 0, NULL)
#define async_data_write_forward_0_1(phoneid, method, answer) \
	async_data_write_forward_fast((phoneid), (method), 0, 0, 0, 0, (answer))
#define async_data_write_forward_1_0(phoneid, method, arg1, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), 0, 0, 0, NULL)
#define async_data_write_forward_1_1(phoneid, method, arg1, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), 0, 0, 0, \
	    (answer))
#define async_data_write_forward_2_0(phoneid, method, arg1, arg2, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    NULL)
#define async_data_write_forward_2_1(phoneid, method, arg1, arg2, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    (answer))
#define async_data_write_forward_3_0(phoneid, method, arg1, arg2, arg3, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    0, NULL)
#define async_data_write_forward_3_1(phoneid, method, arg1, arg2, arg3, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    0, (answer))
#define async_data_write_forward_4_0(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), NULL)
#define async_data_write_forward_4_1(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (answer))

extern int async_data_write_start(int, const void *, size_t);
extern bool async_data_write_receive(ipc_callid_t *, size_t *);
extern int async_data_write_finalize(ipc_callid_t, void *, size_t);

extern int async_data_write_accept(void **, const bool, const size_t,
    const size_t, const size_t, size_t *);
extern void async_data_write_void(sysarg_t);

extern int async_data_write_forward_fast(int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);

#endif

/** @}
 */
