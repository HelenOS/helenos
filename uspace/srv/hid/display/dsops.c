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

/** @addtogroup display
 * @{
 */
/**
 * @file Display ops implementation
 */

#include <disp_srv.h>
#include <errno.h>
#include <io/log.h>
#include "client.h"
#include "display.h"
#include "dsops.h"
#include "seat.h"
#include "window.h"

static errno_t disp_window_create(void *, display_wnd_params_t *, sysarg_t *);
static errno_t disp_window_destroy(void *, sysarg_t);
static errno_t disp_get_event(void *, sysarg_t *, display_wnd_ev_t *);

display_ops_t display_srv_ops = {
	.window_create = disp_window_create,
	.window_destroy = disp_window_destroy,
	.get_event = disp_get_event
};

static errno_t disp_window_create(void *arg, display_wnd_params_t *params,
    sysarg_t *rwnd_id)
{
	errno_t rc;
	ds_client_t *client = (ds_client_t *) arg;
	ds_seat_t *seat;
	ds_window_t *wnd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create()");

	rc = ds_window_create(client, params, &wnd);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() - ds_window_create -> %d", rc);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() -> EOK, id=%zu",
	    wnd->id);

	wnd->dpos.x = ((wnd->id - 1) & 1) * 400;
	wnd->dpos.y = ((wnd->id - 1) & 2) / 2 * 300;

	/*
	 * XXX This should be performed by window manager. It needs to determine
	 * whether the new window should get focus and which seat should
	 * focus on it.
	 */
	seat = ds_display_first_seat(client->display);
	ds_seat_set_focus(seat, wnd);

	*rwnd_id = wnd->id;
	return EOK;
}

static errno_t disp_window_destroy(void *arg, sysarg_t wnd_id)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	wnd = ds_client_find_window(client, wnd_id);
	if (wnd == NULL)
		return ENOENT;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_destroy()");
	ds_client_remove_window(wnd);
	ds_window_destroy(wnd);
	return EOK;
}

static errno_t disp_get_event(void *arg, sysarg_t *wnd_id,
    display_wnd_ev_t *event)
{
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_get_event()");

	rc = ds_client_get_event(client, &wnd, event);
	if (rc != EOK)
		return rc;

	*wnd_id = wnd->id;
	return EOK;
}

/** @}
 */
