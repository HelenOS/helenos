/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <display/info.h>
#include <display/wndresize.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/display.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>
#include "../private/params.h"

static void display_callback_create_srv(display_srv_t *srv, ipc_call_t *call)
{
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

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_destroy == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_destroy(srv->arg, wnd_id);
	async_answer_0(icall, rc);
}

static void display_window_move_req_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	gfx_coord2_t pos;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_coord2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &pos, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->window_move_req == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_move_req(srv->arg, wnd_id, &pos);
	async_answer_0(icall, rc);
}

static void display_window_move_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	gfx_coord2_t dpos;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_coord2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &dpos, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->window_move == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_move(srv->arg, wnd_id, &dpos);
	async_answer_0(icall, rc);
}

static void display_window_get_pos_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	gfx_coord2_t dpos;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_get_pos == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = srv->ops->window_get_pos(srv->arg, wnd_id, &dpos);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (size != sizeof(gfx_coord2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &dpos, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

static void display_window_get_max_rect_srv(display_srv_t *srv,
    ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	gfx_rect_t rect;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_get_max_rect == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = srv->ops->window_get_max_rect(srv->arg, wnd_id, &rect);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (size != sizeof(gfx_rect_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &rect, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

static void display_window_resize_req_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	display_wnd_rsztype_t rsztype;
	gfx_coord2_t pos;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);
	rsztype = (display_wnd_rsztype_t) ipc_get_arg2(icall);

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_coord2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &pos, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->window_resize_req == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_resize_req(srv->arg, wnd_id, rsztype, &pos);
	async_answer_0(icall, rc);
}

static void display_window_resize_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	ipc_call_t call;
	display_wnd_resize_t wresize;
	size_t size;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(display_wnd_resize_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &wresize, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->window_resize == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_resize(srv->arg, wnd_id, &wresize.offs,
	    &wresize.nrect);
	async_answer_0(icall, rc);
}

static void display_window_maximize_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_maximize == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_maximize(srv->arg, wnd_id);
	async_answer_0(icall, rc);
}

static void display_window_unmaximize_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);

	if (srv->ops->window_unmaximize == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_unmaximize(srv->arg, wnd_id);
	async_answer_0(icall, rc);
}

static void display_window_set_cursor_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	display_stock_cursor_t cursor;
	errno_t rc;

	wnd_id = ipc_get_arg1(icall);
	cursor = ipc_get_arg2(icall);

	if (srv->ops->window_set_cursor == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->window_set_cursor(srv->arg, wnd_id, cursor);
	async_answer_0(icall, rc);
}

static void display_get_event_srv(display_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t wnd_id;
	display_wnd_ev_t event;
	ipc_call_t call;
	size_t size;
	errno_t rc;

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

static void display_get_info_srv(display_srv_t *srv, ipc_call_t *icall)
{
	display_info_t info;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	if (srv->ops->get_info == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	/* Transfer information */
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(info)) {
		async_answer_0(icall, EREFUSED);
		async_answer_0(&call, EREFUSED);
		return;
	}

	rc = srv->ops->get_info(srv->arg, &info);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		async_answer_0(&call, rc);
		return;
	}

	rc = async_data_read_finalize(&call, &info, sizeof(info));
	if (rc != EOK) {
		async_answer_0(icall, rc);
		async_answer_0(&call, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

void display_conn(ipc_call_t *icall, display_srv_t *srv)
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
		case DISPLAY_CALLBACK_CREATE:
			display_callback_create_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_CREATE:
			display_window_create_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_DESTROY:
			display_window_destroy_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_MOVE_REQ:
			display_window_move_req_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_MOVE:
			display_window_move_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_GET_POS:
			display_window_get_pos_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_GET_MAX_RECT:
			display_window_get_max_rect_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_RESIZE_REQ:
			display_window_resize_req_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_RESIZE:
			display_window_resize_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_MAXIMIZE:
			display_window_maximize_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_UNMAXIMIZE:
			display_window_unmaximize_srv(srv, &call);
			break;
		case DISPLAY_WINDOW_SET_CURSOR:
			display_window_set_cursor_srv(srv, &call);
			break;
		case DISPLAY_GET_EVENT:
			display_get_event_srv(srv, &call);
			break;
		case DISPLAY_GET_INFO:
			display_get_info_srv(srv, &call);
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

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, DISPLAY_EV_PENDING);
	async_exchange_end(exch);
}

/** @}
 */
