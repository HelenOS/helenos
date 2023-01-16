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
 * @file Display server client
 */

#include <adt/list.h>
#include <errno.h>
#include <stdlib.h>
#include "client.h"
#include "display.h"
#include "seat.h"
#include "window.h"

/** Create client.
 *
 * @param display Parent display
 * @param cb Client callbacks
 * @param cb_arg Callback argument
 * @param rclient Place to store pointer to new client.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_client_create(ds_display_t *display, ds_client_cb_t *cb,
    void *cb_arg, ds_client_t **rclient)
{
	ds_client_t *client;

	client = calloc(1, sizeof(ds_client_t));
	if (client == NULL)
		return ENOMEM;

	list_initialize(&client->windows);
	list_initialize(&client->events);
	client->cb = cb;
	client->cb_arg = cb_arg;

	ds_display_add_client(display, client);

	*rclient = client;
	return EOK;
}

/** Destroy client.
 *
 * @param client Client
 */
void ds_client_destroy(ds_client_t *client)
{
	ds_window_t *window;

	window = ds_client_first_window(client);
	while (window != NULL) {
		ds_window_destroy(window);
		window = ds_client_first_window(client);
	}

	assert(list_empty(&client->windows));
	ds_display_remove_client(client);
	free(client);
}

/** Add window to client.
 *
 * @param client client
 * @param wnd Window
 */
void ds_client_add_window(ds_client_t *client, ds_window_t *wnd)
{
	assert(wnd->client == NULL);
	assert(!link_used(&wnd->lcwindows));

	wnd->client = client;
	wnd->id = client->display->next_wnd_id++;
	list_append(&wnd->lcwindows, &client->windows);
}

/** Remove window from client.
 *
 * @param wnd Window
 */
void ds_client_remove_window(ds_window_t *wnd)
{
	ds_seat_t *seat;

	/* Make sure window is no longer focused in any seat */
	seat = ds_display_first_seat(wnd->display);
	while (seat != NULL) {
		ds_seat_evac_wnd_refs(seat, wnd);
		seat = ds_display_next_seat(seat);
	}

	/* Make sure no event in the queue is referencing the window */
	ds_client_purge_window_events(wnd->client, wnd);

	list_remove(&wnd->lcwindows);
	wnd->client = NULL;
}

/** Find window by ID.
 *
 * @param client Client
 * @param id Window ID
 */
ds_window_t *ds_client_find_window(ds_client_t *client, ds_wnd_id_t id)
{
	ds_window_t *wnd;

	// TODO Make this faster
	wnd = ds_client_first_window(client);
	while (wnd != NULL) {
		if (wnd->id == id)
			return wnd;
		wnd = ds_client_next_window(wnd);
	}

	return NULL;
}

/** Get first window in client.
 *
 * @param client Client
 * @return First window or @c NULL if there is none
 */
ds_window_t *ds_client_first_window(ds_client_t *client)
{
	link_t *link = list_first(&client->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lcwindows);
}

/** Get next window in client.
 *
 * @param wnd Current window
 * @return Next window or @c NULL if there is none
 */
ds_window_t *ds_client_next_window(ds_window_t *wnd)
{
	link_t *link = list_next(&wnd->lcwindows, &wnd->client->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lcwindows);
}

/** Get next event from client event queue.
 *
 * @param client Client
 * @param ewindow Place to store pointer to window receiving the event
 * @param event Place to store event
 * @return EOK on success, ENOENT if event queue is empty
 */
errno_t ds_client_get_event(ds_client_t *client, ds_window_t **ewindow,
    display_wnd_ev_t *event)
{
	link_t *link;
	ds_window_ev_t *wevent;

	link = list_first(&client->events);
	if (link == NULL)
		return ENOENT;

	wevent = list_get_instance(link, ds_window_ev_t, levents);
	list_remove(link);

	*ewindow = wevent->window;
	*event = wevent->event;
	free(wevent);
	return EOK;
}

/** Purge events from client event queue referring to a window.
 *
 * @param client Client
 * @param window Window
 */
void ds_client_purge_window_events(ds_client_t *client,
    ds_window_t *window)
{
	link_t *cur;
	link_t *next;
	ds_window_ev_t *wevent;

	cur = list_first(&client->events);
	while (cur != NULL) {
		next = list_next(cur, &client->events);
		wevent = list_get_instance(cur, ds_window_ev_t, levents);

		if (wevent->window == window)
			list_remove(cur);

		cur = next;
	}
}

/** Post close event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_close_event(ds_client_t *client, ds_window_t *ewindow)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_close;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (client->cb != NULL && client->cb->ev_pending != NULL)
		client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** Post focus event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param event Focus event data
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_focus_event(ds_client_t *client, ds_window_t *ewindow,
    display_wnd_focus_ev_t *event)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_focus;
	wevent->event.ev.focus = *event;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (client->cb != NULL && client->cb->ev_pending != NULL)
		client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** Post keyboard event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param event Event
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_kbd_event(ds_client_t *client, ds_window_t *ewindow,
    kbd_event_t *event)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_kbd;
	wevent->event.ev.kbd = *event;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** Post position event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param event Event
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_pos_event(ds_client_t *client, ds_window_t *ewindow,
    pos_event_t *event)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_pos;
	wevent->event.ev.pos = *event;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (client->cb != NULL && client->cb->ev_pending != NULL)
		client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** Post resize event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param rect New window rectangle
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_resize_event(ds_client_t *client, ds_window_t *ewindow,
    gfx_rect_t *rect)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_resize;
	wevent->event.ev.resize.rect = *rect;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (client->cb != NULL && client->cb->ev_pending != NULL)
		client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** Post unfocus event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param event Unfocus event data
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_unfocus_event(ds_client_t *client, ds_window_t *ewindow,
    display_wnd_unfocus_ev_t *event)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.etype = wev_unfocus;
	wevent->event.ev.unfocus = *event;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (client->cb != NULL && client->cb->ev_pending != NULL)
		client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** @}
 */
