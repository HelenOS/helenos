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
 * @file Display server display
 */

#include <errno.h>
#include <gfx/context.h>
#include <io/log.h>
#include <stdlib.h>
#include "client.h"
#include "seat.h"
#include "window.h"
#include "display.h"

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
	list_initialize(&disp->seats);
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
	assert(list_empty(&disp->seats));
	free(disp);
}

/** Add client to display.
 *
 * @param disp Display
 * @param client Client
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
 * @param client Client
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

	printf("ds_display_find_window: id=0x%x\n", (unsigned) id);

	client = ds_display_first_client(display);
	while (client != NULL) {
		printf("ds_display_find_window: client=%p\n", client);
		wnd = ds_client_find_window(client, id);
		if (wnd != NULL) {
			printf("ds_display_find_window: found wnd=%p id=0x%x\n",
			    wnd, (unsigned) wnd->id);
			return wnd;
		}
		client = ds_display_next_client(client);
	}

	printf("ds_display_find_window: not found\n");
	return NULL;
}

/** Post keyboard event to a display.
 *
 * The event is routed to the correct window by first determining the
 * seat the keyboard device belongs to and then the event is sent to the
 * window focused by that seat.
 *
 * @param display Display
 * @param event Event
 */
errno_t ds_display_post_kbd_event(ds_display_t *display, kbd_event_t *event)
{
	ds_seat_t *seat;

	// TODO Determine which seat the event belongs to
	seat = ds_display_first_seat(display);
	if (seat == NULL)
		return EOK;

	return ds_seat_post_kbd_event(seat, event);
}

/** Add seat to display.
 *
 * @param disp Display
 * @param seat Seat
 */
void ds_display_add_seat(ds_display_t *disp, ds_seat_t *seat)
{
	assert(seat->display == NULL);
	assert(!link_used(&seat->lseats));

	seat->display = disp;
	list_append(&seat->lseats, &disp->seats);
}

/** Remove seat from display.
 *
 * @param seat Seat
 */
void ds_display_remove_seat(ds_seat_t *seat)
{
	list_remove(&seat->lseats);
	seat->display = NULL;
}

/** Get first seat in display.
 *
 * @param disp Display
 * @return First seat or @c NULL if there is none
 */
ds_seat_t *ds_display_first_seat(ds_display_t *disp)
{
	link_t *link = list_first(&disp->seats);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_seat_t, lseats);
}

/** Get next seat in display.
 *
 * @param seat Current seat
 * @return Next seat or @c NULL if there is none
 */
ds_seat_t *ds_display_next_seat(ds_seat_t *seat)
{
	link_t *link = list_next(&seat->lseats, &seat->display->seats);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_seat_t, lseats);
}

/** @}
 */
