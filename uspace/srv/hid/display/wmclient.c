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
 * @file Display server WM client
 */

#include <adt/list.h>
#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include "display.h"
#include "window.h"
#include "wmclient.h"

/** Create WM client.
 *
 * @param display Parent display
 * @param cb WM client callbacks
 * @param cb_arg Callback argument
 * @param rwmclient Place to store pointer to new WM client.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_wmclient_create(ds_display_t *display, ds_wmclient_cb_t *cb,
    void *cb_arg, ds_wmclient_t **rwmclient)
{
	ds_wmclient_t *wmclient;

	wmclient = calloc(1, sizeof(ds_wmclient_t));
	if (wmclient == NULL)
		return ENOMEM;

	list_initialize(&wmclient->events);
	wmclient->cb = cb;
	wmclient->cb_arg = cb_arg;

	ds_display_add_wmclient(display, wmclient);

	*rwmclient = wmclient;
	return EOK;
}

/** Destroy WM client.
 *
 * @param wmclient WM client
 */
void ds_wmclient_destroy(ds_wmclient_t *wmclient)
{
	ds_wmclient_purge_events(wmclient);
	ds_display_remove_wmclient(wmclient);
	free(wmclient);
}

/** Get next event from WM client event queue.
 *
 * @param wmclient WM client
 * @param event Place to store event
 * @return EOK on success, ENOENT if event queue is empty
 */
errno_t ds_wmclient_get_event(ds_wmclient_t *wmclient, wndmgt_ev_t *event)
{
	link_t *link;
	ds_wmclient_ev_t *wevent;

	link = list_first(&wmclient->events);
	if (link == NULL)
		return ENOENT;

	wevent = list_get_instance(link, ds_wmclient_ev_t, levents);
	list_remove(link);

	*event = wevent->event;
	free(wevent);
	return EOK;
}

/** Purge events from WM client event queue.
 *
 * @param client Client
 */
void ds_wmclient_purge_events(ds_wmclient_t *wmclient)
{
	link_t *cur;
	link_t *next;
	ds_wmclient_ev_t *wevent;

	cur = list_first(&wmclient->events);
	while (cur != NULL) {
		next = list_next(cur, &wmclient->events);
		wevent = list_get_instance(cur, ds_wmclient_ev_t, levents);

		list_remove(cur);
		free(wevent);
		cur = next;
	}
}

/** Post window added event to the WM client's message queue.
 *
 * @param wmclient WM client
 * @param wnd_id Window ID of the added window
 *
 * @return EOK on success or an error code
 */
errno_t ds_wmclient_post_wnd_added_event(ds_wmclient_t *wmclient,
    sysarg_t wnd_id)
{
	ds_wmclient_ev_t *wevent;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "wmclient_pos_wnd_added_event "
	    "wmclient=%p wnd_id=%zu\n", (void *)wmclient, wnd_id);

	wevent = calloc(1, sizeof(ds_wmclient_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->wmclient = wmclient;
	wevent->event.etype = wmev_window_added;
	wevent->event.wnd_id = wnd_id;
	list_append(&wevent->levents, &wmclient->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (wmclient->cb != NULL && wmclient->cb->ev_pending != NULL)
		wmclient->cb->ev_pending(wmclient->cb_arg);

	return EOK;
}

/** Post window removed event to the WM client's message queue.
 *
 * @param wmclient WM client
 * @param wnd_id Window ID of the added window
 *
 * @return EOK on success or an error code
 */
errno_t ds_wmclient_post_wnd_removed_event(ds_wmclient_t *wmclient,
    sysarg_t wnd_id)
{
	ds_wmclient_ev_t *wevent;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "wmclient_pos_wnd_removed_event "
	    "wmclient=%p wnd_id=%zu\n", (void *)wmclient, wnd_id);

	wevent = calloc(1, sizeof(ds_wmclient_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->wmclient = wmclient;
	wevent->event.etype = wmev_window_removed;
	wevent->event.wnd_id = wnd_id;
	list_append(&wevent->levents, &wmclient->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (wmclient->cb != NULL && wmclient->cb->ev_pending != NULL)
		wmclient->cb->ev_pending(wmclient->cb_arg);

	return EOK;
}

/** Post window changed event to the WM client's message queue.
 *
 * @param wmclient WM client
 * @param wnd_id Window ID of the added window
 *
 * @return EOK on success or an error code
 */
errno_t ds_wmclient_post_wnd_changed_event(ds_wmclient_t *wmclient,
    sysarg_t wnd_id)
{
	ds_wmclient_ev_t *wevent;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "wmclient_pos_wnd_changed_event "
	    "wmclient=%p wnd_id=%zu\n", (void *)wmclient, wnd_id);

	wevent = calloc(1, sizeof(ds_wmclient_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->wmclient = wmclient;
	wevent->event.etype = wmev_window_changed;
	wevent->event.wnd_id = wnd_id;
	list_append(&wevent->levents, &wmclient->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (wmclient->cb != NULL && wmclient->cb->ev_pending != NULL)
		wmclient->cb->ev_pending(wmclient->cb_arg);

	return EOK;
}

/** @}
 */
