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
#include <gfx/context.h>
#include <io/log.h>
#include <stdlib.h>
#include "client.h"
#include "window.h"
#include "display.h"

static errno_t disp_window_create(void *, sysarg_t *);
static errno_t disp_window_destroy(void *, sysarg_t);
static errno_t disp_get_event(void *, sysarg_t *, display_wnd_ev_t *);

display_ops_t display_srv_ops = {
	.window_create = disp_window_create,
	.window_destroy = disp_window_destroy,
	.get_event = disp_get_event
};

static errno_t disp_window_create(void *arg, sysarg_t *rwnd_id)
{
	errno_t rc;
	ds_client_t *client = (ds_client_t *) arg;
	ds_window_t *wnd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create()");

	rc = ds_window_create(client, &wnd);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() - ds_window_create -> %d", rc);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "disp_window_create() -> EOK, id=%zu",
	    wnd->id);

	wnd->dpos.x = ((wnd->id - 1) & 1) * 400;
	wnd->dpos.y = ((wnd->id - 1) & 2) / 2 * 300;

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

/** Create display.
 *
 * @param gc Graphics context for displaying output
 * @param rdisp Place to store pointer to new display.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_display_create(gfx_context_t *gc, ds_display_t **rdisp)
{
	ds_display_t *disp;

	disp = calloc(1, sizeof(ds_display_t));
	if (disp == NULL)
		return ENOMEM;

	list_initialize(&disp->clients);
	disp->gc = gc;
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
	assert(list_empty(&disp->clients));
	free(disp);
}

/** Add client to display.
 *
 * @param disp Display
 * @param client client
 */
void ds_display_add_client(ds_display_t *disp, ds_client_t *client)
{
	assert(client->display == NULL);
	assert(!link_used(&client->lclients));

	client->display = disp;
	list_append(&client->lclients, &disp->clients);
}

/** Remove client from display.
 *
 * @param client client
 */
void ds_display_remove_client(ds_client_t *client)
{
	list_remove(&client->lclients);
	client->display = NULL;
}

/** Get first client in display.
 *
 * @param disp Display
 * @return First client or @c NULL if there is none
 */
ds_client_t *ds_display_first_client(ds_display_t *disp)
{
	link_t *link = list_first(&disp->clients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_client_t, lclients);
}

/** Get next client in display.
 *
 * @param client Current client
 * @return Next client or @c NULL if there is none
 */
ds_client_t *ds_display_next_client(ds_client_t *client)
{
	link_t *link = list_next(&client->lclients, &client->display->clients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_client_t, lclients);
}

/** Find window in all clients by ID.
 *
 * XXX This is just a hack needed to match GC connection to a window,
 * as we don't have a good safe way to pass the GC endpoint to our client
 * on demand.
 *
 * @param display Display
 * @param id Window ID
 */
#include <stdio.h>
ds_window_t *ds_display_find_window(ds_display_t *display, ds_wnd_id_t id)
{
	ds_client_t *client;
	ds_window_t *wnd;

	printf("ds_display_find_window: id=0x%lx\n", id);

	client = ds_display_first_client(display);
	while (client != NULL) {
		printf("ds_display_find_window: client=%p\n", client);
		wnd = ds_client_find_window(client, id);
		if (wnd != NULL) {
			printf("ds_display_find_window: found wnd=%p id=0x%lx\n",
			    wnd, wnd->id);
			return wnd;
		}
		client = ds_display_next_client(client);
	}

	printf("ds_display_find_window: not found\n");
	return NULL;
}

errno_t ds_display_post_kbd_event(ds_display_t *display, kbd_event_t *event)
{
	ds_client_t *client;
	ds_window_t *wnd;

	// XXX Correctly determine destination window

	client = ds_display_first_client(display);
	if (client == NULL)
		return EOK;

	wnd = ds_client_first_window(client);
	if (wnd == NULL)
		return EOK;

	return ds_client_post_kbd_event(client, wnd, event);
}

/** @}
 */
