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

/** @addtogroup display
 * @{
 */
/**
 * @file Display ops implementation
 */

#include <disp_srv.h>
#include <errno.h>
#include <gfx/coord.h>
#include <io/log.h>
#include "client.h"
#include "display.h"
#include "dsops.h"
#include "seat.h"
#include "window.h"

static errno_t disp_window_create(void *, display_wnd_params_t *, sysarg_t *);
static errno_t disp_window_destroy(void *, sysarg_t);
static errno_t disp_window_move_req(void *, sysarg_t, gfx_coord2_t *,
    sysarg_t);
static errno_t disp_window_move(void *, sysarg_t, gfx_coord2_t *);
static errno_t disp_window_get_pos(void *, sysarg_t, gfx_coord2_t *);
static errno_t disp_window_get_max_rect(void *, sysarg_t, gfx_rect_t *);
static errno_t disp_window_resize_req(void *, sysarg_t,
    display_wnd_rsztype_t, gfx_coord2_t *, sysarg_t);
static errno_t disp_window_resize(void *, sysarg_t, gfx_coord2_t *,
    gfx_rect_t *);
static errno_t disp_window_minimize(void *, sysarg_t);
static errno_t disp_window_maximize(void *, sysarg_t);
static errno_t disp_window_unmaximize(void *, sysarg_t);
static errno_t disp_window_set_cursor(void *, sysarg_t, display_stock_cursor_t);
static errno_t disp_window_set_caption(void *, sysarg_t, const char *);
static errno_t disp_get_event(void *, sysarg_t *, display_wnd_ev_t *);
static errno_t disp_get_info(void *, display_info_t *);

display_ops_t display_srv_ops = {
	.window_create = disp_window_create,
	.window_destroy = disp_window_destroy,
	.window_move_req = disp_window_move_req,
	.window_move = disp_window_move,
	.window_get_pos = disp_window_get_pos,
	.window_get_max_rect = disp_window_get_max_rect,
	.window_resize_req = disp_window_resize_req,
	.window_resize = disp_window_resize,
	.window_minimize = disp_window_minimize,
	.window_maximize = disp_window_maximize,
	.window_unmaximize = disp_window_unmaximize,
	.window_set_cursor = disp_window_set_cursor,
	.window_set_caption = disp_window_set_caption,
	.get_event = disp_get_event,
	.get_info = disp_get_info
};

static errno_t disp_window_create(void *arg, display_wnd_params_t *params,
    sysarg_t *rwnd_id)
{
	errno_t rc;
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create()");

	ds_display_lock(client->display);

	rc = ds_window_create(client, params, &wnd);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() - ds_window_create -> %d", rc);
	if (rc != EOK) {
		ds_display_unlock(client->display);
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() -> EOK, id=%zu",
	    wnd->id);

	ds_display_unlock(client->display);

	*rwnd_id = wnd->id;
	return EOK;
}

static errno_t disp_window_destroy(void *arg, sysarg_t wnd_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_destroy()");
	ds_window_destroy(wnd);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_move_req(void *arg, sysarg_t wnd_id,
    gfx_coord2_t *pos, sysarg_t pos_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_move_req()");
	ds_window_move_req(wnd, pos, pos_id);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_move(void *arg, sysarg_t wnd_id, gfx_coord2_t *pos)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_move()");
	ds_window_move(wnd, pos);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_get_pos(void *arg, sysarg_t wnd_id,
    gfx_coord2_t *pos)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_get_pos()");
	ds_window_get_pos(wnd, pos);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_get_max_rect(void *arg, sysarg_t wnd_id,
    gfx_rect_t *rect)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_get_max_rect()");
	ds_window_get_max_rect(wnd, rect);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_resize_req(void *arg, sysarg_t wnd_id,
    display_wnd_rsztype_t rsztype, gfx_coord2_t *pos, sysarg_t pos_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	if (!display_wndrsz_valid(rsztype))
		return EINVAL;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_resize_req()");
	ds_window_resize_req(wnd, rsztype, pos, pos_id);
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_window_resize(void *arg, sysarg_t wnd_id,
    gfx_coord2_t *offs, gfx_rect_t *nbound)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_resize()");
	rc = ds_window_resize(wnd, offs, nbound);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_window_minimize(void *arg, sysarg_t wnd_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_maximize()");
	rc = ds_window_minimize(wnd);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_window_maximize(void *arg, sysarg_t wnd_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_maximize()");
	rc = ds_window_maximize(wnd);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_window_unmaximize(void *arg, sysarg_t wnd_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_unmaximize()");
	rc = ds_window_unmaximize(wnd);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_window_set_cursor(void *arg, sysarg_t wnd_id,
    display_stock_cursor_t cursor)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_set_cursor()");
	rc = ds_window_set_cursor(wnd, cursor);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_window_set_caption(void *arg, sysarg_t wnd_id,
    const char *caption)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	ds_display_lock(client->display);

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(client->display);
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_set_caption()");
	rc = ds_window_set_caption(wnd, caption);
	ds_display_unlock(client->display);
	return rc;
}

static errno_t disp_get_event(void *arg, sysarg_t *wnd_id,
    display_wnd_ev_t *event)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "disp_window_get_event()");

	ds_display_lock(client->display);

	rc = ds_client_get_event(client, &wnd, event);
	if (rc != EOK) {
		ds_display_unlock(client->display);
		return rc;
	}

	*wnd_id = wnd->id;
	ds_display_unlock(client->display);
	return EOK;
}

static errno_t disp_get_info(void *arg, display_info_t *info)
{
	ds_client_t *client = (ds_client_t *) arg;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_get_info()");

	ds_display_lock(client->display);
	ds_display_get_info(client->display, info);
	ds_display_unlock(client->display);
	return EOK;
}

/** @}
 */
