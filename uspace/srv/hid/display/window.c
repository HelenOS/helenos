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
 * @file Display server window
 */

#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/log.h>
#include <io/pixelmap.h>
#include <macros.h>
#include <memgfx/memgc.h>
#include <stdlib.h>
#include <str.h>
#include <wndmgt.h>
#include "client.h"
#include "display.h"
#include "seat.h"
#include "window.h"
#include "wmclient.h"

static void ds_window_invalidate_cb(void *, gfx_rect_t *);
static void ds_window_update_cb(void *);
static void ds_window_get_preview_rect(ds_window_t *, gfx_rect_t *);

static mem_gc_cb_t ds_window_mem_gc_cb = {
	.invalidate = ds_window_invalidate_cb,
	.update = ds_window_update_cb
};

/** Create window.
 *
 * Create graphics context for rendering into a window.
 *
 * @param client Client owning the window
 * @param params Window parameters
 * @param rwnd Place to store pointer to new window.
 *
 * @return EOK on success or an error code
 */
errno_t ds_window_create(ds_client_t *client, display_wnd_params_t *params,
    ds_window_t **rwnd)
{
	ds_window_t *wnd = NULL;
	ds_seat_t *seat;
	gfx_context_t *dgc;
	gfx_coord2_t dims;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	wnd = calloc(1, sizeof(ds_window_t));
	if (wnd == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Caption */
	wnd->caption = str_dup(params->caption);
	if (wnd->caption == NULL) {
		rc = ENOMEM;
		goto error;
	}

	wnd->flags = params->flags;

	ds_client_add_window(client, wnd);
	ds_display_add_window(client->display, wnd);

	gfx_bitmap_params_init(&bparams);
	bparams.rect = params->rect;

	/* Allocate window bitmap */

	dgc = ds_display_get_gc(wnd->display);
	if (dgc != NULL) {
		rc = gfx_bitmap_create(dgc, &bparams, NULL, &wnd->bitmap);
		if (rc != EOK)
			goto error;

		rc = gfx_bitmap_get_alloc(wnd->bitmap, &alloc);
		if (rc != EOK)
			goto error;

		gfx_rect_dims(&params->rect, &dims);
		wnd->pixelmap.width = dims.x;
		wnd->pixelmap.height = dims.y;
		wnd->pixelmap.data = alloc.pixels;
	} else {
		/* This is just for unit tests */
		gfx_rect_dims(&params->rect, &dims);
		alloc.pitch = dims.x * sizeof(uint32_t);
		alloc.off0 = 0;
		alloc.pixels = calloc(1, alloc.pitch * dims.y);
	}

	rc = mem_gc_create(&params->rect, &alloc, &ds_window_mem_gc_cb,
	    (void *)wnd, &wnd->mgc);
	if (rc != EOK)
		goto error;

	wnd->rect = params->rect;
	wnd->min_size = params->min_size;
	wnd->gc = mem_gc_get_ctx(wnd->mgc);
	wnd->cursor = wnd->display->cursor[dcurs_arrow];

	if ((params->flags & wndf_setpos) != 0) {
		/* Specific window position */
		wnd->dpos = params->pos;
	} else {
		/* Automatic window placement */
		wnd->dpos.x = ((wnd->id - 1) & 1) * 400;
		wnd->dpos.y = ((wnd->id - 1) & 2) / 2 * 300;
	}

	/* Determine which seat should own the window */
	if (params->idev_id != 0)
		seat = ds_display_seat_by_idev(wnd->display, params->idev_id);
	else
		seat = ds_display_default_seat(wnd->display);

	/* Is this a popup window? */
	if ((params->flags & wndf_popup) != 0) {
		ds_seat_set_popup(seat, wnd);
	} else {
		if ((params->flags & wndf_nofocus) == 0)
			ds_seat_set_focus(seat, wnd);
	}

	/* Is this window a panel? */
	if ((params->flags & wndf_avoid) != 0)
		ds_display_update_max_rect(wnd->display);

	(void) ds_display_paint(wnd->display, NULL);

	*rwnd = wnd;
	return EOK;
error:
	if (wnd != NULL) {
		ds_client_remove_window(wnd);
		ds_display_remove_window(wnd);
		if (wnd->mgc != NULL)
			mem_gc_delete(wnd->mgc);
		if (wnd->bitmap != NULL)
			gfx_bitmap_destroy(wnd->bitmap);
		if (wnd->caption != NULL)
			free(wnd->caption);
		free(wnd);
	}

	return rc;
}

/** Destroy window.
 *
 * @param wnd Window
 */
void ds_window_destroy(ds_window_t *wnd)
{
	ds_display_t *disp;

	disp = wnd->display;

	ds_window_unfocus(wnd);

	ds_client_remove_window(wnd);
	ds_display_remove_window(wnd);

	if ((wnd->flags & wndf_avoid) != 0)
		ds_display_update_max_rect(disp);

	mem_gc_delete(wnd->mgc);

	if (wnd->bitmap != NULL)
		gfx_bitmap_destroy(wnd->bitmap);

	free(wnd->caption);
	free(wnd);

	(void) ds_display_paint(disp, NULL);
}

/** Bring window to top.
 *
 * @param wnd Window
 */
void ds_window_bring_to_top(ds_window_t *wnd)
{
	ds_display_window_to_top(wnd);
	(void) ds_display_paint(wnd->display, NULL);
}

/** Get generic graphic context from window.
 *
 * @param wnd Window
 * @return Graphic context
 */
gfx_context_t *ds_window_get_ctx(ds_window_t *wnd)
{
	return wnd->gc;
}

/** Determine if window is visible.
 *
 * @param wnd Window
 * @return @c true iff window is visible
 */
bool ds_window_is_visible(ds_window_t *wnd)
{
	return (wnd->flags & wndf_minimized) == 0;
}

/** Paint a window using its backing bitmap.
 *
 * @param wnd Window to paint
 * @param rect Display rectangle to paint to
 * @return EOK on success or an error code
 */
errno_t ds_window_paint(ds_window_t *wnd, gfx_rect_t *rect)
{
	gfx_rect_t srect;
	gfx_rect_t *brect;
	gfx_rect_t crect;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "ds_window_paint");

	/* Skip painting the window if not visible */
	if (!ds_window_is_visible(wnd))
		return EOK;

	if (rect != NULL) {
		gfx_rect_rtranslate(&wnd->dpos, rect, &srect);

		/* Determine if we have anything to do */
		gfx_rect_clip(&srect, &wnd->rect, &crect);
		if (gfx_rect_is_empty(&crect))
			return EOK;

		brect = &srect;
	} else {
		brect = NULL;
	}

	/* This can happen in unit tests */
	if (wnd->bitmap == NULL)
		return EOK;

	return gfx_bitmap_render(wnd->bitmap, brect, &wnd->dpos);
}

/** Get the preview rectangle for a window.
 *
 * Get the preview rectangle if the window is being resized or moved.
 * If the window is not being resized or moved, return an empty rectangle.
 *
 * @param wnd Window
 * @param rect Place to store preview rectangle
 */
static void ds_window_get_preview_rect(ds_window_t *wnd, gfx_rect_t *rect)
{
	switch (wnd->state) {
	case dsw_idle:
		break;
	case dsw_moving:
		gfx_rect_translate(&wnd->preview_pos, &wnd->rect, rect);
		return;
	case dsw_resizing:
		gfx_rect_translate(&wnd->dpos, &wnd->preview_rect, rect);
		return;
	}

	rect->p0.x = 0;
	rect->p0.y = 0;
	rect->p1.x = 0;
	rect->p1.y = 0;
}

/** Paint window preview if the window is being moved or resized.
 *
 * If the window is not being resized or moved, take no action and return
 * success.
 *
 * @param wnd Window for which to paint preview
 * @param rect Clipping rectangle
 * @return EOK on success or an error code
 */
errno_t ds_window_paint_preview(ds_window_t *wnd, gfx_rect_t *rect)
{
	errno_t rc;
	gfx_color_t *color;
	gfx_rect_t prect;
	gfx_rect_t dr;
	gfx_rect_t pr;
	gfx_context_t *gc;

	/*
	 * Get preview rectangle. If the window is not being resized/moved,
	 * we should get an empty rectangle.
	 */
	ds_window_get_preview_rect(wnd, &prect);
	if (gfx_rect_is_empty(&prect)) {
		/* There is nothing to paint */
		return EOK;
	}

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		return rc;

	gc = ds_display_get_gc(wnd->display);
	if (gc != NULL) {
		gfx_set_color(gc, color);

		/*
		 * TODO: Ideally we'd want XOR operation to make the preview
		 * frame visible on any background. If we wanted to get really
		 * fancy, we'd fill it with a pattern
		 */

		pr.p0.x = prect.p0.x;
		pr.p0.y = prect.p0.y;
		pr.p1.x = prect.p1.x;
		pr.p1.y = prect.p0.y + 1;
		gfx_rect_clip(&pr, rect, &dr);
		gfx_fill_rect(gc, &dr);

		pr.p0.x = prect.p0.x;
		pr.p0.y = prect.p1.y - 1;
		pr.p1.x = prect.p1.x;
		pr.p1.y = prect.p1.y;
		gfx_rect_clip(&pr, rect, &dr);
		gfx_fill_rect(gc, &dr);

		pr.p0.x = prect.p0.x;
		pr.p0.y = prect.p0.y;
		pr.p1.x = prect.p0.x + 1;
		pr.p1.y = prect.p1.y;
		gfx_rect_clip(&pr, rect, &dr);
		gfx_fill_rect(gc, &dr);

		pr.p0.x = prect.p1.x - 1;
		pr.p0.y = prect.p0.y;
		pr.p1.x = prect.p1.x;
		pr.p1.y = prect.p1.y;
		gfx_rect_clip(&pr, rect, &dr);
		gfx_fill_rect(gc, &dr);

	}

	gfx_color_delete(color);
	return EOK;
}

/** Repaint window preview when resizing or moving.
 *
 * Repaint the window preview wich was previously at rectangle @a old_rect.
 * The current preview rectangle is determined from window state. If
 * the window did not previously have a preview, @a old_rect should point
 * to an empty rectangle or be NULL. When window has finished
 * moving or resizing, the preview will be cleared.
 *
 * @param wnd Window for which to paint preview
 * @param rect Clipping rectangle
 * @return EOK on success or an error code
 */
static errno_t ds_window_repaint_preview(ds_window_t *wnd, gfx_rect_t *old_rect)
{
	errno_t rc;
	gfx_rect_t prect;
	gfx_rect_t envelope;
	bool oldr;
	bool newr;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "ds_window_repaint_preview");

	/*
	 * Get current preview rectangle. If the window is not being resized/moved,
	 * we should get an empty rectangle.
	 */
	ds_window_get_preview_rect(wnd, &prect);

	oldr = (old_rect != NULL) && !gfx_rect_is_empty(old_rect);
	newr = !gfx_rect_is_empty(&prect);

	if (oldr && newr && gfx_rect_is_incident(old_rect, &prect)) {
		/*
		 * As an optimization, repaint both rectangles in a single
		 * operation.
		 */

		gfx_rect_envelope(old_rect, &prect, &envelope);

		rc = ds_display_paint(wnd->display, &envelope);
		if (rc != EOK)
			return rc;
	} else {
		/* Repaint each rectangle separately */
		if (oldr) {
			rc = ds_display_paint(wnd->display, old_rect);
			if (rc != EOK)
				return rc;
		}

		if (newr) {
			rc = ds_display_paint(wnd->display, &prect);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

/** Start moving a window by mouse drag.
 *
 * @param wnd Window
 * @param pos Position where mouse button was pressed
 * @param pos_id Positioning device ID
 */
static void ds_window_start_move(ds_window_t *wnd, gfx_coord2_t *pos,
    sysarg_t pos_id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_start_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	wnd->orig_pos = *pos;
	wnd->orig_pos_id = pos_id;
	wnd->state = dsw_moving;
	wnd->preview_pos = wnd->dpos;

	(void) ds_window_repaint_preview(wnd, NULL);
}

/** Finish moving a window by mouse drag.
 *
 * @param wnd Window
 * @param pos Position where mouse button was released
 */
static void ds_window_finish_move(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_coord2_t dmove;
	gfx_coord2_t nwpos;
	gfx_rect_t old_rect;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_finish_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_moving);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dmove);
	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);

	ds_window_get_preview_rect(wnd, &old_rect);

	wnd->dpos = nwpos;
	wnd->state = dsw_idle;
	wnd->orig_pos_id = 0;

	(void) ds_display_paint(wnd->display, NULL);
}

/** Update window position when moving by mouse drag.
 *
 * @param wnd Window
 * @param pos Current mouse position
 */
static void ds_window_update_move(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_coord2_t dmove;
	gfx_coord2_t nwpos;
	gfx_rect_t old_rect;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "ds_window_update_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_moving);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dmove);
	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);

	ds_window_get_preview_rect(wnd, &old_rect);
	wnd->preview_pos = nwpos;

	(void) ds_window_repaint_preview(wnd, &old_rect);
}

/** Start resizing a window by mouse drag.
 *
 * @param wnd Window
 * @param rsztype Resize type (which part of window is being dragged)
 * @param pos Position where mouse button was pressed
 * @param pos_id Positioning device ID
 */
static void ds_window_start_resize(ds_window_t *wnd,
    display_wnd_rsztype_t rsztype, gfx_coord2_t *pos, sysarg_t pos_id)
{
	ds_seat_t *seat;
	display_stock_cursor_t ctype;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_start_resize (%d, %d)",
	    (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	/* Determine which seat started the resize */
	seat = ds_display_seat_by_idev(wnd->display, pos_id);
	if (seat == NULL)
		return;

	wnd->orig_pos = *pos;
	wnd->orig_pos_id = pos_id;
	wnd->state = dsw_resizing;
	wnd->rsztype = rsztype;
	wnd->preview_rect = wnd->rect;

	ctype = display_cursor_from_wrsz(rsztype);
	ds_seat_set_wm_cursor(seat, wnd->display->cursor[ctype]);

	(void) ds_window_repaint_preview(wnd, NULL);
}

/** Finish resizing a window by mouse drag.
 *
 * @param wnd Window
 * @param pos Position where mouse button was released
 */
static void ds_window_finish_resize(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_coord2_t dresize;
	gfx_rect_t nrect;
	ds_seat_t *seat;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_finish_resize (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_resizing);
	gfx_coord2_subtract(pos, &wnd->orig_pos, &dresize);

	/* Compute new rectangle */
	ds_window_calc_resize(wnd, &dresize, &nrect);

	wnd->state = dsw_idle;
	ds_client_post_resize_event(wnd->client, wnd, &nrect);

	/* Determine which seat started the resize */
	seat = ds_display_seat_by_idev(wnd->display, wnd->orig_pos_id);
	if (seat != NULL)
		ds_seat_set_wm_cursor(seat, NULL);

	wnd->orig_pos_id = 0;

	(void) ds_display_paint(wnd->display, NULL);
}

/** Update window position when resizing by mouse drag.
 *
 * @param wnd Window
 * @param pos Current mouse position
 */
static void ds_window_update_resize(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_coord2_t dresize;
	gfx_rect_t nrect;
	gfx_rect_t old_rect;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "ds_window_update_resize (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_resizing);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dresize);
	ds_window_calc_resize(wnd, &dresize, &nrect);

	ds_window_get_preview_rect(wnd, &old_rect);
	wnd->preview_rect = nrect;
	(void) ds_window_repaint_preview(wnd, &old_rect);
}

/** Post keyboard event to window.
 *
 * @param wnd Window
 * @param event Event
 *
 * @return EOK on success or an error code
 */
errno_t ds_window_post_kbd_event(ds_window_t *wnd, kbd_event_t *event)
{
	bool alt_or_shift;

	alt_or_shift = event->mods & (KM_SHIFT | KM_ALT);

	if (event->type == KEY_PRESS && alt_or_shift && event->key == KC_F4) {
		/* On Alt-F4 or Shift-F4 send close event to the window */
		ds_client_post_close_event(wnd->client, wnd);
		return EOK;
	}

	return ds_client_post_kbd_event(wnd->client, wnd, event);
}

/** Post position event to window.
 *
 * @param wnd Window
 * @param event Position event
 *
 * @return EOK on success or an error code
 */
errno_t ds_window_post_pos_event(ds_window_t *wnd, pos_event_t *event)
{
	pos_event_t tevent;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	gfx_rect_t drect;
	bool inside;

	log_msg(LOG_DEFAULT, LVL_DEBUG2,
	    "ds_window_post_pos_event type=%d pos=%d,%d", event->type,
	    (int) event->hpos, (int) event->vpos);

	pos.x = event->hpos;
	pos.y = event->vpos;
	pos_id = event->pos_id;
	gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);
	inside = gfx_pix_inside_rect(&pos, &drect);

	if (event->type == POS_PRESS && event->btn_num == 2 && inside &&
	    (wnd->flags & wndf_maximized) == 0) {
		ds_window_start_move(wnd, &pos, pos_id);
		return EOK;
	}

	if (event->type == POS_RELEASE) {
		/* Finish move/resize if they were started by the same seat */
		if (wnd->state == dsw_moving &&
		    ds_window_orig_seat(wnd, pos_id)) {
			ds_window_finish_move(wnd, &pos);
			return EOK;
		}

		if (wnd->state == dsw_resizing &&
		    ds_window_orig_seat(wnd, pos_id)) {
			ds_window_finish_resize(wnd, &pos);
			return EOK;
		}
	}

	if (event->type == POS_UPDATE) {
		/* Update move/resize if they were started by the same seat */
		if (wnd->state == dsw_moving &&
		    ds_window_orig_seat(wnd, pos_id)) {
			ds_window_update_move(wnd, &pos);
			return EOK;
		}

		if (wnd->state == dsw_resizing &&
		    ds_window_orig_seat(wnd, pos_id)) {
			ds_window_update_resize(wnd, &pos);
			return EOK;
		}
	}

	/* Transform event coordinates to window-local */
	tevent = *event;
	tevent.hpos -= wnd->dpos.x;
	tevent.vpos -= wnd->dpos.y;

	return ds_client_post_pos_event(wnd->client, wnd, &tevent);
}

/** Post focus event to window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_post_focus_event(ds_window_t *wnd)
{
	display_wnd_focus_ev_t efocus;
	errno_t rc;
	ds_wmclient_t *wmclient;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_post_focus_event");

	/* Increase focus counter */
	++wnd->nfocus;
	efocus.nfocus = wnd->nfocus;

	rc = ds_client_post_focus_event(wnd->client, wnd, &efocus);
	if (rc != EOK)
		return rc;

	/* Notify window managers about window information change */
	wmclient = ds_display_first_wmclient(wnd->display);
	while (wmclient != NULL) {
		ds_wmclient_post_wnd_changed_event(wmclient, wnd->id);
		wmclient = ds_display_next_wmclient(wmclient);
	}

	return EOK;
}

/** Post unfocus event to window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_post_unfocus_event(ds_window_t *wnd)
{
	display_wnd_unfocus_ev_t eunfocus;
	errno_t rc;
	ds_wmclient_t *wmclient;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_post_unfocus_event");

	/* Decrease focus counter */
	--wnd->nfocus;
	eunfocus.nfocus = wnd->nfocus;

	rc = ds_client_post_unfocus_event(wnd->client, wnd, &eunfocus);
	if (rc != EOK)
		return rc;

	/* Notify window managers about window information change */
	wmclient = ds_display_first_wmclient(wnd->display);
	while (wmclient != NULL) {
		ds_wmclient_post_wnd_changed_event(wmclient, wnd->id);
		wmclient = ds_display_next_wmclient(wmclient);
	}

	return EOK;
}

/** Start moving a window, detected by client.
 *
 * @param wnd Window
 * @param pos Position where the pointer was when the move started
 *            relative to the window
 * @param pos_id Positioning device ID
 * @param event Button press event
 */
void ds_window_move_req(ds_window_t *wnd, gfx_coord2_t *pos, sysarg_t pos_id)
{
	gfx_coord2_t orig_pos;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_move_req (%d, %d)",
	    (int) pos->x, (int) pos->y);

	gfx_coord2_add(&wnd->dpos, pos, &orig_pos);
	ds_window_start_move(wnd, &orig_pos, pos_id);
}

/** Move window.
 *
 * @param wnd Window
 */
void ds_window_move(ds_window_t *wnd, gfx_coord2_t *dpos)
{
	wnd->dpos = *dpos;
	(void) ds_display_paint(wnd->display, NULL);
}

/** Get window position.
 *
 * @param wnd Window
 */
void ds_window_get_pos(ds_window_t *wnd, gfx_coord2_t *dpos)
{
	*dpos = wnd->dpos;
}

/** Get maximized window rectangle.
 *
 * @param wnd Window
 */
void ds_window_get_max_rect(ds_window_t *wnd, gfx_rect_t *rect)
{
	*rect = wnd->display->max_rect;
}

/** Start resizing a window, detected by client.
 *
 * @param wnd Window
 * @param rsztype Resize type (which part of window is being dragged)
 * @param pos Position where the pointer was when the resize started
 *            relative to the window
 * @param pos_id Positioning device ID
 * @param event Button press event
 */
void ds_window_resize_req(ds_window_t *wnd, display_wnd_rsztype_t rsztype,
    gfx_coord2_t *pos, sysarg_t pos_id)
{
	gfx_coord2_t orig_pos;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_resize_req (%d, %d, %d, %d)",
	    (int)rsztype, (int)pos->x, (int)pos->y, (int)pos_id);

	gfx_coord2_add(&wnd->dpos, pos, &orig_pos);
	ds_window_start_resize(wnd, rsztype, &orig_pos, pos_id);
}

/** Resize window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_resize(ds_window_t *wnd, gfx_coord2_t *offs,
    gfx_rect_t *nrect)
{
	gfx_context_t *dgc;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_t *nbitmap;
	pixelmap_t npixelmap;
	gfx_coord2_t dims;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t ndpos;
	errno_t rc;

	dgc = ds_display_get_gc(wnd->display);
	if (dgc != NULL) {
		gfx_bitmap_params_init(&bparams);
		bparams.rect = *nrect;

		rc = gfx_bitmap_create(dgc, &bparams, NULL, &nbitmap);
		if (rc != EOK)
			return ENOMEM;

		rc = gfx_bitmap_get_alloc(nbitmap, &alloc);
		if (rc != EOK) {
			gfx_bitmap_destroy(nbitmap);
			return ENOMEM;
		}

		gfx_rect_dims(nrect, &dims);
		npixelmap.width = dims.x;
		npixelmap.height = dims.y;
		npixelmap.data = alloc.pixels;

		/* TODO: Transfer contents within overlap */

		if (wnd->bitmap != NULL)
			gfx_bitmap_destroy(wnd->bitmap);

		wnd->bitmap = nbitmap;
		wnd->pixelmap = npixelmap;

		/* Point memory GC to the new bitmap */
		mem_gc_retarget(wnd->mgc, nrect, &alloc);
	}

	gfx_coord2_add(&wnd->dpos, offs, &ndpos);

	wnd->dpos = ndpos;
	wnd->rect = *nrect;

	if ((wnd->flags & wndf_avoid) != 0)
		ds_display_update_max_rect(wnd->display);

	(void) ds_display_paint(wnd->display, NULL);
	return EOK;
}

/** Minimize window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_minimize(ds_window_t *wnd)
{
	/* If already minimized, do nothing and return success. */
	if ((wnd->flags & wndf_minimized) != 0)
		return EOK;

	ds_window_unfocus(wnd);

	wnd->flags |= wndf_minimized;
	(void) ds_display_paint(wnd->display, NULL);
	return EOK;
}

/** Unminimize window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_unminimize(ds_window_t *wnd)
{
	/* If not minimized, do nothing and return success. */
	if ((wnd->flags & wndf_minimized) == 0)
		return EOK;

	wnd->flags &= ~wndf_minimized;
	(void) ds_display_paint(wnd->display, NULL);
	return EOK;
}

/** Maximize window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_maximize(ds_window_t *wnd)
{
	gfx_coord2_t old_dpos;
	gfx_rect_t old_rect;
	gfx_coord2_t offs;
	gfx_rect_t max_rect;
	gfx_rect_t nrect;
	errno_t rc;

	/* If already maximized, do nothing and return success. */
	if ((wnd->flags & wndf_maximized) != 0)
		return EOK;

	/* Remember the old window rectangle and display position */
	old_rect = wnd->rect;
	old_dpos = wnd->dpos;

	ds_window_get_max_rect(wnd, &max_rect);

	/* Keep window contents on the same position on the screen */
	offs.x = max_rect.p0.x - wnd->dpos.x;
	offs.y = max_rect.p0.y - wnd->dpos.y;

	/* Maximized window's coordinates will start at 0,0 */
	gfx_rect_rtranslate(&max_rect.p0, &max_rect, &nrect);

	rc = ds_window_resize(wnd, &offs, &nrect);
	if (rc != EOK)
		return rc;

	/* Set window flags, remember normal rectangle */
	wnd->flags |= wndf_maximized;
	wnd->normal_rect = old_rect;
	wnd->normal_dpos = old_dpos;

	return EOK;
}

/** Unmaximize window.
 *
 * @param wnd Window
 * @return EOK on success or an error code
 */
errno_t ds_window_unmaximize(ds_window_t *wnd)
{
	gfx_coord2_t offs;
	errno_t rc;

	/* If not maximized, do nothing and return success. */
	if ((wnd->flags & wndf_maximized) == 0)
		return EOK;

	/* Keep window contents on the same position on the screen */
	offs.x = wnd->normal_dpos.x - wnd->dpos.x;
	offs.y = wnd->normal_dpos.y - wnd->dpos.y;

	rc = ds_window_resize(wnd, &offs, &wnd->normal_rect);
	if (rc != EOK)
		return rc;

	/* Clear maximized flag */
	wnd->flags &= ~wndf_maximized;

	return EOK;
}

/** Compute new window rectangle after resize operation.
 *
 * @param wnd Window which is being resized (in dsw_resizing state and thus
 *            has rsztype set)
 * @param dresize Amount by which to resize
 * @param nrect Place to store new rectangle
 */
void ds_window_calc_resize(ds_window_t *wnd, gfx_coord2_t *dresize,
    gfx_rect_t *nrect)
{
	if ((wnd->rsztype & display_wr_top) != 0) {
		nrect->p0.y = min(wnd->rect.p0.y + dresize->y,
		    wnd->rect.p1.y - wnd->min_size.y);
	} else {
		nrect->p0.y = wnd->rect.p0.y;
	}

	if ((wnd->rsztype & display_wr_left) != 0) {
		nrect->p0.x = min(wnd->rect.p0.x + dresize->x,
		    wnd->rect.p1.x - wnd->min_size.x);
	} else {
		nrect->p0.x = wnd->rect.p0.x;
	}

	if ((wnd->rsztype & display_wr_bottom) != 0) {
		nrect->p1.y = max(wnd->rect.p1.y + dresize->y,
		    wnd->rect.p0.y + wnd->min_size.y);
	} else {
		nrect->p1.y = wnd->rect.p1.y;
	}

	if ((wnd->rsztype & display_wr_right) != 0) {
		nrect->p1.x = max(wnd->rect.p1.x + dresize->x,
		    wnd->rect.p0.x + wnd->min_size.x);
	} else {
		nrect->p1.x = wnd->rect.p1.x;
	}
}

/** Set window cursor.
 *
 * @param wnd Window
 * @param cursor New cursor
 * @return EOK on success, EINVAL if @a cursor is invalid
 */
errno_t ds_window_set_cursor(ds_window_t *wnd, display_stock_cursor_t cursor)
{
	if (cursor >= dcurs_arrow &&
	    cursor < (display_stock_cursor_t) dcurs_limit) {
		wnd->cursor = wnd->display->cursor[cursor];
		return EOK;
	} else {
		return EINVAL;
	}
}

/** Set window caption.
 *
 * @param wnd Window
 * @param caption New caption
 *
 * @return EOK on success, EINVAL if @a cursor is invalid
 */
errno_t ds_window_set_caption(ds_window_t *wnd, const char *caption)
{
	char *dcaption;
	ds_wmclient_t *wmclient;

	dcaption = str_dup(caption);
	if (dcaption == NULL)
		return ENOMEM;

	free(wnd->caption);
	wnd->caption = dcaption;

	/* Notify window managers about window information change */
	wmclient = ds_display_first_wmclient(wnd->display);
	while (wmclient != NULL) {
		ds_wmclient_post_wnd_changed_event(wmclient, wnd->id);
		wmclient = ds_display_next_wmclient(wmclient);
	}

	return EOK;
}

/** Find alternate window with the allowed flags.
 *
 * An alternate window is a *different* window that is preferably previous
 * in the display order and only has the @a allowed flags.
 *
 * @param wnd Window
 * @param allowed_flags Bitmask of flags that the window is allowed to have
 *
 * @return Alternate window matching the criteria or @c NULL if there is none
 */
ds_window_t *ds_window_find_prev(ds_window_t *wnd,
    display_wnd_flags_t allowed_flags)
{
	ds_window_t *nwnd;

	/* Try preceding windows in display order */
	nwnd = ds_display_next_window(wnd);
	while (nwnd != NULL && (nwnd->flags & ~allowed_flags) != 0) {
		nwnd = ds_display_next_window(nwnd);
	}

	/* Do we already have a matching window? */
	if (nwnd != NULL && (nwnd->flags & ~allowed_flags) == 0) {
		return nwnd;
	}

	/* Try succeeding windows in display order */
	nwnd = ds_display_first_window(wnd->display);
	while (nwnd != NULL && nwnd != wnd &&
	    (nwnd->flags & ~allowed_flags) != 0) {
		nwnd = ds_display_next_window(nwnd);
	}

	if (nwnd == wnd)
		return NULL;

	return nwnd;
}

/** Find alternate window with the allowed flags.
 *
 * An alternate window is a *different* window that is preferably previous
 * in the display order and only has the @a allowed flags.
 *
 * @param wnd Window
 * @param allowed_flags Bitmask of flags that the window is allowed to have
 *
 * @return Alternate window matching the criteria or @c NULL if there is none
 */
ds_window_t *ds_window_find_next(ds_window_t *wnd,
    display_wnd_flags_t allowed_flags)
{
	ds_window_t *nwnd;

	/* Try preceding windows in display order */
	nwnd = ds_display_prev_window(wnd);
	while (nwnd != NULL && (nwnd->flags & ~allowed_flags) != 0) {
		nwnd = ds_display_prev_window(nwnd);
	}

	/* Do we already have a matching window? */
	if (nwnd != NULL && (nwnd->flags & ~allowed_flags) == 0) {
		return nwnd;
	}

	/* Try succeeding windows in display order */
	nwnd = ds_display_last_window(wnd->display);
	while (nwnd != NULL && nwnd != wnd &&
	    (nwnd->flags & ~allowed_flags) != 0) {
		nwnd = ds_display_prev_window(nwnd);
	}

	if (nwnd == wnd)
		return NULL;

	return nwnd;
}

/** Remove focus from window.
 *
 * Used to switch focus to another window when closing or minimizing window.
 *
 * @param wnd Window
 */
void ds_window_unfocus(ds_window_t *wnd)
{
	ds_seat_t *seat;

	/* Make sure window is no longer focused in any seat */
	seat = ds_display_first_seat(wnd->display);
	while (seat != NULL) {
		ds_seat_unfocus_wnd(seat, wnd);
		seat = ds_display_next_seat(seat);
	}
}

/** Determine if input device belongs to the same seat as the original device.
 *
 * Compare the seat ownning @a idev_id with the seat owning @a wnd->orig_pos_id
 * (the device that started the window move or resize).
 *
 * This is used to make sure that, when two seats focus the same window,
 * only devices owned by the seat that started the resize or move can
 * affect it. Otherwise moving the other pointer(s) would disrupt the
 * resize or move operation.
 *
 * @param wnd Window (that is currently being resized or moved)
 * @param idev_id Input device ID
 * @return @c true iff idev_id is owned by the same seat as the input
 *         device that started the resize or move
 */
bool ds_window_orig_seat(ds_window_t *wnd, sysarg_t idev_id)
{
	ds_seat_t *orig_seat;
	ds_seat_t *seat;

	/* Window must be in state such that wnd->orig_pos_id is valid */
	assert(wnd->state == dsw_moving || wnd->state == dsw_resizing);

	orig_seat = ds_display_seat_by_idev(wnd->display, wnd->orig_pos_id);
	seat = ds_display_seat_by_idev(wnd->display, idev_id);

	return seat == orig_seat;
}

/** Window memory GC invalidate callback.
 *
 * This is called by the window's memory GC when a rectangle is modified.
 */
static void ds_window_invalidate_cb(void *arg, gfx_rect_t *rect)
{
	ds_window_t *wnd = (ds_window_t *)arg;
	gfx_rect_t drect;

	/* Repaint the corresponding part of the display */

	gfx_rect_translate(&wnd->dpos, rect, &drect);
	ds_display_lock(wnd->display);
	(void) ds_display_paint(wnd->display, &drect);
	ds_display_unlock(wnd->display);
}

/** Window memory GC update callback.
 *
 * This is called by the window's memory GC when it is to be updated.
 */
static void ds_window_update_cb(void *arg)
{
	ds_window_t *wnd = (ds_window_t *)arg;

	(void) wnd;
}

/** @}
 */
