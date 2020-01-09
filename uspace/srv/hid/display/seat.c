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
#include <gfx/color.h>
#include <gfx/render.h>
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
	seat->pntpos.x = 0;
	seat->pntpos.y = 0;

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

/** Set seat focus to a window.
 *
 * @param seat Seat
 * @param wnd Window to focus
 */
void ds_seat_set_focus(ds_seat_t *seat, ds_window_t *wnd)
{
	seat->focus = wnd;
}

/** Evacuate focus from window.
 *
 * If seat's focus is @a wnd, it will be set to a different window.
 *
 * @param seat Seat
 * @param wnd Window to evacuate focus from
 */
void ds_seat_evac_focus(ds_seat_t *seat, ds_window_t *wnd)
{
	ds_window_t *nwnd;

	if (seat->focus == wnd) {
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

/** Draw cross at seat pointer position.
 *
 * @param seat Seat
 * @param len Cross length
 * @param w Cross extra width
 * @param br Brightness (0 to 65535)
 *
 * @return EOK on success or an error code
 */
static errno_t ds_seat_draw_cross(ds_seat_t *seat, gfx_coord_t len,
    gfx_coord_t w, uint16_t br)
{
	gfx_color_t *color = NULL;
	gfx_context_t *gc;
	gfx_rect_t rect, r0;
	errno_t rc;

	gc = ds_display_get_gc(seat->display);
	if (gc == NULL)
		return EOK;

	rc = gfx_color_new_rgb_i16(br, br, br, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(gc, color);
	if (rc != EOK)
		goto error;

	r0.p0.x = -len;
	r0.p0.y = -w;
	r0.p1.x = +len + 1;
	r0.p1.y = +w + 1;
	gfx_rect_translate(&seat->pntpos, &r0, &rect);

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		goto error;

	r0.p0.x = -w;
	r0.p0.y = -len;
	r0.p1.x = +w + 1;
	r0.p1.y = +len + 1;
	gfx_rect_translate(&seat->pntpos, &r0, &rect);

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);
	return EOK;
error:
	if (color != NULL)
		gfx_color_delete(color);
	return rc;
}

/** Draw or clear seat pointer
 *
 * @param seat Seat
 * @param shown @c true to display pointer, @c false to clear it
 *
 * @return EOK on success or an error code
 */
static errno_t ds_seat_draw_pointer(ds_seat_t *seat, bool shown)
{
	errno_t rc;

	rc = ds_seat_draw_cross(seat, 8, 1, 0);
	if (rc != EOK)
		return rc;

	rc = ds_seat_draw_cross(seat, 8, 0, shown ? 65535 : 0);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Post pointing device event to the seat
 *
 * Update pointer position and generate position event.
 *
 * @param seat Seat
 * @param event Event
 *
 * @return EOK on success or an error code
 */
#include <stdio.h>
errno_t ds_seat_post_ptd_event(ds_seat_t *seat, ptd_event_t *event)
{
	gfx_coord2_t npos;
	ds_window_t *wnd;
	pos_event_t pevent;
	errno_t rc;

	printf("ds_seat_post_ptd_event\n");
	wnd = ds_display_window_by_pos(seat->display, &seat->pntpos);

	/* Focus window on button press */
	if (event->type == PTD_PRESS && event->btn_num == 1) {
		printf("PTD_PRESS (button = %d)\n", event->btn_num);
		if (wnd != NULL) {
			printf("set focus\n");
			ds_seat_set_focus(seat, wnd);
		}
	}

	if (event->type == PTD_PRESS || event->type == PTD_RELEASE) {
		pevent.pos_id = 0;
		pevent.type = (event->type == PTD_PRESS) ?
		    POS_PRESS : POS_RELEASE;
		pevent.btn_num = event->btn_num;
		pevent.hpos = seat->pntpos.x;
		pevent.vpos = seat->pntpos.y;

		rc = ds_seat_post_pos_event(seat, &pevent);
		if (rc != EOK)
			return rc;
	}

	if (event->type == PTD_MOVE) {
		printf("PTD_MOVE\n");

		gfx_coord2_add(&seat->pntpos, &event->dmove, &npos);
		if (npos.x < 0)
			npos.x = 0;
		if (npos.y < 0)
			npos.y = 0;
		if (npos.x > 1024)
			npos.x = 1024;
		if (npos.y > 768)
			npos.y = 768;

		printf("clear pointer\n");
		(void) ds_seat_draw_pointer(seat, false);
		seat->pntpos = npos;

		pevent.pos_id = 0;
		pevent.type = POS_UPDATE;
		pevent.btn_num = 0;
		pevent.hpos = seat->pntpos.x;
		pevent.vpos = seat->pntpos.y;

		rc = ds_seat_post_pos_event(seat, &pevent);
		if (rc != EOK)
			return rc;

		printf("draw pointer\n");
		(void) ds_seat_draw_pointer(seat, true);
	}

	return EOK;
}

/** Post position event to seat.
 *
 * Deliver event to relevant windows.
 *
 * @param seat Seat
 * @param event Position event
 */
errno_t ds_seat_post_pos_event(ds_seat_t *seat, pos_event_t *event)
{
	ds_window_t *wnd;
	errno_t rc;

	printf("ds_seat_post_pos_event\n");

	wnd = ds_display_window_by_pos(seat->display, &seat->pntpos);
	if (wnd != NULL) {
		printf("send event to window at pointer position\n");
		rc = ds_window_post_pos_event(wnd, event);
		if (rc != EOK)
			return rc;
	}

	if (seat->focus != wnd) {
		printf("send event to focused window\n");

		rc = ds_window_post_pos_event(seat->focus, event);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** @}
 */
