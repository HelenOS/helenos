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
 * @file Display server CFG client
 */

#include <adt/list.h>
#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include "cfgclient.h"
#include "display.h"
#include "window.h"

/** Create CFG client.
 *
 * @param display Parent display
 * @param cb CFG client callbacks
 * @param cb_arg Callback argument
 * @param rcfgclient Place to store pointer to new CFG client.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_cfgclient_create(ds_display_t *display, ds_cfgclient_cb_t *cb,
    void *cb_arg, ds_cfgclient_t **rcfgclient)
{
	ds_cfgclient_t *cfgclient;

	cfgclient = calloc(1, sizeof(ds_cfgclient_t));
	if (cfgclient == NULL)
		return ENOMEM;

	list_initialize(&cfgclient->events);
	cfgclient->cb = cb;
	cfgclient->cb_arg = cb_arg;

	ds_display_add_cfgclient(display, cfgclient);

	*rcfgclient = cfgclient;
	return EOK;
}

/** Destroy CFG client.
 *
 * @param cfgclient CFG client
 */
void ds_cfgclient_destroy(ds_cfgclient_t *cfgclient)
{
	ds_cfgclient_purge_events(cfgclient);
	ds_display_remove_cfgclient(cfgclient);
	free(cfgclient);
}

/** Get next event from CFG client event queue.
 *
 * @param cfgclient CFG client
 * @param event Place to store event
 * @return EOK on success, ENOENT if event queue is empty
 */
errno_t ds_cfgclient_get_event(ds_cfgclient_t *cfgclient, dispcfg_ev_t *event)
{
	link_t *link;
	ds_cfgclient_ev_t *wevent;

	link = list_first(&cfgclient->events);
	if (link == NULL)
		return ENOENT;

	wevent = list_get_instance(link, ds_cfgclient_ev_t, levents);
	list_remove(link);

	*event = wevent->event;
	free(wevent);
	return EOK;
}

/** Purge events from CFG client event queue.
 *
 * @param client Client
 */
void ds_cfgclient_purge_events(ds_cfgclient_t *cfgclient)
{
	link_t *cur;
	link_t *next;
	ds_cfgclient_ev_t *wevent;

	cur = list_first(&cfgclient->events);
	while (cur != NULL) {
		next = list_next(cur, &cfgclient->events);
		wevent = list_get_instance(cur, ds_cfgclient_ev_t, levents);

		list_remove(cur);
		free(wevent);
		cur = next;
	}
}

/** Post seat added event to the CFG client's message queue.
 *
 * @param cfgclient CFG client
 * @param seat_id Seat ID of the added seat
 *
 * @return EOK on success or an error code
 */
errno_t ds_cfgclient_post_seat_added_event(ds_cfgclient_t *cfgclient,
    sysarg_t seat_id)
{
	ds_cfgclient_ev_t *sevent;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "cfgclient_pos_seat_added_event "
	    "cfgclient=%p seat_id=%zu\n", (void *)cfgclient, seat_id);

	sevent = calloc(1, sizeof(ds_cfgclient_ev_t));
	if (sevent == NULL)
		return ENOMEM;

	sevent->cfgclient = cfgclient;
	sevent->event.etype = dcev_seat_added;
	sevent->event.seat_id = seat_id;
	list_append(&sevent->levents, &cfgclient->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (cfgclient->cb != NULL && cfgclient->cb->ev_pending != NULL)
		cfgclient->cb->ev_pending(cfgclient->cb_arg);

	return EOK;
}

/** Post seat removed event to the CFG client's message queue.
 *
 * @param cfgclient CFG client
 * @param seat_id Seat ID of the added seat
 *
 * @return EOK on success or an error code
 */
errno_t ds_cfgclient_post_seat_removed_event(ds_cfgclient_t *cfgclient,
    sysarg_t seat_id)
{
	ds_cfgclient_ev_t *sevent;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "cfgclient_pos_seat_removed_event "
	    "cfgclient=%p seat_id=%zu\n", (void *)cfgclient, seat_id);

	sevent = calloc(1, sizeof(ds_cfgclient_ev_t));
	if (sevent == NULL)
		return ENOMEM;

	sevent->cfgclient = cfgclient;
	sevent->event.etype = dcev_seat_removed;
	sevent->event.seat_id = seat_id;
	list_append(&sevent->levents, &cfgclient->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	if (cfgclient->cb != NULL && cfgclient->cb->ev_pending != NULL)
		cfgclient->cb->ev_pending(cfgclient->cb_arg);

	return EOK;
}

/** @}
 */
