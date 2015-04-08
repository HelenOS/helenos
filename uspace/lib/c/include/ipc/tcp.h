/*
 * Copyright (c) 2015 Jiri Svobda
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

#ifndef LIBC_IPC_TCP_H_
#define LIBC_IPC_TCP_H_

#include <ipc/common.h>

typedef enum {
	TCP_CONN_CREATE = IPC_FIRST_USER_METHOD,
	TCP_CONN_DESTROY,
	TCP_LISTENER_CREATE,
	TCP_LISTENER_DESTROY,
	TCP_CONN_SEND,
	TCP_CONN_SEND_FIN,
	TCP_CONN_PUSH,
	TCP_CONN_RESET,
	TCP_CONN_RECV
} tcp_request_t;

typedef enum {
	TCP_CONNECTED = IPC_FIRST_USER_METHOD
	TCP_CONN_FAILED,
	TCP_CONN_RESET,
	TCP_DATA_AVAIL,
	TCP_URG_DATA
} tcp_notif_t;

typedef enum {
	TCP_NEW_CONN = IPC_FIRST_USER_METHOD
} tcp_listen_cb_t;

#endif

/** @}
 */
