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
#include "client.h"
#include "display.h"
#include "seat.h"
#include "window.h"

static void ds_window_update_cb(void *, gfx_rect_t *);

/** Create window.
 *
 * Create graphics context for rendering into a window.
 *
 * @param client Client owning the window
 * @param params Window parameters
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t ds_window_create(ds_client_t *client, display_wnd_params_t *params,
    ds_window_t **rgc)
{
	ds_window_t *wnd = NULL;
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

	ds_client_add_window(client, wnd);
	ds_display_add_window(client->display, wnd);

	gfx_bitmap_params_init(&bparams);
	bparams.rect = params->rect;

	dgc = ds_display_get_gc(wnd->display); // XXX
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

	rc = mem_gc_create(&params->rect, &alloc, ds_window_update_cb,
	    (void *)wnd, &wnd->mgc);
	if (rc != EOK)
		goto error;

	wnd->rect = params->rect;
	wnd->min_size = params->min_size;
	wnd->gc = mem_gc_get_ctx(wnd->mgc);
	wnd->cursor = wnd->display->cursor[dcurs_arrow];
	*rgc = wnd;
	return EOK;
error:
	if (wnd != NULL) {
		if (wnd->bitmap != NULL)
			gfx_bitmap_destroy(wnd->bitmap);
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

	ds_client_remove_window(wnd);
	ds_display_remove_window(wnd);

	mem_gc_delete(wnd->mgc);

	if (wnd->bitmap != NULL)
		gfx_bitmap_destroy(wnd->bitmap);

	free(wnd);

	(void) ds_display_paint(disp, NULL);
}

/** Bring window to top.
 *
 * @param wnd Window
 */
void ds_window_bring_to_top(ds_window_t *wnd)
{
	ds_display_t *disp = wnd->display;

	ds_display_remove_window(wnd);
	ds_display_add_window(disp, wnd);
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_paint");

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

/** Start moving a window by mouse drag.
 *
 * @param wnd Window
 * @param pos Position where mouse button was pressed
 */
static void ds_window_start_move(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_color_t *color;
	gfx_context_t *gc;
	gfx_rect_t drect;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_start_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	wnd->orig_pos = *pos;
	wnd->state = dsw_moving;
	wnd->preview_pos = wnd->dpos;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		return;

	gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);

	gc = ds_display_get_gc(wnd->display); // XXX
	if (gc != NULL) {
		gfx_set_color(gc, color);
		gfx_fill_rect(gc, &drect);
	}

	gfx_color_delete(color);
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_finish_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_moving);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dmove);

	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);
	wnd->dpos = nwpos;
	wnd->state = dsw_idle;

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
	gfx_rect_t drect;
	gfx_color_t *color;
	gfx_context_t *gc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_update_move (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_moving);

	gfx_rect_translate(&wnd->preview_pos, &wnd->rect, &drect);
	ds_display_paint(wnd->display, &drect);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dmove);

	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);
	wnd->preview_pos = nwpos;

	gfx_rect_translate(&nwpos, &wnd->rect, &drect);

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		return;

	gc = ds_display_get_gc(wnd->display); // XXX
	if (gc != NULL) {
		gfx_set_color(gc, color);
		gfx_fill_rect(gc, &drect);
	}

	gfx_color_delete(color);
}

/** Start resizing a window by mouse drag.
 *
 * @param wnd Window
 * @param rsztype Resize type (which part of window is being dragged)
 * @param pos Position where mouse button was pressed
 */
static void ds_window_start_resize(ds_window_t *wnd,
    display_wnd_rsztype_t rsztype, gfx_coord2_t *pos)
{
	ds_seat_t *seat;
	display_stock_cursor_t ctype;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_start_resize (%d, %d)",
	    (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	wnd->orig_pos = *pos;
	wnd->state = dsw_resizing;
	wnd->rsztype = rsztype;
	wnd->preview_rect = wnd->rect;

	// XXX Need client to tell us which seat started the resize!
	seat = ds_display_first_seat(wnd->display);
	ctype = display_cursor_from_wrsz(rsztype);
	ds_seat_set_wm_cursor(seat, wnd->display->cursor[ctype]);
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

	(void) ds_display_paint(wnd->display, NULL);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dresize);

	/* Compute new rectangle */
	ds_window_calc_resize(wnd, &dresize, &nrect);

	wnd->state = dsw_idle;
	ds_client_post_resize_event(wnd->client, wnd, &nrect);

	// XXX Need to know which seat started the resize!
	seat = ds_display_first_seat(wnd->display);
	ds_seat_set_wm_cursor(seat, NULL);
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
	gfx_rect_t drect;
	gfx_color_t *color;
	gfx_context_t *gc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_update_resize (%d, %d)",
	    (int) pos->x, (int) pos->y);

	assert(wnd->state == dsw_resizing);

	gfx_rect_translate(&wnd->dpos, &wnd->preview_rect, &drect);
	(void) ds_display_paint(wnd->display, &drect);

	gfx_coord2_subtract(pos, &wnd->orig_pos, &dresize);

	ds_window_calc_resize(wnd, &dresize, &nrect);
	gfx_rect_translate(&wnd->dpos, &nrect, &drect);
	wnd->preview_rect = nrect;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		return;

	gc = ds_display_get_gc(wnd->display); // XXX
	if (gc != NULL) {
		gfx_set_color(gc, color);
		gfx_fill_rect(gc, &drect);
	}

	gfx_color_delete(color);
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
 */
errno_t ds_window_post_pos_event(ds_window_t *wnd, pos_event_t *event)
{
	pos_event_t tevent;
	gfx_coord2_t pos;
	gfx_rect_t drect;
	bool inside;

	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "ds_window_post_pos_event type=%d pos=%d,%d\n", event->type,
	    (int) event->hpos, (int) event->vpos);

	pos.x = event->hpos;
	pos.y = event->vpos;
	gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);
	inside = gfx_pix_inside_rect(&pos, &drect);

	if (event->type == POS_PRESS && event->btn_num == 2 && inside) {
		ds_window_start_move(wnd, &pos);
		return EOK;
	}

	if (event->type == POS_RELEASE) {
		if (wnd->state == dsw_moving) {
			ds_window_finish_move(wnd, &pos);
			return EOK;
		}

		if (wnd->state == dsw_resizing) {
			ds_window_finish_resize(wnd, &pos);
			return EOK;
		}
	}

	if (event->type == POS_UPDATE) {
		if (wnd->state == dsw_moving) {
			ds_window_update_move(wnd, &pos);
			return EOK;
		}

		if (wnd->state == dsw_resizing) {
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
 */
errno_t ds_window_post_focus_event(ds_window_t *wnd)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_post_focus_event\n");

	return ds_client_post_focus_event(wnd->client, wnd);
}

/** Post unfocus event to window.
 *
 * @param wnd Window
 */
errno_t ds_window_post_unfocus_event(ds_window_t *wnd)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_post_unfocus_event\n");

	return ds_client_post_unfocus_event(wnd->client, wnd);
}

/** Start moving a window, detected by client.
 *
 * @param wnd Window
 * @param pos Position where the pointer was when the move started
 *            relative to the window
 * @param event Button press event
 */
void ds_window_move_req(ds_window_t *wnd, gfx_coord2_t *pos)
{
	gfx_coord2_t orig_pos;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_move_req (%d, %d)",
	    (int) pos->x, (int) pos->y);

	gfx_coord2_add(&wnd->dpos, pos, &orig_pos);
	ds_window_start_move(wnd, &orig_pos);
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

/** Start resizing a window, detected by client.
 *
 * @param wnd Window
 * @param rsztype Resize type (which part of window is being dragged)
 * @param pos Position where the pointer was when the resize started
 *            relative to the window
 * @param event Button press event
 */
void ds_window_resize_req(ds_window_t *wnd, display_wnd_rsztype_t rsztype,
    gfx_coord2_t *pos)
{
	gfx_coord2_t orig_pos;

	log_msg(LOG_DEFAULT, LVL_NOTE, "ds_window_resize_req (%d, %d, %d)",
	    (int) rsztype, (int) pos->x, (int) pos->y);

	gfx_coord2_add(&wnd->dpos, pos, &orig_pos);
	ds_window_start_resize(wnd, rsztype, &orig_pos);
}

/** Resize window.
 *
 * @param wnd Window
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

	dgc = ds_display_get_gc(wnd->display); // XXX
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

	(void) ds_display_paint(wnd->display, NULL);
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

/** Window memory GC update callbac.
 *
 * This is called by the window's memory GC when a rectangle us updated.
 */
static void ds_window_update_cb(void *arg, gfx_rect_t *rect)
{
	ds_window_t *wnd = (ds_window_t *)arg;
	gfx_rect_t drect;

	/* Repaint the corresponding part of the display */
	gfx_rect_translate(&wnd->dpos, rect, &drect);
	(void) ds_display_paint(wnd->display, &drect);
}

/** @}
 */
