/*
 * Copyright (c) 2020 Jiri Svoboda
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
 * @file GFX memory backend
 *
 * This implements a graphics context over a block of memory (i.e. a simple
 * software renderer).
 */

#include <assert.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/pixel.h>
#include <io/pixelmap.h>
#include <memgfx/memgc.h>
#include <stdlib.h>
#include "../private/memgc.h"

static errno_t mem_gc_set_color(void *, gfx_color_t *);
static errno_t mem_gc_fill_rect(void *, gfx_rect_t *);
static errno_t mem_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t mem_gc_bitmap_destroy(void *);
static errno_t mem_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t mem_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);
static void mem_gc_invalidate_rect(mem_gc_t *, gfx_rect_t *);

gfx_context_ops_t mem_gc_ops = {
	.set_color = mem_gc_set_color,
	.fill_rect = mem_gc_fill_rect,
	.bitmap_create = mem_gc_bitmap_create,
	.bitmap_destroy = mem_gc_bitmap_destroy,
	.bitmap_render = mem_gc_bitmap_render,
	.bitmap_get_alloc = mem_gc_bitmap_get_alloc
};

/** Set color on memory GC.
 *
 * Set drawing color on memory GC.
 *
 * @param arg Canvas GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_set_color(void *arg, gfx_color_t *color)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;
	uint16_t r, g, b;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	mgc->color = PIXEL(0, r >> 8, g >> 8, b >> 8);
	return EOK;
}

/** Fill rectangle on memory GC.
 *
 * @param arg Canvas GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;
	gfx_rect_t crect;
	gfx_coord2_t dims;
	gfx_coord_t x, y;
	pixelmap_t pixelmap;

	/* Make sure we have a sorted, clipped rectangle */
	gfx_rect_clip(rect, &mgc->rect, &crect);

	gfx_rect_dims(&mgc->rect, &dims);
	pixelmap.width = dims.x;
	pixelmap.height = dims.y;
	pixelmap.data = mgc->alloc.pixels;

	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			pixelmap_put_pixel(&pixelmap, x, y, mgc->color);
		}
	}

	mem_gc_invalidate_rect(mgc, &crect);
	return EOK;
}

/** Create memory GC.
 *
 * Create graphics context for rendering into a block of memory.
 *
 * @param rect Bounding rectangle
 * @param alloc Allocation info
 * @param rgc Place to store pointer to new memory GC
 *
 * @return EOK on success or an error code
 */
errno_t mem_gc_create(gfx_rect_t *rect, gfx_bitmap_alloc_t *alloc,
    mem_gc_t **rgc)
{
	mem_gc_t *mgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	mgc = calloc(1, sizeof(mem_gc_t));
	if (mgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&mem_gc_ops, mgc, &gc);
	if (rc != EOK)
		goto error;

	mgc->gc = gc;
	mgc->rect = *rect;
	mgc->alloc = *alloc;

	/*
	 * These are the limitations of pixelmap which we are using.
	 * Rather than falling back to an ad-hoc method of pixel access
	 * (which is not searchable), use pixelmap for now and switch
	 * to a better, more universal method later (e.g. supporting
	 * different color depths).
	 */
	assert(rect->p0.x == 0);
	assert(rect->p0.y == 0);
	assert(alloc->pitch == rect->p1.x * (int)sizeof(uint32_t));

	mem_gc_clear_update_rect(mgc);

	*rgc = mgc;
	return EOK;
error:
	if (mgc != NULL)
		free(mgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete memory GC.
 *
 * @param mgc Canvas GC
 */
errno_t mem_gc_delete(mem_gc_t *mgc)
{
	errno_t rc;

	rc = gfx_context_delete(mgc->gc);
	if (rc != EOK)
		return rc;

	free(mgc);
	return EOK;
}

/** Get generic graphic context from memory GC.
 *
 * @param mgc Canvas GC
 * @return Graphic context
 */
gfx_context_t *mem_gc_get_ctx(mem_gc_t *mgc)
{
	return mgc->gc;
}

static void mem_gc_invalidate_rect(mem_gc_t *mgc, gfx_rect_t *rect)
{
	gfx_rect_t nrect;

	gfx_rect_envelope(&mgc->upd_rect, rect, &nrect);
	mgc->upd_rect = nrect;
}

void mem_gc_get_update_rect(mem_gc_t *mgc, gfx_rect_t *rect)
{
	*rect = mgc->upd_rect;
}

void mem_gc_clear_update_rect(mem_gc_t *mgc)
{
	mgc->upd_rect.p0.x = 0;
	mgc->upd_rect.p0.y = 0;
	mgc->upd_rect.p1.x = 0;
	mgc->upd_rect.p1.y = 0;
}

/** Create bitmap in memory GC.
 *
 * @param arg Canvas GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t mem_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;
	mem_gc_bitmap_t *mbm = NULL;
	gfx_coord2_t dim;
	errno_t rc;

	mbm = calloc(1, sizeof(mem_gc_bitmap_t));
	if (mbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	mbm->rect = params->rect;
	mbm->flags = params->flags;
	mbm->key_color = params->key_color;

	if (alloc == NULL) {
		mbm->alloc.pitch = dim.x * sizeof(uint32_t);
		mbm->alloc.off0 = 0;
		mbm->alloc.pixels = malloc(mbm->alloc.pitch * dim.y);
		mbm->myalloc = true;

		if (mbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}
	} else {
		mbm->alloc = *alloc;
	}

	mbm->mgc = mgc;
	*rbm = (void *)mbm;
	return EOK;
error:
	if (mbm != NULL)
		free(mbm);
	return rc;
}

/** Destroy bitmap in memory GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t mem_gc_bitmap_destroy(void *bm)
{
	mem_gc_bitmap_t *mbm = (mem_gc_bitmap_t *)bm;
	if (mbm->myalloc)
		free(mbm->alloc.pixels);

	free(mbm);
	return EOK;
}

/** Render bitmap in memory GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t mem_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	mem_gc_bitmap_t *mbm = (mem_gc_bitmap_t *)bm;
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_coord2_t offs;
	gfx_coord2_t dim;
	gfx_coord_t x, y;
	gfx_coord2_t sdims;
	gfx_coord2_t ddims;
	pixelmap_t smap;
	pixelmap_t dmap;
	pixel_t pixel;

	if (srect0 != NULL)
		gfx_rect_clip(srect0, &mbm->rect, &srect);
	else
		srect = mbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);

	gfx_coord2_subtract(&drect.p1, &drect.p0, &dim);

	gfx_rect_dims(&mbm->rect, &sdims);
	smap.width = sdims.x;
	smap.height = sdims.y;
	smap.data = mbm->alloc.pixels;

	gfx_rect_dims(&mbm->mgc->rect, &ddims);
	dmap.width = ddims.x;
	dmap.height = ddims.y;
	dmap.data = mbm->mgc->alloc.pixels;

	if ((mbm->flags & bmpf_color_key) == 0) {
		for (y = drect.p0.y; y < drect.p1.y; y++) {
			for (x = drect.p0.x; x < drect.p1.x; x++) {
				pixel = pixelmap_get_pixel(&smap, x - offs.x,
				    y - offs.y);
				pixelmap_put_pixel(&dmap, x, y, pixel);
			}
		}
	} else {
		for (y = drect.p0.y; y < drect.p1.y; y++) {
			for (x = drect.p0.x; x < drect.p1.x; x++) {
				pixel = pixelmap_get_pixel(&smap, x - offs.x,
				    y - offs.y);
				if (pixel != mbm->key_color)
					pixelmap_put_pixel(&dmap, x, y, pixel);
			}
		}
	}

	mem_gc_invalidate_rect(mbm->mgc, &drect);
	return EOK;
}

/** Get allocation info for bitmap in memory GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t mem_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	mem_gc_bitmap_t *mbm = (mem_gc_bitmap_t *)bm;
	*alloc = mbm->alloc;
	return EOK;
}

/** @}
 */
