/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <sif.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "client.h"
#include "cursor.h"
#include "display.h"
#include "idevcfg.h"
#include "seat.h"
#include "window.h"

static void ds_seat_get_pointer_rect(ds_seat_t *, gfx_rect_t *);
static errno_t ds_seat_repaint_pointer(ds_seat_t *, gfx_rect_t *);

/** Create seat.
 *
 * @param display Parent display
 * @param name Seat name
 * @param rseat Place to store pointer to new seat.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_seat_create(ds_display_t *display, const char *name,
    ds_seat_t **rseat)
{
	ds_seat_t *seat;
	ds_seat_t *s0;

	s0 = ds_display_first_seat(display);
	while (s0 != NULL) {
		if (str_cmp(s0->name, name) == 0)
			return EEXIST;
		s0 = ds_display_next_seat(s0);
	}

	seat = calloc(1, sizeof(ds_seat_t));
	if (seat == NULL)
		return ENOMEM;

	seat->name = str_dup(name);
	if (seat->name == NULL) {
		free(seat);
		return ENOMEM;
	}

	list_initialize(&seat->idevcfgs);

	ds_display_add_seat(display, seat);
	seat->pntpos.x = 0;
	seat->pntpos.y = 0;

	seat->client_cursor = display->cursor[dcurs_arrow];
	seat->wm_cursor = NULL;
	seat->focus = ds_display_first_window(display);

	*rseat = seat;
	return EOK;
}

/** Destroy seat.
 *
 * @param seat Seat
 */
void ds_seat_destroy(ds_seat_t *seat)
{
	ds_idevcfg_t *idevcfg;

	/* Remove all input device configuration entries pointing to this seat */
	idevcfg = ds_seat_first_idevcfg(seat);
	while (idevcfg != NULL) {
		ds_idevcfg_destroy(idevcfg);
		idevcfg = ds_seat_first_idevcfg(seat);
	}

	/* Remove this seat's focus */
	if (seat->focus != NULL)
		ds_window_post_unfocus_event(seat->focus);

	ds_display_remove_seat(seat);

	free(seat->name);
	free(seat);
}

/** Load seat from SIF node.
 *
 * @param display Display
 * @param snode Seat node from which to load the seat
 * @param rseat Place to store pointer to the newly loaded seat
 *
 * @return EOK on success or an error code
 */
errno_t ds_seat_load(ds_display_t *display, sif_node_t *snode,
    ds_seat_t **rseat)
{
	const char *sid;
	const char *name;
	char *endptr;
	unsigned long id;
	errno_t rc;

	sid = sif_node_get_attr(snode, "id");
	if (sid == NULL)
		return EIO;

	name = sif_node_get_attr(snode, "name");
	if (name == NULL)
		return EIO;

	id = strtoul(sid, &endptr, 10);
	if (*endptr != '\0')
		return EIO;

	rc = ds_seat_create(display, name, rseat);
	if (rc != EOK)
		return EIO;

	(*rseat)->id = id;
	return EOK;
}

/** Save seat to SIF node.
 *
 * @param seat Seat
 * @param snode Seat node into which the seat should be saved
 *
 * @return EOK on success or an error code
 */
errno_t ds_seat_save(ds_seat_t *seat, sif_node_t *snode)
{
	char *sid;
	errno_t rc;
	int rv;

	rv = asprintf(&sid, "%lu", (unsigned long)seat->id);
	if (rv < 0) {
		rc = ENOMEM;
		return rc;
	}

	rc = sif_node_set_attr(snode, "id", sid);
	if (rc != EOK) {
		free(sid);
		return rc;
	}

	free(sid);

	rc = sif_node_set_attr(snode, "name", seat->name);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Set seat focus to a window.
 *
 * @param seat Seat
 * @param wnd Window to focus
 */
void ds_seat_set_focus(ds_seat_t *seat, ds_window_t *wnd)
{
	errno_t rc;

	if (wnd == seat->focus) {
		/* Focus is not changing */
		return;
	}

	if (wnd != NULL) {
		rc = ds_window_unminimize(wnd);
		if (rc != EOK)
			return;
	}

	if (seat->focus != NULL)
		ds_window_post_unfocus_event(seat->focus);

	seat->focus = wnd;

	if (wnd != NULL) {
		ds_window_post_focus_event(wnd);
		ds_window_bring_to_top(wnd);
	}

	/* When focus changes, popup window should be closed */
	ds_seat_set_popup(seat, NULL);
}

/** Set seat popup window.
 *
 * @param seat Seat
 * @param wnd Popup window
 */
void ds_seat_set_popup(ds_seat_t *seat, ds_window_t *wnd)
{
	if (wnd == seat->popup)
		return;

	if (seat->popup != NULL) {
		/* Window is no longer the popup window, send close request */
		ds_client_post_close_event(seat->popup->client,
		    seat->popup);
	}

	seat->popup = wnd;
}

/** Evacuate seat references to window.
 *
 * If seat's focus is @a wnd, it will be set to NULL.
 * If seat's popup window is @a wnd, it will be set to NULL.
 *
 * @param seat Seat
 * @param wnd Window to evacuate references from
 */
void ds_seat_evac_wnd_refs(ds_seat_t *seat, ds_window_t *wnd)
{
	if (seat->focus == wnd)
		ds_seat_set_focus(seat, NULL);

	if (seat->popup == wnd)
		ds_seat_set_popup(seat, NULL);
}

/** Unfocus window.
 *
 * If seat's focus is @a wnd, it will be set to a different window
 * that is not minimized, preferably not a system window.
 *
 * @param seat Seat
 * @param wnd Window to remove focus from
 */
void ds_seat_unfocus_wnd(ds_seat_t *seat, ds_window_t *wnd)
{
	ds_window_t *nwnd;

	if (seat->focus != wnd)
		return;

	/* Find alternate window that is neither system nor minimized */
	nwnd = ds_window_find_prev(wnd, ~(wndf_minimized | wndf_system));

	if (nwnd == NULL) {
		/* Find alternate window that is not minimized */
		nwnd = ds_window_find_prev(wnd, ~wndf_minimized);
	}

	ds_seat_set_focus(seat, nwnd);
}

/** Switch focus to another window.
 *
 * @param seat Seat
 * @param wnd Window to evacuate focus from
 */
void ds_seat_switch_focus(ds_seat_t *seat)
{
	ds_window_t *nwnd;

	if (seat->focus != NULL) {
		/* Find alternate window that is not a system window */
		nwnd = ds_window_find_next(seat->focus, ~wndf_system);
	} else {
		/* Currently no focus. Focus topmost window. */
		nwnd = ds_display_first_window(seat->display);
	}

	/* Only switch focus if there is another window */
	if (nwnd != NULL)
		ds_seat_set_focus(seat, nwnd);
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
		ds_seat_switch_focus(seat);
		return EOK;
	}

	dwindow = seat->popup;
	if (dwindow == NULL)
		dwindow = seat->focus;

	if (dwindow == NULL)
		return EOK;

	return ds_window_post_kbd_event(dwindow, event);
}

/** Get current cursor used by seat.
 *
 * @param wmcurs WM curor
 * @param ccurs Client cursor
 * @return
 */
static ds_cursor_t *ds_seat_compute_cursor(ds_cursor_t *wmcurs, ds_cursor_t *ccurs)
{
	if (wmcurs != NULL)
		return wmcurs;

	return ccurs;
}

/** Get current cursor used by seat.
 *
 * @param seat Seat
 * @return Current cursor
 */
static ds_cursor_t *ds_seat_get_cursor(ds_seat_t *seat)
{
	return ds_seat_compute_cursor(seat->wm_cursor, seat->client_cursor);
}

/** Set client cursor.
 *
 * Set cursor selected by client. This may update the actual cursor
 * if WM is not overriding the cursor.
 *
 * @param seat Seat
 * @param cursor Client cursor
 */
static void ds_seat_set_client_cursor(ds_seat_t *seat, ds_cursor_t *cursor)
{
	ds_cursor_t *old_cursor;
	ds_cursor_t *new_cursor;
	gfx_rect_t old_rect;

	old_cursor = ds_seat_get_cursor(seat);
	new_cursor = ds_seat_compute_cursor(seat->wm_cursor, cursor);

	if (new_cursor != old_cursor) {
		ds_seat_get_pointer_rect(seat, &old_rect);
		seat->client_cursor = cursor;
		ds_seat_repaint_pointer(seat, &old_rect);
	} else {
		seat->client_cursor = cursor;
	}
}

/** Set WM cursor.
 *
 * Set cursor override for window management.
 *
 * @param seat Seat
 * @param cursor WM cursor override or @c NULL not to override the cursor
 */
void ds_seat_set_wm_cursor(ds_seat_t *seat, ds_cursor_t *cursor)
{
	ds_cursor_t *old_cursor;
	ds_cursor_t *new_cursor;
	gfx_rect_t old_rect;

	old_cursor = ds_seat_get_cursor(seat);
	new_cursor = ds_seat_compute_cursor(cursor, seat->client_cursor);

	if (new_cursor != old_cursor) {
		ds_seat_get_pointer_rect(seat, &old_rect);
		seat->wm_cursor = cursor;
		ds_seat_repaint_pointer(seat, &old_rect);
	} else {
		seat->wm_cursor = cursor;
	}
}

/** Get rectangle covered by pointer.
 *
 * @param seat Seat
 * @param rect Place to store rectangle
 */
void ds_seat_get_pointer_rect(ds_seat_t *seat, gfx_rect_t *rect)
{
	ds_cursor_t *cursor;

	cursor = ds_seat_get_cursor(seat);
	ds_cursor_get_rect(cursor, &seat->pntpos, rect);
}

/** Repaint seat pointer
 *
 * Repaint the pointer after it has moved or changed. This is done by
 * repainting the area of the display previously (@a old_rect) and currently
 * covered by the pointer.
 *
 * @param seat Seat
 * @param old_rect Rectangle previously covered by pointer
 *
 * @return EOK on success or an error code
 */
static errno_t ds_seat_repaint_pointer(ds_seat_t *seat, gfx_rect_t *old_rect)
{
	gfx_rect_t new_rect;
	gfx_rect_t envelope;
	errno_t rc;

	ds_seat_get_pointer_rect(seat, &new_rect);

	if (gfx_rect_is_incident(old_rect, &new_rect)) {
		/* Rectangles do not intersect. Repaint them separately. */
		rc = ds_display_paint(seat->display, &new_rect);
		if (rc != EOK)
			return rc;

		rc = ds_display_paint(seat->display, old_rect);
		if (rc != EOK)
			return rc;
	} else {
		/*
		 * Rectangles intersect. As an optimization, repaint them
		 * in a single operation.
		 */
		gfx_rect_envelope(old_rect, &new_rect, &envelope);

		rc = ds_display_paint(seat->display, &envelope);
		if (rc != EOK)
			return rc;
	}

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
errno_t ds_seat_post_ptd_event(ds_seat_t *seat, ptd_event_t *event)
{
	ds_display_t *disp = seat->display;
	gfx_coord2_t npos;
	gfx_rect_t old_rect;
	ds_window_t *wnd;
	pos_event_t pevent;
	errno_t rc;

	wnd = ds_display_window_by_pos(seat->display, &seat->pntpos);

	/* Focus window on button press */
	if (event->type == PTD_PRESS && event->btn_num == 1) {
		if (wnd != NULL && (wnd->flags & wndf_popup) == 0 &&
		    (wnd->flags & wndf_nofocus) == 0) {
			ds_seat_set_focus(seat, wnd);
		}
	}

	if (event->type == PTD_PRESS || event->type == PTD_RELEASE ||
	    event->type == PTD_DCLICK) {
		pevent.pos_id = event->pos_id;
		switch (event->type) {
		case PTD_PRESS:
			pevent.type = POS_PRESS;
			break;
		case PTD_RELEASE:
			pevent.type = POS_RELEASE;
			break;
		case PTD_DCLICK:
			pevent.type = POS_DCLICK;
			break;
		default:
			assert(false);
		}

		pevent.btn_num = event->btn_num;
		pevent.hpos = seat->pntpos.x;
		pevent.vpos = seat->pntpos.y;

		rc = ds_seat_post_pos_event(seat, &pevent);
		if (rc != EOK)
			return rc;
	}

	if (event->type == PTD_MOVE) {
		gfx_coord2_add(&seat->pntpos, &event->dmove, &npos);
		gfx_coord2_clip(&npos, &disp->rect, &npos);

		ds_seat_get_pointer_rect(seat, &old_rect);
		seat->pntpos = npos;

		pevent.pos_id = event->pos_id;
		pevent.type = POS_UPDATE;
		pevent.btn_num = 0;
		pevent.hpos = seat->pntpos.x;
		pevent.vpos = seat->pntpos.y;

		rc = ds_seat_post_pos_event(seat, &pevent);
		if (rc != EOK)
			return rc;

		ds_seat_repaint_pointer(seat, &old_rect);
	}

	if (event->type == PTD_ABS_MOVE) {
		/*
		 * Project input device area onto display area. Technically
		 * we probably want to project onto the area of a particular
		 * display device. The tricky part is figuring out which
		 * display device the input device is associated with.
		 */
		gfx_coord2_project(&event->apos, &event->abounds,
		    &disp->rect, &npos);

		gfx_coord2_clip(&npos, &disp->rect, &npos);

		ds_seat_get_pointer_rect(seat, &old_rect);
		seat->pntpos = npos;

		pevent.pos_id = event->pos_id;
		pevent.type = POS_UPDATE;
		pevent.btn_num = 0;
		pevent.hpos = seat->pntpos.x;
		pevent.vpos = seat->pntpos.y;

		rc = ds_seat_post_pos_event(seat, &pevent);
		if (rc != EOK)
			return rc;

		ds_seat_repaint_pointer(seat, &old_rect);
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
	ds_window_t *pwindow;
	ds_window_t *cwindow;
	errno_t rc;

	/* Window under pointer */
	pwindow = ds_display_window_by_pos(seat->display, &seat->pntpos);

	/* Current window: popup or focused */
	cwindow = seat->popup;
	if (cwindow == NULL)
		cwindow = seat->focus;

	/*
	 * Deliver move and release event to current window if different
	 * from pwindow
	 */
	if (event->type != POS_PRESS && cwindow != NULL &&
	    cwindow != pwindow) {
		rc = ds_window_post_pos_event(cwindow, event);
		if (rc != EOK)
			return rc;
	}

	if (pwindow != NULL) {
		/* Moving over a window */
		ds_seat_set_client_cursor(seat, pwindow->cursor);

		rc = ds_window_post_pos_event(pwindow, event);
		if (rc != EOK)
			return rc;
	} else {
		/* Not over a window */
		ds_seat_set_client_cursor(seat,
		    seat->display->cursor[dcurs_arrow]);
	}

	/* Click outside popup window */
	if (event->type == POS_PRESS && pwindow != seat->popup) {
		/* Close popup window */
		ds_seat_set_popup(seat, NULL);
	}

	return EOK;
}

/** Paint seat pointer.
 *
 * @param seat Seat whose pointer to paint
 * @param rect Clipping rectangle
 */
errno_t ds_seat_paint_pointer(ds_seat_t *seat, gfx_rect_t *rect)
{
	ds_cursor_t *cursor;

	cursor = ds_seat_get_cursor(seat);
	return ds_cursor_paint(cursor, &seat->pntpos, rect);
}

/** Add input device configuration entry to seat.
 *
 * @param seat Seat
 * @param idevcfg Input device configuration
 */
void ds_seat_add_idevcfg(ds_seat_t *seat, ds_idevcfg_t *idevcfg)
{
	assert(idevcfg->seat == NULL);
	assert(!link_used(&idevcfg->lseatidcfgs));

	idevcfg->seat = seat;
	list_append(&idevcfg->lseatidcfgs, &seat->idevcfgs);
}

/** Remove input device configuration entry from seat.
 *
 * @param idevcfg Input device configuration entry
 */
void ds_seat_remove_idevcfg(ds_idevcfg_t *idevcfg)
{
	list_remove(&idevcfg->lseatidcfgs);
	idevcfg->seat = NULL;
}

/** Get first input device configuration entry in seat.
 *
 * @param disp Display
 * @return First input device configuration entry or @c NULL if there is none
 */
ds_idevcfg_t *ds_seat_first_idevcfg(ds_seat_t *seat)
{
	link_t *link = list_first(&seat->idevcfgs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_idevcfg_t, lseatidcfgs);
}

/** Get next input device configuration entry in seat.
 *
 * @param idevcfg Current input device configuration entry
 * @return Next input device configuration entry or @c NULL if there is none
 */
ds_idevcfg_t *ds_seat_next_idevcfg(ds_idevcfg_t *idevcfg)
{
	link_t *link = list_next(&idevcfg->lseatidcfgs, &idevcfg->seat->idevcfgs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_idevcfg_t, lseatidcfgs);
}

/** @}
 */
