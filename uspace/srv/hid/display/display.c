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
 * @file Display management
 */

#include <disp_srv.h>
#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include "display.h"
#include "window.h"

static errno_t disp_window_create(void *, sysarg_t *);
static errno_t disp_window_destroy(void *, sysarg_t);

display_ops_t display_srv_ops = {
	.window_create = disp_window_create,
	.window_destroy = disp_window_destroy
};

static errno_t disp_window_create(void *arg, sysarg_t *rwnd_id)
{
	errno_t rc;
	ds_display_t *disp = (ds_display_t *) arg;
	ds_window_t *wnd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create()");

	rc = ds_window_create(disp, &wnd);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() - ds_window_create -> %d", rc);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() -> EOK, id=%zu",
	    wnd->id);
	*rwnd_id = wnd->id;
	return EOK;
}

static errno_t disp_window_destroy(void *arg, sysarg_t wnd_id)
{
	ds_display_t *disp = (ds_display_t *) arg;
	ds_window_t *wnd;

	wnd = ds_display_find_window(disp, wnd_id);
	if (wnd == NULL)
		return ENOENT;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_destroy()");
	ds_display_remove_window(wnd);
	ds_window_delete(wnd);
	return EOK;
}

/** Create display.
 *
 * @param rdisp Place to store pointer to new display.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_display_create(ds_display_t **rdisp)
{
	ds_display_t *disp;

	disp = calloc(1, sizeof(ds_display_t));
	if (disp == NULL)
		return ENOMEM;

	list_initialize(&disp->windows);
	disp->next_wnd_id = 1;
	*rdisp = disp;
	return EOK;
}

/** Destroy display.
 *
 * @param disp Display
 */
void ds_display_destroy(ds_display_t *disp)
{
	assert(list_empty(&disp->windows));
	free(disp);
}

/** Add window to display.
 *
 * @param disp Display
 * @param wnd Window
 * @return EOK on success, ENOMEM if there are no free window identifiers
 */
errno_t ds_display_add_window(ds_display_t *disp, ds_window_t *wnd)
{
	assert(wnd->display == NULL);
	assert(!link_used(&wnd->lwindows));

	wnd->display = disp;
	wnd->id = disp->next_wnd_id++;
	list_append(&wnd->lwindows, &disp->windows);

	return EOK;
}

/** Remove window from display.
 *
 * @param wnd Window
 */
void ds_display_remove_window(ds_window_t *wnd)
{
	list_remove(&wnd->lwindows);
	wnd->display = NULL;
}

/** Find window by ID.
 *
 * @param disp Display
 * @param id Window ID
 */
ds_window_t *ds_display_find_window(ds_display_t *disp, ds_wnd_id_t id)
{
	ds_window_t *wnd;

	// TODO Make this faster
	wnd = ds_display_first_window(disp);
	while (wnd != NULL) {
		if (wnd->id == id)
			return wnd;
		wnd = ds_display_next_window(wnd);
	}

	return NULL;
}

/** Get first window in display.
 *
 * @param disp Display
 * @return First window or @c NULL if there is none
 */
ds_window_t *ds_display_first_window(ds_display_t *disp)
{
	link_t *link = list_first(&disp->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lwindows);
}

/** Get next window in display.
 *
 * @param wnd Current window
 * @return Next window or @c NULL if there is none
 */
ds_window_t *ds_display_next_window(ds_window_t *wnd)
{
	link_t *link = list_next(&wnd->lwindows, &wnd->display->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lwindows);
}

/** @}
 */
