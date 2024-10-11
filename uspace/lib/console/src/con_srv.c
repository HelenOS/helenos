/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libconsole
 * @{
 */
/**
 * @file
 * @brief Console protocol server stub
 */
#include <as.h>
#include <errno.h>
#include <io/cons_event.h>
#include <ipc/console.h>
#include <stdlib.h>
#include <stddef.h>

#include <io/con_srv.h>

static errno_t console_ev_encode(cons_event_t *event, ipc_call_t *icall)
{
	ipc_set_arg1(icall, event->type);

	switch (event->type) {
	case CEV_KEY:
		ipc_set_arg2(icall, event->ev.key.type);
		ipc_set_arg3(icall, event->ev.key.key);
		ipc_set_arg4(icall, event->ev.key.mods);
		ipc_set_arg5(icall, event->ev.key.c);
		return EOK;
	case CEV_POS:
		ipc_set_arg2(icall, (event->ev.pos.pos_id << 16) | (event->ev.pos.type & 0xffff));
		ipc_set_arg3(icall, event->ev.pos.btn_num);
		ipc_set_arg4(icall, event->ev.pos.hpos);
		ipc_set_arg5(icall, event->ev.pos.vpos);
		return EOK;
	case CEV_RESIZE:
		ipc_set_arg2(icall, 0);
		ipc_set_arg3(icall, 0);
		ipc_set_arg4(icall, 0);
		ipc_set_arg5(icall, 0);
		return EOK;
	}

	return EIO;
}

static void con_read_srv(con_srv_t *srv, ipc_call_t *icall)
{
	void *buf;
	size_t size;
	errno_t rc;

	ipc_call_t call;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	if (srv->srvs->ops->read == NULL) {
		async_answer_0(&call, ENOTSUP);
		async_answer_0(icall, ENOTSUP);
		free(buf);
		return;
	}

	size_t nread;
	rc = srv->srvs->ops->read(srv, buf, size, &nread);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(buf);
		return;
	}

	async_data_read_finalize(&call, buf, nread);
	free(buf);

	async_answer_1(icall, EOK, nread);
}

static void con_write_srv(con_srv_t *srv, ipc_call_t *icall)
{
	void *data;
	size_t size;
	errno_t rc;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	if (srv->srvs->ops->write == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	size_t nwritten = 0;
	rc = srv->srvs->ops->write(srv, data, size, &nwritten);
	free(data);

	async_answer_1(icall, rc, nwritten);
}

static void con_sync_srv(con_srv_t *srv, ipc_call_t *icall)
{
	if (srv->srvs->ops->sync == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->sync(srv);
	async_answer_0(icall, EOK);
}

static void con_clear_srv(con_srv_t *srv, ipc_call_t *icall)
{
	if (srv->srvs->ops->clear == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->clear(srv);
	async_answer_0(icall, EOK);
}

static void con_set_pos_srv(con_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t col;
	sysarg_t row;

	col = ipc_get_arg1(icall);
	row = ipc_get_arg2(icall);

	if (srv->srvs->ops->set_pos == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_pos(srv, col, row);
	async_answer_0(icall, EOK);
}

static void con_get_pos_srv(con_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	sysarg_t col;
	sysarg_t row;

	if (srv->srvs->ops->get_pos == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_pos(srv, &col, &row);
	async_answer_2(icall, rc, col, row);
}

static void con_get_size_srv(con_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	sysarg_t cols;
	sysarg_t rows;

	if (srv->srvs->ops->get_size == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_size(srv, &cols, &rows);
	async_answer_2(icall, rc, cols, rows);
}

static void con_get_color_cap_srv(con_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	console_caps_t ccap;

	if (srv->srvs->ops->get_color_cap == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_color_cap(srv, &ccap);
	async_answer_1(icall, rc, (sysarg_t)ccap);
}

static void con_set_style_srv(con_srv_t *srv, ipc_call_t *icall)
{
	console_style_t style;

	style = ipc_get_arg1(icall);

	if (srv->srvs->ops->set_style == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_style(srv, style);
	async_answer_0(icall, EOK);
}

static void con_set_color_srv(con_srv_t *srv, ipc_call_t *icall)
{
	console_color_t bgcolor;
	console_color_t fgcolor;
	console_color_attr_t flags;

	bgcolor = ipc_get_arg1(icall);
	fgcolor = ipc_get_arg2(icall);
	flags = ipc_get_arg3(icall);

	if (srv->srvs->ops->set_color == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_color(srv, bgcolor, fgcolor, flags);
	async_answer_0(icall, EOK);
}

static void con_set_rgb_color_srv(con_srv_t *srv, ipc_call_t *icall)
{
	pixel_t bgcolor;
	pixel_t fgcolor;

	bgcolor = ipc_get_arg1(icall);
	fgcolor = ipc_get_arg2(icall);

	if (srv->srvs->ops->set_rgb_color == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_rgb_color(srv, bgcolor, fgcolor);
	async_answer_0(icall, EOK);
}

static void con_set_cursor_visibility_srv(con_srv_t *srv, ipc_call_t *icall)
{
	bool show;

	show = ipc_get_arg1(icall);

	if (srv->srvs->ops->set_cursor_visibility == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_cursor_visibility(srv, show);
	async_answer_0(icall, EOK);
}

static void con_set_caption_srv(con_srv_t *srv, ipc_call_t *icall)
{
	char *caption;
	errno_t rc;

	rc = async_data_write_accept((void **) &caption, true, 0,
	    CON_CAPTION_MAXLEN, 0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	if (srv->srvs->ops->set_caption == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->set_caption(srv, caption);
	free(caption);
	async_answer_0(icall, EOK);
}

static void con_get_event_srv(con_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	cons_event_t event;
	ipc_call_t result;

	if (srv->srvs->ops->get_event == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_event(srv, &event);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = console_ev_encode(&event, &result);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_5(icall, rc, ipc_get_arg1(&result), ipc_get_arg2(&result),
	    ipc_get_arg3(&result), ipc_get_arg4(&result), ipc_get_arg5(&result));
}

/** Create shared buffer for efficient rendering */
static void con_map_srv(con_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	charfield_t *buf;
	sysarg_t cols, rows;
	ipc_call_t call;
	size_t size;

	if (srv->srvs->ops->map == NULL || srv->srvs->ops->unmap == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	cols = ipc_get_arg1(icall);
	rows = ipc_get_arg2(icall);

	if (!async_share_in_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		return;
	}

	/* Check size */
	if (size != PAGES2SIZE(SIZE2PAGES(cols * rows * sizeof(charfield_t)))) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = srv->srvs->ops->map(srv, cols, rows, &buf);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = async_share_in_finalize(&call, buf, AS_AREA_READ |
	    AS_AREA_WRITE | AS_AREA_CACHEABLE);
	if (rc != EOK) {
		srv->srvs->ops->unmap(srv);
		async_answer_0(icall, EIO);
		return;
	}

	async_answer_0(icall, EOK);
}

/** Delete shared buffer */
static void con_unmap_srv(con_srv_t *srv, ipc_call_t *icall)
{
	if (srv->srvs->ops->unmap == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->unmap(srv);
	async_answer_0(icall, EOK);
}

/** Update console area from shared buffer */
static void con_update_srv(con_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t c0, r0, c1, r1;

	c0 = ipc_get_arg1(icall);
	r0 = ipc_get_arg2(icall);
	c1 = ipc_get_arg3(icall);
	r1 = ipc_get_arg4(icall);

	if (srv->srvs->ops->update == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	srv->srvs->ops->update(srv, c0, r0, c1, r1);
	async_answer_0(icall, EOK);
}

static con_srv_t *con_srv_create(con_srvs_t *srvs)
{
	con_srv_t *srv;

	srv = calloc(1, sizeof(*srv));
	if (srv == NULL)
		return NULL;

	srv->srvs = srvs;
	return srv;
}

void con_srvs_init(con_srvs_t *srvs)
{
	srvs->ops = NULL;
	srvs->sarg = NULL;
	srvs->abort_timeout = 0;
	srvs->aborted = false;
}

errno_t con_conn(ipc_call_t *icall, con_srvs_t *srvs)
{
	con_srv_t *srv;
	errno_t rc;

	/* Accept the connection */
	async_accept_0(icall);

	srv = con_srv_create(srvs);
	if (srv == NULL)
		return ENOMEM;

	srv->client_sess = NULL;

	rc = srvs->ops->open(srvs, srv);
	if (rc != EOK)
		return rc;

	while (true) {
		ipc_call_t call;
		bool received = false;

		while (!received) {
			/* XXX Need to be able to abort immediately */
			received = async_get_call_timeout(&call,
			    srvs->abort_timeout);

			if (srv->srvs->aborted) {
				if (received)
					async_answer_0(&call, EINTR);
				break;
			}
		}

		if (!received || srv->srvs->aborted)
			break;

		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case VFS_OUT_READ:
			con_read_srv(srv, &call);
			break;
		case VFS_OUT_WRITE:
			con_write_srv(srv, &call);
			break;
		case VFS_OUT_SYNC:
			con_sync_srv(srv, &call);
			break;
		case CONSOLE_CLEAR:
			con_clear_srv(srv, &call);
			break;
		case CONSOLE_SET_POS:
			con_set_pos_srv(srv, &call);
			break;
		case CONSOLE_GET_POS:
			con_get_pos_srv(srv, &call);
			break;
		case CONSOLE_GET_SIZE:
			con_get_size_srv(srv, &call);
			break;
		case CONSOLE_GET_COLOR_CAP:
			con_get_color_cap_srv(srv, &call);
			break;
		case CONSOLE_SET_STYLE:
			con_set_style_srv(srv, &call);
			break;
		case CONSOLE_SET_COLOR:
			con_set_color_srv(srv, &call);
			break;
		case CONSOLE_SET_RGB_COLOR:
			con_set_rgb_color_srv(srv, &call);
			break;
		case CONSOLE_SET_CURSOR_VISIBILITY:
			con_set_cursor_visibility_srv(srv, &call);
			break;
		case CONSOLE_SET_CAPTION:
			con_set_caption_srv(srv, &call);
			break;
		case CONSOLE_GET_EVENT:
			con_get_event_srv(srv, &call);
			break;
		case CONSOLE_MAP:
			con_map_srv(srv, &call);
			break;
		case CONSOLE_UNMAP:
			con_unmap_srv(srv, &call);
			break;
		case CONSOLE_UPDATE:
			con_update_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}

	rc = srvs->ops->close(srv);
	free(srv);

	return rc;
}

/** @}
 */
