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
 * @file GFX window backend
 *
 * This implements a graphics context over display server window.
 */

#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/log.h>
#include <io/pixelmap.h>
#include <macros.h>
#include <stdlib.h>
#include "client.h"
#include "display.h"
#include "window.h"

static errno_t ds_window_set_color(void *, gfx_color_t *);
static errno_t ds_window_fill_rect(void *, gfx_rect_t *);
static errno_t ds_window_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t ds_window_bitmap_destroy(void *);
static errno_t ds_window_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t ds_window_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

gfx_context_ops_t ds_window_ops = {
	.set_color = ds_window_set_color,
	.fill_rect = ds_window_fill_rect,
	.bitmap_create = ds_window_bitmap_create,
	.bitmap_destroy = ds_window_bitmap_destroy,
	.bitmap_render = ds_window_bitmap_render,
	.bitmap_get_alloc = ds_window_bitmap_get_alloc
};

/** Set color on window GC.
 *
 * Set drawing color on window GC.
 *
 * @param arg Console GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t ds_window_set_color(void *arg, gfx_color_t *color)
{
	ds_window_t *wnd = (ds_window_t *) arg;
	uint16_t r, g, b;

	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_set_color gc=%p",
	    ds_display_get_gc(wnd->display));

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	wnd->color = PIXEL(0, r >> 8, g >> 8, b >> 8);

	return gfx_set_color(ds_display_get_gc(wnd->display), color);
}

/** Fill rectangle on window GC.
 *
 * @param arg Window GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ds_window_fill_rect(void *arg, gfx_rect_t *rect)
{
	ds_window_t *wnd = (ds_window_t *) arg;
	gfx_rect_t crect;
	gfx_rect_t drect;
	gfx_coord_t x, y;

	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_fill_rect");

	gfx_rect_clip(rect, &wnd->rect, &crect);
	gfx_rect_translate(&wnd->dpos, &crect, &drect);

	/* Render a copy to the backbuffer */
	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			pixelmap_put_pixel(&wnd->pixelmap, x - wnd->rect.p0.x,
			    y - wnd->rect.p0.y, wnd->color);
		}
	}

	return gfx_fill_rect(ds_display_get_gc(wnd->display), &drect);
}

/** Create bitmap in window GC.
 *
 * @param arg Window GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t ds_window_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	ds_window_t *wnd = (ds_window_t *) arg;
	ds_window_bitmap_t *cbm = NULL;
	errno_t rc;

	cbm = calloc(1, sizeof(ds_window_bitmap_t));
	if (cbm == NULL)
		return ENOMEM;

	rc = gfx_bitmap_create(ds_display_get_gc(wnd->display), params, alloc,
	    &cbm->bitmap);
	if (rc != EOK)
		goto error;

	cbm->wnd = wnd;
	cbm->rect = params->rect;
	*rbm = (void *)cbm;
	return EOK;
error:
	if (cbm != NULL)
		free(cbm);
	return rc;
}

/** Destroy bitmap in window GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t ds_window_bitmap_destroy(void *bm)
{
	ds_window_bitmap_t *cbm = (ds_window_bitmap_t *)bm;

	gfx_bitmap_destroy(cbm->bitmap);
	free(cbm);
	return EOK;
}

/** Render bitmap in window GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t ds_window_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	ds_window_bitmap_t *cbm = (ds_window_bitmap_t *)bm;
	gfx_coord2_t doffs;
	gfx_coord2_t offs;
	gfx_rect_t srect;
	gfx_rect_t swrect;
	gfx_rect_t crect;
	gfx_coord_t x, y;
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	pixel_t pixel;
	errno_t rc;

	if (srect0 != NULL) {
		/* Clip source rectangle to bitmap rectangle */
		gfx_rect_clip(srect0, &cbm->rect, &srect);
	} else {
		/* Source is entire bitmap rectangle */
		srect = cbm->rect;
	}

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Transform window rectangle back to bitmap coordinate system */
	gfx_rect_rtranslate(&offs, &cbm->wnd->rect, &swrect);

	/* Clip so that transformed rectangle will be inside the window */
	gfx_rect_clip(&srect, &swrect, &crect);

	/* Offset for rendering on screen = window pos + offs */
	gfx_coord2_add(&cbm->wnd->dpos, &offs, &doffs);

	rc = gfx_bitmap_get_alloc(cbm->bitmap, &alloc);
	if (rc != EOK)
		return rc;

	pixelmap.width = cbm->rect.p1.x - cbm->rect.p0.x;
	pixelmap.height = cbm->rect.p1.y - cbm->rect.p0.y;
	pixelmap.data = alloc.pixels;

	/* Render a copy to the backbuffer */
	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			pixel = pixelmap_get_pixel(&pixelmap,
			    x - cbm->rect.p0.x, y - cbm->rect.p0.y);
			pixelmap_put_pixel(&cbm->wnd->pixelmap,
			    x + offs.x - cbm->rect.p0.x + cbm->wnd->rect.p0.x,
			    y + offs.y - cbm->rect.p0.y + cbm->wnd->rect.p0.y,
			    pixel);
		}
	}

	return gfx_bitmap_render(cbm->bitmap, &crect, &doffs);
}

/** Get allocation info for bitmap in window GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t ds_window_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	ds_window_bitmap_t *cbm = (ds_window_bitmap_t *)bm;

	return gfx_bitmap_get_alloc(cbm->bitmap, alloc);
}

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
	gfx_context_t *gc = NULL;
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

	rc = gfx_context_new(&ds_window_ops, wnd, &gc);
	if (rc != EOK)
		goto error;

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
	}

	wnd->rect = params->rect;
	wnd->min_size = params->min_size;
	wnd->gc = gc;
	*rgc = wnd;
	return EOK;
error:
	if (wnd != NULL) {
		if (wnd->bitmap != NULL)
			gfx_bitmap_destroy(wnd->bitmap);
		free(wnd);
	}

	gfx_context_delete(gc);
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

	(void) gfx_context_delete(wnd->gc);
	if (wnd->bitmap != NULL)
		gfx_bitmap_destroy(wnd->bitmap);

	free(wnd);

	(void) ds_display_paint(disp, NULL);
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
	}

	gfx_coord2_add(&wnd->dpos, offs, &ndpos);

	wnd->dpos = ndpos;
	wnd->rect = *nrect;

	(void) ds_display_paint(wnd->display, NULL);
	return EOK;
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
 * @param event Button press event
 */
static void ds_window_start_move(ds_window_t *wnd, pos_event_t *event)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_start_move (%d, %d)",
	    (int) event->hpos, (int) event->vpos);

	if (wnd->state != dsw_idle)
		return;

	wnd->orig_pos.x = event->hpos;
	wnd->orig_pos.y = event->vpos;
	wnd->state = dsw_moving;
}

/** Finish moving a window by mouse drag.
 *
 * @param wnd Window
 * @param event Button release event
 */
static void ds_window_finish_move(ds_window_t *wnd, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_coord2_t dmove;
	gfx_coord2_t nwpos;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_finish_move (%d, %d)",
	    (int) event->hpos, (int) event->vpos);

	if (wnd->state != dsw_moving)
		return;

	pos.x = event->hpos;
	pos.y = event->vpos;
	gfx_coord2_subtract(&pos, &wnd->orig_pos, &dmove);

	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);
	wnd->dpos = nwpos;
	wnd->state = dsw_idle;

	(void) ds_display_paint(wnd->display, NULL);
}

/** Update window position when moving by mouse drag.
 *
 * @param wnd Window
 * @param event Position update event
 */
static void ds_window_update_move(ds_window_t *wnd, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_coord2_t dmove;
	gfx_coord2_t nwpos;
	gfx_rect_t drect;
	gfx_color_t *color;
	gfx_context_t *gc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_update_move (%d, %d)",
	    (int) event->hpos, (int) event->vpos);

	if (wnd->state != dsw_moving)
		return;

	gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);

	gc = ds_display_get_gc(wnd->display); // XXX
	if (gc != NULL) {
		gfx_set_color(gc, wnd->display->bg_color);
		gfx_fill_rect(gc, &drect);
	}

	pos.x = event->hpos;
	pos.y = event->vpos;
	gfx_coord2_subtract(&pos, &wnd->orig_pos, &dmove);

	gfx_coord2_add(&wnd->dpos, &dmove, &nwpos);
	gfx_rect_translate(&nwpos, &wnd->rect, &drect);

	wnd->orig_pos = pos;
	wnd->dpos = nwpos;

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

/** Finish resizing a window by mouse drag.
 *
 * @param wnd Window
 * @param event Button release event
 */
static void ds_window_finish_resize(ds_window_t *wnd, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_coord2_t dresize;
	gfx_rect_t nrect;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_finish_resize (%d, %d)",
	    (int) event->hpos, (int) event->vpos);

	if (wnd->state != dsw_resizing)
		return;

	(void) ds_display_paint(wnd->display, NULL);

	pos.x = event->hpos;
	pos.y = event->vpos;
	gfx_coord2_subtract(&pos, &wnd->orig_pos, &dresize);

	/* Compute new rectangle */
	ds_window_calc_resize(wnd, &dresize, &nrect);

	wnd->state = dsw_idle;
	ds_client_post_resize_event(wnd->client, wnd, &nrect);
}

/** Update window position when resizing by mouse drag.
 *
 * @param wnd Window
 * @param event Position update event
 */
static void ds_window_update_resize(ds_window_t *wnd, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_coord2_t dresize;
	gfx_rect_t nrect;
	gfx_rect_t drect;
	gfx_color_t *color;
	gfx_context_t *gc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_update_resize (%d, %d)",
	    (int) event->hpos, (int) event->vpos);

	if (wnd->state != dsw_resizing)
		return;

	gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);

	gc = ds_display_get_gc(wnd->display); // XXX
	if (gc != NULL) {
		gfx_set_color(gc, wnd->display->bg_color);
		gfx_fill_rect(gc, &drect);
	}

	pos.x = event->hpos;
	pos.y = event->vpos;
	gfx_coord2_subtract(&pos, &wnd->orig_pos, &dresize);

	ds_window_calc_resize(wnd, &dresize, &nrect);
	gfx_rect_translate(&wnd->dpos, &nrect, &drect);

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

	if (event->type == POS_PRESS && event->btn_num == 2 && inside)
		ds_window_start_move(wnd, event);

	if (event->type == POS_RELEASE) {
		ds_window_finish_move(wnd, event);
		ds_window_finish_resize(wnd, event);
	}

	if (event->type == POS_UPDATE) {
		ds_window_update_move(wnd, event);
		ds_window_update_resize(wnd, event);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ds_window_move_req (%d, %d)",
	    (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	gfx_coord2_add(&wnd->dpos, pos, &wnd->orig_pos);
	wnd->state = dsw_moving;
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
	log_msg(LOG_DEFAULT, LVL_NOTE, "ds_window_resize_req (%d, %d, %d)",
	    (int) rsztype, (int) pos->x, (int) pos->y);

	if (wnd->state != dsw_idle)
		return;

	gfx_coord2_add(&wnd->dpos, pos, &wnd->orig_pos);
	wnd->state = dsw_resizing;
	wnd->rsztype = rsztype;
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

/** @}
 */
