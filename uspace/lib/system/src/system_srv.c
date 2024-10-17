/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libsystem
 * @{
 */
/**
 * @file
 * @brief System control protocol server stub
 */

#include <errno.h>
#include <io/log.h>
#include <ipc/system.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>
#include <system_srv.h>

static void system_callback_create_srv(system_srv_t *srv, ipc_call_t *call)
{
	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	srv->client_sess = sess;
	async_answer_0(call, EOK);
}

static void system_shutdown_srv(system_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;

	if (srv->ops->shutdown == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->shutdown(srv->arg);
	async_answer_0(icall, rc);
}

void system_conn(ipc_call_t *icall, system_srv_t *srv)
{
	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;

		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case SYSTEM_CALLBACK_CREATE:
			system_callback_create_srv(srv, &call);
			break;
		case SYSTEM_SHUTDOWN:
			system_shutdown_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}

	/* Hang up callback session */
	if (srv->client_sess != NULL) {
		async_hangup(srv->client_sess);
		srv->client_sess = NULL;
	}
}

/** Initialize system server structure
 *
 * @param srv System server structure to initialize
 */
void system_srv_initialize(system_srv_t *srv)
{
	memset(srv, 0, sizeof(*srv));
}

/** Send 'shutdown complete' event to client.
 *
 * @param srv System server structure
 */
void system_srv_shutdown_complete(system_srv_t *srv)
{
	async_exch_t *exch;

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, SYSTEM_SHUTDOWN_COMPLETE);
	async_exchange_end(exch);
}

/** Send 'shutdown failed' event to client.
 *
 * @param srv System server structure
 */
void system_srv_shutdown_failed(system_srv_t *srv)
{
	async_exch_t *exch;

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, SYSTEM_SHUTDOWN_FAILED);
	async_exchange_end(exch);
}

/** @}
 */
