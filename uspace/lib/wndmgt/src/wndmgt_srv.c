/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libwndmgt
 * @{
 */
/**
 * @file
 * @brief Window management protocol server stub
 */

#include <wndmgt_srv.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/wndmgt.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <wndmgt.h>
#include "../private/wndmgt.h"

static void wndmgt_callback_create_srv(wndmgt_srv_t *srv, ipc_call_t *call)
{
	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	srv->client_sess = sess;
	async_answer_0(call, EOK);
}

static void wndmgt_get_window_list_srv(wndmgt_srv_t *srv, ipc_call_t *icall)
{
	ipc_call_t call;
	wndmgt_window_list_t *list = NULL;
	size_t size;
	errno_t rc;

	if (srv->ops->get_window_list == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_window_list(srv->arg, &list);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Send list size */

	if (!async_data_read_receive(&call, &size)) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(list->nwindows)) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &list->nwindows, size);
	if (rc != EOK) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Send window list */

	if (!async_data_read_receive(&call, &size)) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != list->nwindows * sizeof(sysarg_t)) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, list->windows, size);
	if (rc != EOK) {
		wndmgt_free_window_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
	wndmgt_free_window_list(list);
}

static void wndmgt_get_window_info_srv(wndmgt_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	wndmgt_window_info_t *info = NULL;
	size_t capsize;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->get_window_info == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_window_info(srv->arg, wnd_id, &info);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Send caption size */

	if (!async_data_read_receive(&call, &size)) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(size_t)) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	capsize = str_size(info->caption);

	rc = async_data_read_finalize(&call, &capsize, size);
	if (rc != EOK) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Send caption */

	if (!async_data_read_receive(&call, &size)) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != capsize) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, info->caption, size);
	if (rc != EOK) {
		wndmgt_free_window_info(info);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_2(icall, EOK, info->flags, info->nfocus);
	wndmgt_free_window_info(info);
}

static void wndmgt_activate_window_srv(wndmgt_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t dev_id;
	sysarg_t wnd_id;
	errno_t rc;

	dev_id = ipc_get_arg1(icall);
	wnd_id = ipc_get_arg2(icall);

	if (srv->ops->activate_window == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->activate_window(srv->arg, dev_id, wnd_id);
	async_answer_0(icall, rc);
}

static void wndmgt_close_window_srv(wndmgt_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->close_window == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->close_window(srv->arg, wnd_id);
	async_answer_0(icall, rc);
}

static void wndmgt_get_event_srv(wndmgt_srv_t *srv, ipc_call_t *icall)
{
	wndmgt_ev_t event;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	if (srv->ops->get_event == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_event(srv->arg, &event);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Transfer event data */
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(event)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = async_data_read_finalize(&call, &event, sizeof(event));
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

void wndmgt_conn(ipc_call_t *icall, wndmgt_srv_t *srv)
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
		case WNDMGT_CALLBACK_CREATE:
			wndmgt_callback_create_srv(srv, &call);
			break;
		case WNDMGT_GET_WINDOW_LIST:
			wndmgt_get_window_list_srv(srv, &call);
			break;
		case WNDMGT_GET_WINDOW_INFO:
			wndmgt_get_window_info_srv(srv, &call);
			break;
		case WNDMGT_ACTIVATE_WINDOW:
			wndmgt_activate_window_srv(srv, &call);
			break;
		case WNDMGT_CLOSE_WINDOW:
			wndmgt_close_window_srv(srv, &call);
			break;
		case WNDMGT_GET_EVENT:
			wndmgt_get_event_srv(srv, &call);
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

/** Initialize window management server structure
 *
 * @param srv Window management server structure to initialize
 */
void wndmgt_srv_initialize(wndmgt_srv_t *srv)
{
	memset(srv, 0, sizeof(*srv));
}

/** Send 'pending' event to client.
 *
 * @param srv Window management server structure
 */
void wndmgt_srv_ev_pending(wndmgt_srv_t *srv)
{
	async_exch_t *exch;

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, WNDMGT_EV_PENDING);
	async_exchange_end(exch);
}

/** @}
 */
