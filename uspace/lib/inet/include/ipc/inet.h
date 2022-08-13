/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_IPC_INET_H
#define LIBINET_IPC_INET_H

#include <ipc/common.h>

/** Requests on Inet default port */
typedef enum {
	INET_CALLBACK_CREATE = IPC_FIRST_USER_METHOD,
	INET_GET_SRCADDR,
	INET_SEND,
	INET_SET_PROTO
} inet_request_t;

/** Events on Inet default port */
typedef enum {
	INET_EV_RECV = IPC_FIRST_USER_METHOD
} inet_event_t;

/** Requests on Inet configuration port */
typedef enum {
	INETCFG_ADDR_CREATE_STATIC = IPC_FIRST_USER_METHOD,
	INETCFG_ADDR_DELETE,
	INETCFG_ADDR_GET,
	INETCFG_ADDR_GET_ID,
	INETCFG_GET_ADDR_LIST,
	INETCFG_GET_LINK_LIST,
	INETCFG_GET_SROUTE_LIST,
	INETCFG_LINK_ADD,
	INETCFG_LINK_GET,
	INETCFG_LINK_REMOVE,
	INETCFG_SROUTE_CREATE,
	INETCFG_SROUTE_DELETE,
	INETCFG_SROUTE_GET,
	INETCFG_SROUTE_GET_ID
} inetcfg_request_t;

/** Events on Inet ping port */
typedef enum {
	INETPING_EV_RECV = IPC_FIRST_USER_METHOD
} inetping_event_t;

/** Requests on Inet ping port */
typedef enum {
	INETPING_SEND = IPC_FIRST_USER_METHOD,
	INETPING_GET_SRCADDR
} inetping_request_t;

#endif

/**
 * @}
 */
