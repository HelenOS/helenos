/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libdisplay
 * @{
 */
/**
 * @file
 * @brief Display protocol server stub
 */

#include <disp_srv.h>
#include <display/event.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/display.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>

#include <stdio.h>
static void display_callback_create_srv(display_srv_t *srv, ipc_call_t *call)
{
	printf("display_callback_create_srv\n");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	srv->client_sess = sess;
	async_answer_0(call, EOK);
}

static void display_window_create_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	display_wnd_params_t params;
	size_t size;
	errno_t rc;

	printf("display_window_create_srv\n");

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(display_wnd_params_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &params, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->window_create == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_create(srv->arg, &params, &wnd_id);
	async_answer_1(icall, rc, wnd_id);
}

static void display_window_destroy_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	errno_t rc;

	printf("display_window_destroy_srv\n");

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_create == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_destroy(srv->arg, wnd_id);
	async_answer_0(icall, rc);
}

static void display_get_event_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	display_wnd_ev_t event;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	printf("display_get_event_srv\n");

	if (srv->ops->get_event == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_event(srv->arg, &wnd_id, &event);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Transfer event data */
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(event)) {
		async_answer_0(icall, EREFUSED);
		async_answer_0(&call, EREFUSED);
		return;
	}

	rc = async_data_read_finalize(&call, &event, sizeof(event));
	if (rc != EOK) {
		async_answer_0(icall, rc);
		async_answer_0(&call, rc);
		return;
	}

	async_answer_1(icall, EOK, wnd_id);
}

void display_conn(ipc_call_t *icall, display_srv_t *srv)
{
	/* Accept the connection */
	async_accept_0(icall);
	printf("display_conn\n");

	while (true) {
		ipc_call_t call;

		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		printf("display_conn method=%u\n", (unsigned) method);
		switch (method) {
		case DISPLAY_CALLBACK_CREATE:
			display_callback_create_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_CREATE:
			display_window_create_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_DESTROY:
			display_window_destroy_srv(srv, &call);
			break;
		case DISPLAY_GET_EVENT:
			display_get_event_srv(srv, &call);
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

/** Initialize display server structure
 *
 * @param srv Display server structure to initialize
 */
void display_srv_initialize(display_srv_t *srv)
{
	memset(srv, 0, sizeof(*srv));
}

/** Send 'pending' event to client.
 *
 * @param srv Display server structure
 */
void display_srv_ev_pending(display_srv_t *srv)
{
	async_exch_t *exch;

	printf("display_srv_ev_pending()\n");

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, DISPLAY_EV_PENDING);
	async_exchange_end(exch);
}

/** @}
 */
