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

/** @addtogroup libcongfx
 * @{
 */
/**
 * @file GFX console backend
 *
 * This implements a graphics context over a classic console interface.
 * This is just for experimentation purposes. In the end we want the
 * console to actually directly suport GFX interface.
 */

#include <congfx/console.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <gfx/render.h>
#include <io/pixel.h>
#include <io/pixelmap.h>
#include <stdlib.h>
#include "../private/console.h"
#include "../private/color.h"

static errno_t console_gc_set_color(void *, gfx_color_t *);
static errno_t console_gc_fill_rect(void *, gfx_rect_t *);
static errno_t console_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t console_gc_bitmap_destroy(void *);
static errno_t console_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t console_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

gfx_context_ops_t console_gc_ops = {
	.set_color = console_gc_set_color,
	.fill_rect = console_gc_fill_rect,
	.bitmap_create = console_gc_bitmap_create,
	.bitmap_destroy = console_gc_bitmap_destroy,
	.bitmap_render = console_gc_bitmap_render,
	.bitmap_get_alloc = console_gc_bitmap_get_alloc
};

/** Set color on console GC.
 *
 * Set drawing color on console GC.
 *
 * @param arg Console GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t console_gc_set_color(void *arg, gfx_color_t *color)
{
	console_gc_t *cgc = (console_gc_t *) arg;

	cgc->clr = PIXEL(0, color->r >> 8, color->g >> 8, color->b >> 8);
	return EOK;
}

/** Fill rectangle on console GC.
 *
 * @param arg Console GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t console_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	console_gc_t *cgc = (console_gc_t *) arg;
	int rv;
	gfx_coord_t x, y;

	// XXX We should handle p0.x > p1.x and p0.y > p1.y

	console_set_rgb_color(cgc->con, cgc->clr, cgc->clr);

	for (y = rect->p0.y; y < rect->p1.y; y++) {
		console_set_pos(cgc->con, rect->p0.x, y);

		for (x = rect->p0.x; x < rect->p1.x; x++) {
			rv = fputc('X', cgc->fout);
			if (rv < 0)
				return EIO;
		}

		console_flush(cgc->con);
	}

	return EOK;
}

/** Create console GC.
 *
 * Create graphics context for rendering into a console.
 *
 * @param con Console object
 * @param fout File to which characters are written (console)
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t console_gc_create(console_ctrl_t *con, FILE *fout,
    console_gc_t **rgc)
{
	console_gc_t *cgc = NULL;
	gfx_context_t *gc = NULL;
	sysarg_t rows;
	sysarg_t cols;
	errno_t rc;

	cgc = calloc(1, sizeof(console_gc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = console_get_size(con, &cols, &rows);
	if (rc != EOK)
		goto error;

	rc = gfx_context_new(&console_gc_ops, cgc, &gc);
	if (rc != EOK)
		goto error;

	cgc->gc = gc;
	cgc->con = con;
	cgc->fout = fout;
	cgc->rect.p0.x = 0;
	cgc->rect.p0.y = 0;
	cgc->rect.p1.x = cols;
	cgc->rect.p1.y = rows - 1; /* make sure we avoid bottom-right corner */

	*rgc = cgc;
	return EOK;
error:
	if (cgc != NULL)
		free(cgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete console GC.
 *
 * @param cgc Console GC
 */
errno_t console_gc_delete(console_gc_t *cgc)
{
	errno_t rc;

	rc = gfx_context_delete(cgc->gc);
	if (rc != EOK)
		return rc;

	free(cgc);
	return EOK;
}

/** Get generic graphic context from console GC.
 *
 * @param cgc Console GC
 * @return Graphic context
 */
gfx_context_t *console_gc_get_ctx(console_gc_t *cgc)
{
	return cgc->gc;
}

/** Create bitmap in console GC.
 *
 * @param arg console GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t console_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	console_gc_t *cgc = (console_gc_t *) arg;
	console_gc_bitmap_t *cbm = NULL;
	gfx_coord2_t dim;
	errno_t rc;

	/* Check that we support all requested flags */
	if ((params->flags & ~bmpf_color_key) != 0)
		return ENOTSUP;

	cbm = calloc(1, sizeof(console_gc_bitmap_t));
	if (cbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	cbm->rect = params->rect;
	cbm->flags = params->flags;
	cbm->key_color = params->key_color;

	if (alloc == NULL) {
		cbm->alloc.pitch = dim.x * sizeof(uint32_t);
		cbm->alloc.off0 = 0;
		cbm->alloc.pixels = calloc(dim.x * dim.y, sizeof(uint32_t));
		if (cbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}

		cbm->myalloc = true;
	} else {
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

/** Destroy bitmap in console GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t console_gc_bitmap_destroy(void *bm)
{
	console_gc_bitmap_t *cbm = (console_gc_bitmap_t *)bm;
	if (cbm->myalloc)
		free(cbm->alloc.pixels);
	free(cbm);
	return EOK;
}

/** Render bitmap in console GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t console_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	console_gc_bitmap_t *cbm = (console_gc_bitmap_t *)bm;
	gfx_coord_t x, y;
	int rv;
	pixel_t clr;
	pixelmap_t pixelmap;
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_rect_t crect;
	gfx_coord2_t offs;

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

	gfx_rect_translate(&offs, &srect, &drect);
	gfx_rect_clip(&drect, &cbm->cgc->rect, &crect);

	pixelmap.width = cbm->rect.p1.x - cbm->rect.p0.x;
	pixelmap.height = cbm->rect.p1.y = cbm->rect.p1.y;
	pixelmap.data = cbm->alloc.pixels;

	if ((cbm->flags & bmpf_color_key) == 0) {
		for (y = crect.p0.y; y < crect.p1.y; y++) {
			console_set_pos(cbm->cgc->con, crect.p0.x, y);

			for (x = crect.p0.x; x < crect.p1.x; x++) {
				clr = pixelmap_get_pixel(&pixelmap,
				    x - offs.x - cbm->rect.p0.x,
				    y - offs.y - cbm->rect.p0.y);
				console_set_rgb_color(cbm->cgc->con, clr, clr);

				rv = fputc('X', cbm->cgc->fout);
				if (rv < 0)
					return EIO;

				console_flush(cbm->cgc->con);
			}
		}
	} else {
		for (y = crect.p0.y; y < crect.p1.y; y++) {
			for (x = crect.p0.x; x < crect.p1.x; x++) {

				clr = pixelmap_get_pixel(&pixelmap,
				    x - offs.x - cbm->rect.p0.x,
				    y - offs.y - cbm->rect.p0.y);
				console_set_rgb_color(cbm->cgc->con, clr, clr);

				if (clr != cbm->key_color) {
					console_set_pos(cbm->cgc->con, x, y);
					rv = fputc('X', cbm->cgc->fout);
					if (rv < 0)
						return EIO;

					console_flush(cbm->cgc->con);
				}

			}
		}
	}

	return EOK;
}

/** Get allocation info for bitmap in console GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t console_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	console_gc_bitmap_t *cbm = (console_gc_bitmap_t *)bm;
	*alloc = cbm->alloc;
	return EOK;
}

/** @}
 */
