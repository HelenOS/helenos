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
 * @file Display server seat
 */

#include <adt/list.h>
#include <errno.h>
#include <stdlib.h>
#include "client.h"
#include "display.h"
#include "seat.h"
#include "window.h"

/** Create seat.
 *
 * @param display Parent display
 * @param rseat Place to store pointer to new seat.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_seat_create(ds_display_t *display, ds_seat_t **rseat)
{
	ds_seat_t *seat;

	seat = calloc(1, sizeof(ds_seat_t));
	if (seat == NULL)
		return ENOMEM;

	ds_display_add_seat(display, seat);

	*rseat = seat;
	return EOK;
}

/** Destroy seat.
 *
 * @param seat Seat
 */
void ds_seat_destroy(ds_seat_t *seat)
{
	ds_display_remove_seat(seat);
	free(seat);
}

void ds_seat_set_focus(ds_seat_t *seat, ds_window_t *wnd)
{
	seat->focus = wnd;
}

void ds_seat_evac_focus(ds_seat_t *seat, ds_window_t *wnd)
{
	ds_window_t *nwnd;

	if (seat->focus == wnd) {
		/* Focus a different window. XXX Delegate to WM */
		nwnd = ds_display_next_window(wnd);
		if (nwnd == NULL)
			nwnd = ds_display_first_window(wnd->display);
		if (nwnd == wnd)
			nwnd = NULL;

		ds_seat_set_focus(seat, nwnd);
	}
}

/** Post keyboard event to the seat's focused window.
 *
 * @param seat Seat
 * @param event Event
 *
 * @return EOK on success or an error code
 */
errno_t ds_seat_post_kbd_event(ds_seat_t *seat, kbd_event_t *event)
{
	ds_window_t *dwindow;
	bool alt_or_shift;

	alt_or_shift = event->mods & (KM_SHIFT | KM_ALT);
	if (event->type == KEY_PRESS && alt_or_shift && event->key == KC_TAB) {
		/* On Alt-Tab or Shift-Tab, switch focus to next window */
		ds_seat_evac_focus(seat, seat->focus);
		return EOK;
	}

	dwindow = seat->focus;
	if (dwindow == NULL)
		return EOK;

	return ds_client_post_kbd_event(dwindow->client, dwindow, event);
}

/** @}
 */
