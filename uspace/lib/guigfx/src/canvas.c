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

/** @addtogroup libguigfx
 * @{
 */
/**
 * @file GFX canvas backend
 *
 * This implements a graphics context over a libgui canvas.
 * This is just for experimentation purposes and its kind of backwards.
 */

#include <draw/drawctx.h>
#include <draw/source.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <guigfx/canvas.h>
#include <io/pixel.h>
#include <stdlib.h>
#include <transform.h>
#include "../private/canvas.h"
//#include "../../private/color.h"

static errno_t canvas_gc_set_color(void *, gfx_color_t *);
static errno_t canvas_gc_fill_rect(void *, gfx_rect_t *);
static errno_t canvas_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t canvas_gc_bitmap_destroy(void *);
static errno_t canvas_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t canvas_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

gfx_context_ops_t canvas_gc_ops = {
	.set_color = canvas_gc_set_color,
	.fill_rect = canvas_gc_fill_rect,
	.bitmap_create = canvas_gc_bitmap_create,
	.bitmap_destroy = canvas_gc_bitmap_destroy,
	.bitmap_render = canvas_gc_bitmap_render,
	.bitmap_get_alloc = canvas_gc_bitmap_get_alloc
};

/** Set color on canvas GC.
 *
 * Set drawing color on canvas GC.
 *
 * @param arg Canvas GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t canvas_gc_set_color(void *arg, gfx_color_t *color)
{
	canvas_gc_t *cgc = (canvas_gc_t *) arg;

	return gfx_set_color(cgc->mbgc, color);
}

/** Fill rectangle on canvas GC.
 *
 * @param arg Canvas GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t canvas_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	canvas_gc_t *cgc = (canvas_gc_t *) arg;
	errno_t rc;

	rc = gfx_fill_rect(cgc->mbgc, rect);
	if (rc == EOK)
		update_canvas(cgc->canvas, cgc->surface);

	return rc;
}

/** Create canvas GC.
 *
 * Create graphics context for rendering into a canvas.
 *
 * @param canvas Canvas object
 * @param fout File to which characters are written (canvas)
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t canvas_gc_create(canvas_t *canvas, surface_t *surface,
    canvas_gc_t **rgc)
{
	canvas_gc_t *cgc = NULL;
	gfx_context_t *gc = NULL;
	surface_coord_t w, h;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	surface_get_resolution(surface, &w, &h);

	cgc = calloc(1, sizeof(canvas_gc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&canvas_gc_ops, cgc, &gc);
	if (rc != EOK)
		goto error;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = w;
	rect.p1.y = h;

	alloc.pitch = w * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = surface_direct_access(surface);

	rc = mem_gc_create(&rect, &alloc, &cgc->mgc);
	if (rc != EOK)
		goto error;

	cgc->mbgc = mem_gc_get_ctx(cgc->mgc);

	cgc->gc = gc;
	cgc->canvas = canvas;
	cgc->surface = surface;
	*rgc = cgc;
	return EOK;
error:
	if (gc != NULL)
		gfx_context_delete(gc);
	if (cgc != NULL)
		free(cgc);
	return rc;
}

/** Delete canvas GC.
 *
 * @param cgc Canvas GC
 */
errno_t canvas_gc_delete(canvas_gc_t *cgc)
{
	errno_t rc;

	mem_gc_delete(cgc->mgc);

	rc = gfx_context_delete(cgc->gc);
	if (rc != EOK)
		return rc;

	free(cgc);
	return EOK;
}

/** Get generic graphic context from canvas GC.
 *
 * @param cgc Canvas GC
 * @return Graphic context
 */
gfx_context_t *canvas_gc_get_ctx(canvas_gc_t *cgc)
{
	return cgc->gc;
}

/** Create bitmap in canvas GC.
 *
 * @param arg Canvas GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t canvas_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	canvas_gc_t *cgc = (canvas_gc_t *) arg;
	canvas_gc_bitmap_t *cbm = NULL;
	errno_t rc;

	cbm = calloc(1, sizeof(canvas_gc_bitmap_t));
	if (cbm == NULL)
		return ENOMEM;

	rc = gfx_bitmap_create(cgc->mbgc, params, alloc, &cbm->mbitmap);
	if (rc != EOK)
		goto error;

	cbm->cgc = cgc;
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
static errno_t canvas_gc_bitmap_destroy(void *bm)
{
	canvas_gc_bitmap_t *cbm = (canvas_gc_bitmap_t *)bm;
	errno_t rc;

	rc = gfx_bitmap_destroy(cbm->mbitmap);
	if (rc != EOK)
		return rc;

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
static errno_t canvas_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	canvas_gc_bitmap_t *cbm = (canvas_gc_bitmap_t *)bm;
	errno_t rc;

	rc = gfx_bitmap_render(cbm->mbitmap, srect0, offs0);
	if (rc == EOK)
		update_canvas(cbm->cgc->canvas, cbm->cgc->surface);

	return rc;
}

/** Get allocation info for bitmap in canvas GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t canvas_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	canvas_gc_bitmap_t *cbm = (canvas_gc_bitmap_t *)bm;

	return gfx_bitmap_get_alloc(cbm->mbitmap, alloc);
}

/** @}
 */
