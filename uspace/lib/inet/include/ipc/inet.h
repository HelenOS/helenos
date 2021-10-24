/*
 * Copyright (c) 2012 Jiri Svoboda
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
