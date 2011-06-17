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

#if ((defined(LIBC_IPC_H_)) && (!defined(LIBC_ASYNC_OBSOLETE_C_)))
	#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef LIBC_ASYNC_OBSOLETE_H_
#define LIBC_ASYNC_OBSOLETE_H_

extern void async_obsolete_serialize_start(void);
extern void async_obsolete_serialize_end(void);

#define async_obsolete_send_0(phoneid, method, dataptr) \
	async_obsolete_send_fast((phoneid), (method), 0, 0, 0, 0, (dataptr))
#define async_obsolete_send_1(phoneid, method, arg1, dataptr) \
	async_obsolete_send_fast((phoneid), (method), (arg1), 0, 0, 0, (dataptr))
#define async_obsolete_send_2(phoneid, method, arg1, arg2, dataptr) \
	async_obsolete_send_fast((phoneid), (method), (arg1), (arg2), 0, 0, (dataptr))
#define async_obsolete_send_3(phoneid, method, arg1, arg2, arg3, dataptr) \
	async_obsolete_send_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (dataptr))
#define async_obsolete_send_4(phoneid, method, arg1, arg2, arg3, arg4, dataptr) \
	async_obsolete_send_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (dataptr))
#define async_obsolete_send_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, dataptr) \
	async_obsolete_send_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (dataptr))

extern aid_t async_obsolete_send_fast(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr);
extern aid_t async_obsolete_send_slow(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    ipc_call_t *dataptr);

#define async_obsolete_req_0_0(phoneid, method) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, NULL, NULL, NULL, NULL, \
	    NULL)
#define async_obsolete_req_0_1(phoneid, method, r1) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), NULL, NULL, NULL, \
	    NULL)
#define async_obsolete_req_0_2(phoneid, method, r1, r2) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), NULL, NULL, \
	    NULL)
#define async_obsolete_req_0_3(phoneid, method, r1, r2, r3) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), NULL, \
	    NULL)
#define async_obsolete_req_0_4(phoneid, method, r1, r2, r3, r4) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), (r4), \
	    NULL)
#define async_obsolete_req_0_5(phoneid, method, r1, r2, r3, r4, r5) \
	async_obsolete_req_fast((phoneid), (method), 0, 0, 0, 0, (r1), (r2), (r3), (r4), \
	    (r5))
#define async_obsolete_req_1_0(phoneid, method, arg1) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, NULL, NULL, NULL, \
	    NULL, NULL)
#define async_obsolete_req_1_1(phoneid, method, arg1, rc1) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), NULL, NULL, \
	    NULL, NULL)
#define async_obsolete_req_1_2(phoneid, method, arg1, rc1, rc2) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), NULL, \
	    NULL, NULL)
#define async_obsolete_req_1_3(phoneid, method, arg1, rc1, rc2, rc3) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    NULL, NULL)
#define async_obsolete_req_1_4(phoneid, method, arg1, rc1, rc2, rc3, rc4) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    (rc4), NULL)
#define async_obsolete_req_1_5(phoneid, method, arg1, rc1, rc2, rc3, rc4, rc5) \
	async_obsolete_req_fast((phoneid), (method), (arg1), 0, 0, 0, (rc1), (rc2), (rc3), \
	    (rc4), (rc5))
#define async_obsolete_req_2_0(phoneid, method, arg1, arg2) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, NULL, NULL, \
	    NULL, NULL, NULL)
#define async_obsolete_req_2_1(phoneid, method, arg1, arg2, rc1) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), NULL, \
	    NULL, NULL, NULL)
#define async_obsolete_req_2_2(phoneid, method, arg1, arg2, rc1, rc2) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    NULL, NULL, NULL)
#define async_obsolete_req_2_3(phoneid, method, arg1, arg2, rc1, rc2, rc3) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), NULL, NULL)
#define async_obsolete_req_2_4(phoneid, method, arg1, arg2, rc1, rc2, rc3, rc4) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), (rc4), NULL)
#define async_obsolete_req_2_5(phoneid, method, arg1, arg2, rc1, rc2, rc3, rc4, rc5) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), 0, 0, (rc1), (rc2), \
	    (rc3), (rc4), (rc5))
#define async_obsolete_req_3_0(phoneid, method, arg1, arg2, arg3) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, NULL, NULL, \
	    NULL, NULL, NULL)
#define async_obsolete_req_3_1(phoneid, method, arg1, arg2, arg3, rc1) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    NULL, NULL, NULL, NULL)
#define async_obsolete_req_3_2(phoneid, method, arg1, arg2, arg3, rc1, rc2) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), NULL, NULL, NULL)
#define async_obsolete_req_3_3(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), NULL, NULL)
#define async_obsolete_req_3_4(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), (rc4), NULL)
#define async_obsolete_req_3_5(phoneid, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4, \
    rc5) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, (rc1), \
	    (rc2), (rc3), (rc4), (rc5))
#define async_obsolete_req_4_0(phoneid, method, arg1, arg2, arg3, arg4) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), NULL, \
	    NULL, NULL, NULL, NULL)
#define async_obsolete_req_4_1(phoneid, method, arg1, arg2, arg3, arg4, rc1) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    NULL, NULL, NULL, NULL)
#define async_obsolete_req_4_2(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    (rc2), NULL, NULL, NULL)
#define async_obsolete_req_4_3(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), (rc1), \
	    (rc2), (rc3), NULL, NULL)
#define async_obsolete_req_4_4(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (rc1), (rc2), (rc3), (rc4), NULL)
#define async_obsolete_req_4_5(phoneid, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4, rc5) \
	async_obsolete_req_fast((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (rc1), (rc2), (rc3), (rc4), (rc5))
#define async_obsolete_req_5_0(phoneid, method, arg1, arg2, arg3, arg4, arg5) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), NULL, NULL, NULL, NULL, NULL)
#define async_obsolete_req_5_1(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), NULL, NULL, NULL, NULL)
#define async_obsolete_req_5_2(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), NULL, NULL, NULL)
#define async_obsolete_req_5_3(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), NULL, NULL)
#define async_obsolete_req_5_4(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), (rc4), NULL)
#define async_obsolete_req_5_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4, rc5) \
	async_obsolete_req_slow((phoneid), (method), (arg1), (arg2), (arg3), (arg4), \
	    (arg5), (rc1), (rc2), (rc3), (rc4), (rc5))

extern sysarg_t async_obsolete_req_fast(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4, sysarg_t *r5);
extern sysarg_t async_obsolete_req_slow(int phoneid, sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5);

extern void async_obsolete_msg_0(int, sysarg_t);
extern void async_obsolete_msg_1(int, sysarg_t, sysarg_t);
extern void async_obsolete_msg_2(int, sysarg_t, sysarg_t, sysarg_t);
extern void async_obsolete_msg_3(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_obsolete_msg_4(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_obsolete_msg_5(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

extern int async_obsolete_forward_slow(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);

extern int async_obsolete_connect_to_me(int, sysarg_t, sysarg_t, sysarg_t,
    async_client_conn_t, void *);
extern int async_obsolete_connect_me_to(int, sysarg_t, sysarg_t, sysarg_t);
extern int async_obsolete_connect_me_to_blocking(int, sysarg_t, sysarg_t, sysarg_t);
extern int async_obsolete_hangup(int);

#define async_obsolete_share_in_start_0_0(phoneid, dst, size) \
	async_obsolete_share_in_start((phoneid), (dst), (size), 0, NULL)
#define async_obsolete_share_in_start_0_1(phoneid, dst, size, flags) \
	async_obsolete_share_in_start((phoneid), (dst), (size), 0, (flags))
#define async_obsolete_share_in_start_1_0(phoneid, dst, size, arg) \
	async_obsolete_share_in_start((phoneid), (dst), (size), (arg), NULL)
#define async_obsolete_share_in_start_1_1(phoneid, dst, size, arg, flags) \
	async_obsolete_share_in_start((phoneid), (dst), (size), (arg), (flags))

extern int async_obsolete_share_in_start(int, void *, size_t, sysarg_t,
    unsigned int *);
extern int async_obsolete_share_out_start(int, void *, unsigned int);

#define async_obsolete_data_write_start(p, buf, len) \
	async_obsolete_data_write_start_generic((p), (buf), (len), IPC_XF_NONE)

#define async_obsolete_data_read_start(p, buf, len) \
	async_obsolete_data_read_start_generic((p), (buf), (len), IPC_XF_NONE)

#define async_obsolete_data_write_forward_0_0(phoneid, method, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), 0, 0, 0, 0, NULL)
#define async_obsolete_data_write_forward_0_1(phoneid, method, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), 0, 0, 0, 0, (answer))
#define async_obsolete_data_write_forward_1_0(phoneid, method, arg1, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), 0, 0, 0, NULL)
#define async_obsolete_data_write_forward_1_1(phoneid, method, arg1, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), 0, 0, 0, \
	    (answer))
#define async_obsolete_data_write_forward_2_0(phoneid, method, arg1, arg2, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    NULL)
#define async_obsolete_data_write_forward_2_1(phoneid, method, arg1, arg2, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    (answer))
#define async_obsolete_data_write_forward_3_0(phoneid, method, arg1, arg2, arg3, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    0, NULL)
#define async_obsolete_data_write_forward_3_1(phoneid, method, arg1, arg2, arg3, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    0, (answer))
#define async_obsolete_data_write_forward_4_0(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), NULL)
#define async_obsolete_data_write_forward_4_1(phoneid, method, arg1, arg2, arg3, arg4, answer) \
	async_obsolete_data_write_forward_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (answer))

extern aid_t async_obsolete_data_read(int, void *, size_t, ipc_call_t *);
extern int async_obsolete_data_read_start_generic(int, void *, size_t, int);

extern int async_obsolete_data_write_start_generic(int, const void *, size_t, int);
extern void async_obsolete_data_write_void(const int);

extern int async_obsolete_forward_fast(ipc_callid_t, int, sysarg_t, sysarg_t,
    sysarg_t, unsigned int);

extern int async_obsolete_data_write_forward_fast(int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);

#endif

/** @}
 */
