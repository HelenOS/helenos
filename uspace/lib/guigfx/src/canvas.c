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
	uint16_t r, g, b;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	cgc->color = PIXEL(0, r >> 8, g >> 8, b >> 8);
	return EOK;
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
	gfx_coord_t x, y;

	// XXX We should handle p0.x > p1.x and p0.y > p1.y

	for (y = rect->p0.y; y < rect->p1.y; y++) {
		for (x = rect->p0.x; x < rect->p1.x; x++) {
			surface_put_pixel(cgc->surface, x, y, cgc->color);
		}
	}

	update_canvas(cgc->canvas, cgc->surface);

	return EOK;
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
	errno_t rc;

	cgc = calloc(1, sizeof(canvas_gc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&canvas_gc_ops, cgc, &gc);
	if (rc != EOK)
		goto error;

	cgc->gc = gc;
	cgc->canvas = canvas;
	cgc->surface = surface;
	*rgc = cgc;
	return EOK;
error:
	if (cgc != NULL)
		free(cgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete canvas GC.
 *
 * @param cgc Canvas GC
 */
errno_t canvas_gc_delete(canvas_gc_t *cgc)
{
	errno_t rc;

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
	gfx_coord2_t dim;
	errno_t rc;

	cbm = calloc(1, sizeof(canvas_gc_bitmap_t));
	if (cbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	cbm->rect = params->rect;
	cbm->flags = params->flags;
	cbm->key_color = params->key_color;

	if (alloc == NULL) {
		cbm->surface = surface_create(dim.x, dim.y, NULL, 0);
		if (cbm->surface == NULL) {
			rc = ENOMEM;
			goto error;
		}

		cbm->alloc.pitch = dim.x * sizeof(uint32_t);
		cbm->alloc.off0 = 0;
		cbm->alloc.pixels = surface_direct_access(cbm->surface);
		cbm->myalloc = true;
	} else {
		cbm->surface = surface_create(dim.x, dim.y, alloc->pixels, 0);
		if (cbm->surface == NULL) {
			rc = ENOMEM;
			goto error;
		}

		cbm->alloc = *alloc;
	}

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
	if (cbm->myalloc)
		surface_destroy(cbm->surface);
	// XXX if !cbm->myalloc, surface is leaked - no way to destroy it
	// without destroying the pixel buffer
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
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_coord2_t offs;
	gfx_coord2_t dim;
	gfx_coord_t x, y;
	pixel_t pixel;

	if (srect0 != NULL)
		srect = *srect0;
	else
		srect = cbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);

	gfx_coord2_subtract(&drect.p1, &drect.p0, &dim);

	transform_t transform;
	transform_identity(&transform);
	transform_translate(&transform, offs.x - cbm->rect.p0.x,
	    offs.y - cbm->rect.p0.y);

	source_t source;
	source_init(&source);
	source_set_transform(&source, transform);
	source_set_texture(&source, cbm->surface,
	    PIXELMAP_EXTEND_TRANSPARENT_BLACK);

	if ((cbm->flags & bmpf_color_key) == 0) {
		drawctx_t drawctx;
		drawctx_init(&drawctx, cbm->cgc->surface);

		drawctx_set_source(&drawctx, &source);
		drawctx_transfer(&drawctx, drect.p0.x, drect.p0.y, dim.x, dim.y);
	} else {
		for (y = drect.p0.y; y < drect.p1.y; y++) {
			for (x = drect.p0.x; x < drect.p1.x; x++) {
				pixel = source_determine_pixel(&source, x, y);
				if (pixel != cbm->key_color) {
					surface_put_pixel(cbm->cgc->surface,
					    x, y, pixel);
				}
			}
		}
	}

	update_canvas(cbm->cgc->canvas, cbm->cgc->surface);
	return EOK;
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
	*alloc = cbm->alloc;
	return EOK;
}

/** @}
 */
