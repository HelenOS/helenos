/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_ASYNC_H_
#define _LIBC_PRIVATE_ASYNC_H_

#include <async.h>
#include <adt/list.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <time.h>
#include <stdbool.h>

/** Session data */
struct async_sess {
	/** List of inactive exchanges */
	list_t exch_list;

	/** Session interface */
	iface_t iface;

	/** Exchange management style */
	exch_mgmt_t mgmt;

	/** Session identification */
	cap_phone_handle_t phone;

	/** First clone connection argument */
	iface_t arg1;

	/** Second clone connection argument */
	sysarg_t arg2;

	/** Third clone connection argument */
	sysarg_t arg3;

	/** Exchange mutex */
	fibril_mutex_t mutex;

	/** Number of opened exchanges */
	int exchanges;

	/** Mutex for stateful connections */
	fibril_mutex_t remote_state_mtx;

	/** Data for stateful connections */
	void *remote_state_data;
};

/** Exchange data */
struct async_exch {
	/** Link into list of inactive exchanges */
	link_t sess_link;

	/** Link into global list of inactive exchanges */
	link_t global_link;

	/** Session pointer */
	async_sess_t *sess;

	/** Exchange identification */
	cap_phone_handle_t phone;
};

extern void __async_server_init(void);
extern void __async_server_fini(void);
extern void __async_client_init(void);
extern void __async_client_fini(void);
extern void __async_ports_init(void);
extern void __async_ports_fini(void);

extern errno_t async_create_port_internal(iface_t, async_port_handler_t,
    void *, port_id_t *);
extern async_port_handler_t async_get_port_handler(iface_t, port_id_t, void **);

extern void async_reply_received(ipc_call_t *);

#endif

/** @}
 */
