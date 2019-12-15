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

	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_set_color gc=%p",
	    ds_display_get_gc(wnd->display));
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

	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_fill_rect");
	gfx_rect_clip(rect, &wnd->rect, &crect);
	gfx_rect_translate(&wnd->dpos, &crect, &drect);
	return gfx_fill_rect(ds_display_get_gc(wnd->display), &drect);
}

/** Create bitmap in canvas GC.
 *
 * @param arg Canvas GC
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

/** Destroy bitmap in canvas GC.
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

/** Render bitmap in canvas GC.
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

	return gfx_bitmap_render(cbm->bitmap, srect0, &doffs);
}

/** Get allocation info for bitmap in canvas GC.
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
 * @param client Client owning the window=
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

	wnd->rect = params->rect;
	wnd->gc = gc;
	*rgc = wnd;
	return EOK;
error:
	if (wnd != NULL)
		free(wnd);
	gfx_context_delete(gc);
	return rc;
}

/** Delete window GC.
 *
 * @param wnd Window GC
 */
void ds_window_destroy(ds_window_t *wnd)
{
	ds_client_remove_window(wnd);
	ds_display_remove_window(wnd);
	(void) gfx_context_delete(wnd->gc);

	free(wnd);
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

/** @}
 */
