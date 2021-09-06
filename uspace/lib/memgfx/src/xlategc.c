/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libmemgfx
 * @{
 */
/**
 * @file Translating graphics context
 *
 * A graphics context that just forwards all operations to another graphics
 * context with an offset.
 */

#include <assert.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/cursor.h>
#include <gfx/render.h>
#include <memgfx/xlategc.h>
#include <stdlib.h>
#include "../private/xlategc.h"

static errno_t xlate_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t xlate_gc_set_color(void *, gfx_color_t *);
static errno_t xlate_gc_fill_rect(void *, gfx_rect_t *);
static errno_t xlate_gc_update(void *);
static errno_t xlate_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t xlate_gc_bitmap_destroy(void *);
static errno_t xlate_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t xlate_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);
static errno_t xlate_gc_cursor_get_pos(void *, gfx_coord2_t *);
static errno_t xlate_gc_cursor_set_pos(void *, gfx_coord2_t *);
static errno_t xlate_gc_cursor_set_visible(void *, bool);

gfx_context_ops_t xlate_gc_ops = {
	.set_clip_rect = xlate_gc_set_clip_rect,
	.set_color = xlate_gc_set_color,
	.fill_rect = xlate_gc_fill_rect,
	.update = xlate_gc_update,
	.bitmap_create = xlate_gc_bitmap_create,
	.bitmap_destroy = xlate_gc_bitmap_destroy,
	.bitmap_render = xlate_gc_bitmap_render,
	.bitmap_get_alloc = xlate_gc_bitmap_get_alloc,
	.cursor_get_pos = xlate_gc_cursor_get_pos,
	.cursor_set_pos = xlate_gc_cursor_set_pos,
	.cursor_set_visible = xlate_gc_cursor_set_visible
};

/** Set clipping rectangle on translating GC.
 *
 * @param arg Translating GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;
	gfx_rect_t crect;

	if (rect != NULL) {
		gfx_rect_translate(&xgc->off, rect, &crect);
		return gfx_set_clip_rect(xgc->bgc, &crect);
	} else {
		return gfx_set_clip_rect(xgc->bgc, NULL);
	}
}

/** Set color on translating GC.
 *
 * Set drawing color on translating GC.
 *
 * @param arg Translating GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_set_color(void *arg, gfx_color_t *color)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;

	return gfx_set_color(xgc->bgc, color);
}

/** Fill rectangle on translating GC.
 *
 * @param arg Translating GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;
	gfx_rect_t frect;

	gfx_rect_translate(&xgc->off, rect, &frect);
	return gfx_fill_rect(xgc->bgc, &frect);
}

/** Update translating GC.
 *
 * @param arg Translating GC
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_update(void *arg)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;

	return gfx_update(xgc->bgc);
}

/** Create translating GC.
 *
 * Create graphics context that renders into another GC with offset.
 *
 * @param off Offset
 * @param bgc Backing GC
 * @param rgc Place to store pointer to new translating GC
 *
 * @return EOK on success or an error code
 */
errno_t xlate_gc_create(gfx_coord2_t *off, gfx_context_t *bgc,
    xlate_gc_t **rgc)
{
	xlate_gc_t *xgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	xgc = calloc(1, sizeof(xlate_gc_t));
	if (xgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&xlate_gc_ops, xgc, &gc);
	if (rc != EOK)
		goto error;

	xgc->bgc = bgc;
	xgc->gc = gc;
	xgc->off = *off;

	*rgc = xgc;
	return EOK;
error:
	if (xgc != NULL)
		free(xgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete translating GC.
 *
 * @param xgc Translating GC
 */
errno_t xlate_gc_delete(xlate_gc_t *xgc)
{
	errno_t rc;

	rc = gfx_context_delete(xgc->gc);
	if (rc != EOK)
		return rc;

	free(xgc);
	return EOK;
}

/** Get generic graphic context from translating GC.
 *
 * @param xgc Translating GC
 * @return Graphic context
 */
gfx_context_t *xlate_gc_get_ctx(xlate_gc_t *xgc)
{
	return xgc->gc;
}

/** Create bitmap in translating GC.
 *
 * @param arg Translating GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t xlate_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;
	xlate_gc_bitmap_t *xbm;
	gfx_bitmap_t *bm = NULL;
	errno_t rc;

	xbm = calloc(1, sizeof(xlate_gc_bitmap_t));
	if (xbm == NULL)
		return ENOMEM;

	rc = gfx_bitmap_create(xgc->bgc, params, alloc, &bm);
	if (rc != EOK) {
		free(xbm);
		return rc;
	}

	xbm->xgc = xgc;
	xbm->bm = bm;

	*rbm = (void *)xbm;
	return EOK;
}

/** Destroy bitmap in translating GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_bitmap_destroy(void *bm)
{
	xlate_gc_bitmap_t *xbm = (xlate_gc_bitmap_t *)bm;
	errno_t rc;

	rc = gfx_bitmap_destroy(xbm->bm);
	free(xbm);
	return rc;
}

/** Render bitmap in translating GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	xlate_gc_bitmap_t *xbm = (xlate_gc_bitmap_t *)bm;
	gfx_coord2_t offs;

	if (offs0 != NULL)
		gfx_coord2_add(offs0, &xbm->xgc->off, &offs);
	else
		offs = xbm->xgc->off;

	return gfx_bitmap_render(xbm->bm, srect0, &offs);
}

/** Get allocation info for bitmap in translating GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	xlate_gc_bitmap_t *xbm = (xlate_gc_bitmap_t *)bm;
	return gfx_bitmap_get_alloc(xbm->bm, alloc);
}

/** Get cursor position on translating GC.
 *
 * @param arg Translating GC
 * @param pos Place to store position
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_cursor_get_pos(void *arg, gfx_coord2_t *pos)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;
	gfx_coord2_t cpos;
	errno_t rc;

	rc = gfx_cursor_get_pos(xgc->bgc, &cpos);
	if (rc != EOK)
		return rc;

	gfx_coord2_subtract(&cpos, &xgc->off, pos);
	return EOK;
}

/** Set cursor position on translating GC.
 *
 * @param arg Translating GC
 * @param pos New position
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_cursor_set_pos(void *arg, gfx_coord2_t *pos)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;
	gfx_coord2_t cpos;

	gfx_coord2_add(pos, &xgc->off, &cpos);
	return gfx_cursor_set_pos(xgc->bgc, &cpos);
}

/** Set cursor visibility on translating GC.
 *
 * @param arg Translating GC
 * @param visible @c true iff cursor should be made visible
 *
 * @return EOK on success or an error code
 */
static errno_t xlate_gc_cursor_set_visible(void *arg, bool visible)
{
	xlate_gc_t *xgc = (xlate_gc_t *) arg;

	return gfx_cursor_set_visible(xgc->bgc, visible);
}

/** Set translation offset on translating GC.
 *
 * @param xgc Translating GC
 * @param off Offset
 */
void xlate_gc_set_off(xlate_gc_t *xgc, gfx_coord2_t *off)
{
	xgc->off = *off;
}

/** @}
 */
