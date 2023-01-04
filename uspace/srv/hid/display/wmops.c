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
 * @file Window management ops implementation
 */

#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include <str.h>
#include <wndmgt_srv.h>
#include "display.h"
#include "seat.h"
#include "window.h"
#include "wmclient.h"

static errno_t dispwm_get_window_list(void *, wndmgt_window_list_t **);
static errno_t dispwm_get_window_info(void *, sysarg_t, wndmgt_window_info_t **);
static errno_t dispwm_activate_window(void *, sysarg_t, sysarg_t);
static errno_t dispwm_close_window(void *, sysarg_t);
static errno_t dispwm_get_event(void *, wndmgt_ev_t *);

wndmgt_ops_t wndmgt_srv_ops = {
	.get_window_list = dispwm_get_window_list,
	.get_window_info = dispwm_get_window_info,
	.activate_window = dispwm_activate_window,
	.close_window = dispwm_close_window,
	.get_event = dispwm_get_event,
};

/** Get window list.
 *
 * @param arg Argument (WM client)
 * @param rlist Place to store pointer to new list
 * @return EOK on success or an error code
 */
static errno_t dispwm_get_window_list(void *arg, wndmgt_window_list_t **rlist)
{
	ds_wmclient_t *wmclient = (ds_wmclient_t *)arg;
	wndmgt_window_list_t *list;
	ds_window_t *wnd;
	unsigned i;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dispwm_get_window_list()");

	list = calloc(1, sizeof(wndmgt_window_list_t));
	if (list == NULL)
		return ENOMEM;

	ds_display_lock(wmclient->display);

	/* Count the number of windows */
	list->nwindows = 0;
	wnd = ds_display_first_window(wmclient->display);
	while (wnd != NULL) {
		++list->nwindows;
		wnd = ds_display_next_window(wnd);
	}

	/* Allocate array for window IDs */
	list->windows = calloc(list->nwindows, sizeof(sysarg_t));
	if (list->windows == NULL) {
		ds_display_unlock(wmclient->display);
		free(list);
		return ENOMEM;
	}

	/* Fill in window IDs */
	i = 0;
	wnd = ds_display_first_window(wmclient->display);
	while (wnd != NULL) {
		list->windows[i++] = wnd->id;
		wnd = ds_display_next_window(wnd);
	}

	ds_display_unlock(wmclient->display);
	*rlist = list;
	return EOK;
}

/** Get window information.
 *
 * @param arg Argument (WM client)
 * @param wnd_id Window ID
 * @param rinfo Place to store pointer to new window information structure
 * @return EOK on success or an error code
 */
static errno_t dispwm_get_window_info(void *arg, sysarg_t wnd_id,
    wndmgt_window_info_t **rinfo)
{
	ds_wmclient_t *wmclient = (ds_wmclient_t *)arg;
	ds_window_t *wnd;
	wndmgt_window_info_t *info;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dispwm_get_window_info()");

	ds_display_lock(wmclient->display);
	wnd = ds_display_find_window(wmclient->display, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(wmclient->display);
		return ENOENT;
	}

	info = calloc(1, sizeof(wndmgt_window_info_t));
	if (info == NULL) {
		ds_display_unlock(wmclient->display);
		return ENOMEM;
	}

	info->caption = str_dup(wnd->caption);
	if (info->caption == NULL) {
		ds_display_unlock(wmclient->display);
		free(info);
		return ENOMEM;
	}

	info->flags = wnd->flags;
	info->nfocus = wnd->nfocus;

	ds_display_unlock(wmclient->display);
	*rinfo = info;
	return EOK;
}

/** Activate window.
 *
 * @param arg Argument (WM client)
 * @param dev_id Input device ID
 * @param wnd_id Window ID
 * @return EOK on success or an error code
 */
static errno_t dispwm_activate_window(void *arg, sysarg_t dev_id,
    sysarg_t wnd_id)
{
	ds_wmclient_t *wmclient = (ds_wmclient_t *)arg;
	ds_window_t *wnd;
	ds_seat_t *seat;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dispwm_activate_window()");

	ds_display_lock(wmclient->display);
	wnd = ds_display_find_window(wmclient->display, wnd_id);
	if (wnd == NULL) {
		ds_display_unlock(wmclient->display);
		return ENOENT;
	}

	/* Determine which seat's focus should be changed */
	seat = ds_display_seat_by_idev(wnd->display, dev_id);
	if (seat == NULL) {
		ds_display_unlock(wmclient->display);
		return ENOENT;
	}

	/* Switch focus */
	ds_seat_set_focus(seat, wnd);

	ds_display_unlock(wmclient->display);
	return EOK;
}

/** Close window.
 *
 * @param arg Argument (WM client)
 * @param wnd_id Window ID
 * @return EOK on success or an error code
 */
static errno_t dispwm_close_window(void *arg, sysarg_t wnd_id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dispwm_close_window()");
	(void)arg;
	(void)wnd_id;
	return EOK;
}

/** Get window management event.
 *
 * @param arg Argument (WM client)
 * @param ev Place to store event
 * @return EOK on success, ENOENT if there are no events
 */
static errno_t dispwm_get_event(void *arg, wndmgt_ev_t *ev)
{
	ds_wmclient_t *wmclient = (ds_wmclient_t *)arg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dispwm_get_event()");

	ds_display_lock(wmclient->display);
	rc = ds_wmclient_get_event(wmclient, ev);
	ds_display_unlock(wmclient->display);
	return rc;
}

/** @}
 */
