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

#if ((defined(_LIBC_IPC_H_)) && (!defined(_LIBC_ASYNC_C_)))
#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef _LIBC_ASYNC_H_
#define _LIBC_ASYNC_H_

#include <ipc/common.h>
#include <time.h>
#include <stdbool.h>
#include <abi/proc/task.h>
#include <abi/ddi/irq.h>
#include <abi/ipc/event.h>
#include <abi/ipc/interfaces.h>
#include <abi/cap.h>

#include <_bits/__noreturn.h>

typedef sysarg_t aid_t;
typedef sysarg_t port_id_t;

typedef void *(*async_client_data_ctor_t)(void);
typedef void (*async_client_data_dtor_t)(void *);

/** Port connection handler
 *
 * @param call Incoming call or NULL if connection initiated from inside
 *             using async_create_callback_port()
 * @param arg  Local argument.
 *
 */
typedef void (*async_port_handler_t)(ipc_call_t *, void *);

/** Notification handler */
typedef void (*async_notification_handler_t)(ipc_call_t *, void *);

/** Exchange management style
 *
 */
typedef enum {
	/** No explicit exchange management
	 *
	 * Suitable for protocols which use a single
	 * IPC message per exchange only.
	 *
	 */
	EXCHANGE_ATOMIC = 0,

	/** Exchange management via mutual exclusion
	 *
	 * Suitable for any kind of client/server communication,
	 * but can limit parallelism.
	 *
	 */
	EXCHANGE_SERIALIZE = 1,

	/** Exchange management via phone cloning
	 *
	 * Suitable for servers which support client
	 * data tracking by task hashes and do not
	 * mind cloned phones.
	 *
	 */
	EXCHANGE_PARALLEL = 2
} exch_mgmt_t;

/** Forward declarations */
struct async_exch;
struct async_sess;

typedef struct async_sess async_sess_t;
typedef struct async_exch async_exch_t;

extern __noreturn void async_manager(void);

extern bool async_get_call(ipc_call_t *);
extern bool async_get_call_timeout(ipc_call_t *, usec_t);

extern aid_t async_send_0(async_exch_t *, sysarg_t, ipc_call_t *);
extern aid_t async_send_1(async_exch_t *, sysarg_t, sysarg_t, ipc_call_t *);
extern aid_t async_send_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    ipc_call_t *);
extern aid_t async_send_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, ipc_call_t *);
extern aid_t async_send_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);
extern aid_t async_send_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

extern void async_wait_for(aid_t, errno_t *);
extern errno_t async_wait_timeout(aid_t, errno_t *, usec_t);
extern void async_forget(aid_t);

extern void async_set_client_data_constructor(async_client_data_ctor_t);
extern void async_set_client_data_destructor(async_client_data_dtor_t);
extern void *async_get_client_data(void);
extern void *async_get_client_data_by_id(task_id_t);
extern void async_put_client_data_by_id(task_id_t);

extern errno_t async_create_port(iface_t, async_port_handler_t, void *,
    port_id_t *);
extern void async_set_fallback_port_handler(async_port_handler_t, void *);
extern errno_t async_create_callback_port(async_exch_t *, iface_t, sysarg_t,
    sysarg_t, async_port_handler_t, void *, port_id_t *);

extern errno_t async_irq_subscribe(int, async_notification_handler_t, void *,
    const irq_code_t *, cap_irq_handle_t *);
extern errno_t async_irq_unsubscribe(cap_irq_handle_t);

extern errno_t async_event_subscribe(event_type_t, async_notification_handler_t,
    void *);
extern errno_t async_event_task_subscribe(event_task_type_t,
    async_notification_handler_t, void *);
extern errno_t async_event_unsubscribe(event_type_t);
extern errno_t async_event_task_unsubscribe(event_task_type_t);
extern errno_t async_event_unmask(event_type_t);
extern errno_t async_event_task_unmask(event_task_type_t);

/*
 * Wrappers for simple communication.
 */

extern void async_msg_0(async_exch_t *, sysarg_t);
extern void async_msg_1(async_exch_t *, sysarg_t, sysarg_t);
extern void async_msg_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern void async_msg_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);

/*
 * Wrappers for answer routines.
 */

extern errno_t async_answer_0(ipc_call_t *, errno_t);
extern errno_t async_answer_1(ipc_call_t *, errno_t, sysarg_t);
extern errno_t async_answer_2(ipc_call_t *, errno_t, sysarg_t, sysarg_t);
extern errno_t async_answer_3(ipc_call_t *, errno_t, sysarg_t, sysarg_t,
    sysarg_t);
extern errno_t async_answer_4(ipc_call_t *, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t async_answer_5(ipc_call_t *, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * Wrappers for forwarding routines.
 */

extern errno_t async_forward_0(ipc_call_t *, async_exch_t *, sysarg_t,
    unsigned int);
extern errno_t async_forward_1(ipc_call_t *, async_exch_t *, sysarg_t,
    sysarg_t, unsigned int);
extern errno_t async_forward_2(ipc_call_t *, async_exch_t *, sysarg_t,
    sysarg_t, sysarg_t, unsigned int);
extern errno_t async_forward_3(ipc_call_t *, async_exch_t *, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);
extern errno_t async_forward_4(ipc_call_t *, async_exch_t *, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, unsigned int);
extern errno_t async_forward_5(ipc_call_t *, async_exch_t *, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t, unsigned int);

/*
 * User-friendly wrappers for async_req_*_*().
 * The functions are in the form async_req_m_n(), where m is the number of
 * payload arguments and n is the number of return arguments.
 */

extern errno_t async_req_0_0(async_exch_t *, sysarg_t);
extern errno_t async_req_0_1(async_exch_t *, sysarg_t, sysarg_t *);
extern errno_t async_req_0_2(async_exch_t *, sysarg_t, sysarg_t *, sysarg_t *);
extern errno_t async_req_0_3(async_exch_t *, sysarg_t, sysarg_t *, sysarg_t *,
    sysarg_t *);
extern errno_t async_req_0_4(async_exch_t *, sysarg_t, sysarg_t *, sysarg_t *,
    sysarg_t *, sysarg_t *);
extern errno_t async_req_0_5(async_exch_t *, sysarg_t, sysarg_t *, sysarg_t *,
    sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_1_0(async_exch_t *, sysarg_t, sysarg_t);
extern errno_t async_req_1_1(async_exch_t *, sysarg_t, sysarg_t, sysarg_t *);
extern errno_t async_req_1_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t *,
    sysarg_t *);
extern errno_t async_req_1_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t *,
    sysarg_t *, sysarg_t *);
extern errno_t async_req_1_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t *,
    sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_1_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t *,
    sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_2_0(async_exch_t *, sysarg_t, sysarg_t, sysarg_t);
extern errno_t async_req_2_1(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *);
extern errno_t async_req_2_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *, sysarg_t *);
extern errno_t async_req_2_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_2_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_2_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_3_0(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern errno_t async_req_3_1(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *);
extern errno_t async_req_3_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *, sysarg_t *);
extern errno_t async_req_3_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_3_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_3_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_4_0(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t async_req_4_1(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *);
extern errno_t async_req_4_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *);
extern errno_t async_req_4_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_4_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_4_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);
extern errno_t async_req_5_0(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);
extern errno_t async_req_5_1(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *);
extern errno_t async_req_5_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *, sysarg_t *);
extern errno_t async_req_5_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *);
extern errno_t async_req_5_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);
extern errno_t async_req_5_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *, sysarg_t *);

extern errno_t async_accept_0(ipc_call_t *);
extern sysarg_t async_get_label(void);

extern async_sess_t *async_connect_me_to(async_exch_t *, iface_t, sysarg_t,
    sysarg_t, errno_t *);
extern async_sess_t *async_connect_me_to_blocking(async_exch_t *, iface_t,
    sysarg_t, sysarg_t, errno_t *);
extern async_sess_t *async_connect_kbox(task_id_t, errno_t *);

extern errno_t async_connect_to_me(async_exch_t *, iface_t, sysarg_t, sysarg_t);

extern void async_hangup(async_sess_t *);

extern async_exch_t *async_exchange_begin(async_sess_t *);
extern void async_exchange_end(async_exch_t *);

/*
 * FIXME These functions just work around problems with parallel exchange
 * management. Proper solution needs to be implemented.
 */
extern void async_sess_args_set(async_sess_t *, iface_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for async_share_in_start().
 */

extern errno_t async_share_in_start_0_0(async_exch_t *, size_t, void **);
extern errno_t async_share_in_start_0_1(async_exch_t *, size_t,
    unsigned int *, void **);
extern errno_t async_share_in_start_1_0(async_exch_t *, size_t, sysarg_t,
    void **);
extern errno_t async_share_in_start_1_1(async_exch_t *, size_t, sysarg_t,
    unsigned int *, void **);

extern bool async_share_in_receive(ipc_call_t *, size_t *);
extern errno_t async_share_in_finalize(ipc_call_t *, void *, unsigned int);

extern errno_t async_share_out_start(async_exch_t *, void *, unsigned int);
extern bool async_share_out_receive(ipc_call_t *, size_t *, unsigned int *);
extern errno_t async_share_out_finalize(ipc_call_t *, void **);

extern errno_t async_data_read_forward_0_0(async_exch_t *, sysarg_t);
extern errno_t async_data_read_forward_1_0(async_exch_t *, sysarg_t, sysarg_t);
extern errno_t async_data_read_forward_2_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t);
extern errno_t async_data_read_forward_3_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t async_data_read_forward_4_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

extern errno_t async_data_read_forward_0_1(async_exch_t *, sysarg_t,
    ipc_call_t *);
extern errno_t async_data_read_forward_1_1(async_exch_t *, sysarg_t, sysarg_t,
    ipc_call_t *);
extern errno_t async_data_read_forward_2_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, ipc_call_t *);
extern errno_t async_data_read_forward_3_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);
extern errno_t async_data_read_forward_4_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

extern aid_t async_data_read(async_exch_t *, void *, size_t, ipc_call_t *);
extern errno_t async_data_read_start(async_exch_t *, void *, size_t);
extern bool async_data_read_receive(ipc_call_t *, size_t *);
extern errno_t async_data_read_finalize(ipc_call_t *, const void *, size_t);

extern errno_t async_data_write_forward_0_0(async_exch_t *, sysarg_t);
extern errno_t async_data_write_forward_1_0(async_exch_t *, sysarg_t, sysarg_t);
extern errno_t async_data_write_forward_2_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t);
extern errno_t async_data_write_forward_3_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t async_data_write_forward_4_0(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

extern errno_t async_data_write_forward_0_1(async_exch_t *, sysarg_t,
    ipc_call_t *);
extern errno_t async_data_write_forward_1_1(async_exch_t *, sysarg_t, sysarg_t,
    ipc_call_t *);
extern errno_t async_data_write_forward_2_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, ipc_call_t *);
extern errno_t async_data_write_forward_3_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);
extern errno_t async_data_write_forward_4_1(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

extern errno_t async_data_write_start(async_exch_t *, const void *, size_t);
extern bool async_data_write_receive(ipc_call_t *, size_t *);
extern errno_t async_data_write_finalize(ipc_call_t *, void *, size_t);

extern errno_t async_data_write_accept(void **, const bool, const size_t,
    const size_t, const size_t, size_t *);
extern void async_data_write_void(errno_t);

extern async_sess_t *async_callback_receive(exch_mgmt_t);
extern async_sess_t *async_callback_receive_start(exch_mgmt_t, ipc_call_t *);

extern errno_t async_state_change_start(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, async_exch_t *);
extern bool async_state_change_receive(ipc_call_t *);
extern errno_t async_state_change_finalize(ipc_call_t *, async_exch_t *);

extern void *async_remote_state_acquire(async_sess_t *);
extern void async_remote_state_update(async_sess_t *, void *);
extern void async_remote_state_release(async_sess_t *);
extern void async_remote_state_release_exchange(async_exch_t *);

extern void *async_as_area_create(void *, size_t, unsigned int, async_sess_t *,
    sysarg_t, sysarg_t, sysarg_t);

errno_t async_spawn_notification_handler(void);

#endif

/** @}
 */
