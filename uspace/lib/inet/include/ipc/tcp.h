/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_IPC_TCP_H
#define LIBINET_IPC_TCP_H

#include <ipc/common.h>

typedef enum {
	TCP_CALLBACK_CREATE = IPC_FIRST_USER_METHOD,
	TCP_CONN_CREATE,
	TCP_CONN_DESTROY,
	TCP_LISTENER_CREATE,
	TCP_LISTENER_DESTROY,
	TCP_CONN_SEND,
	TCP_CONN_SEND_FIN,
	TCP_CONN_PUSH,
	TCP_CONN_RESET,
	TCP_CONN_RECV,
	TCP_CONN_RECV_WAIT
} tcp_request_t;

typedef enum {
	TCP_EV_CONNECTED = IPC_FIRST_USER_METHOD,
	TCP_EV_CONN_FAILED,
	TCP_EV_CONN_RESET,
	TCP_EV_DATA,
	TCP_EV_URG_DATA,
	TCP_EV_NEW_CONN
} tcp_event_t;

#endif

/** @}
 */
