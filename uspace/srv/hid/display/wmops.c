/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file Window management ops implementation
 */

#include <errno.h>
//#include <io/log.h>
#include <stdlib.h>
#include <str.h>
#include <wndmgt_srv.h>
//#include "client.h"
//#include "display.h"
//#include "dsops.h"
//#include "seat.h"
//#include "window.h"

static errno_t dispwm_get_window_list(void *, wndmgt_window_list_t **);
static errno_t dispwm_get_window_info(void *, sysarg_t, wndmgt_window_info_t **);
static errno_t dispwm_activate_window(void *, sysarg_t);
static errno_t dispwm_close_window(void *, sysarg_t);
static errno_t dispwm_get_event(void *, wndmgt_ev_t *);

wndmgt_ops_t wndmgt_srv_ops = {
	.get_window_list = dispwm_get_window_list,
	.get_window_info = dispwm_get_window_info,
	.activate_window = dispwm_activate_window,
	.close_window = dispwm_close_window,
	.get_event = dispwm_get_event,
};

static errno_t dispwm_get_window_list(void *arg, wndmgt_window_list_t **rlist)
{
	wndmgt_window_list_t *list;

	list = calloc(1, sizeof(wndmgt_window_list_t));
	if (list == NULL)
		return ENOMEM;

	list->nwindows = 2;
	list->windows = calloc(2, sizeof(sysarg_t));
	if (list->windows == NULL) {
		free(list);
		return ENOMEM;
	}

	list->windows[0] = 1;
	list->windows[1] = 2;

	*rlist = list;
	return EOK;
}

static errno_t dispwm_get_window_info(void *arg, sysarg_t wnd_id,
    wndmgt_window_info_t **rinfo)
{
/*	ds_client_t *client = (ds_client_t *) arg;
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
	return EOK;*/
	wndmgt_window_info_t *info;

	info = calloc(1, sizeof(wndmgt_window_info_t));
	if (info == NULL)
		return ENOMEM;

	info->caption = str_dup("Hello");
	if (info->caption == NULL) {
		free(info);
		return ENOMEM;
	}

	*rinfo = info;
	return EOK;
}

static errno_t dispwm_activate_window(void *arg, sysarg_t wnd_id)
{
	(void)arg;
	(void)wnd_id;
	return EOK;
}

static errno_t dispwm_close_window(void *arg, sysarg_t wnd_id)
{
	(void)arg;
	(void)wnd_id;
	return EOK;
}

static errno_t dispwm_get_event(void *arg, wndmgt_ev_t *ev)
{
	return ENOTSUP;
}

/** @}
 */
