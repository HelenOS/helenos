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
 * @file GFX memory backend
 *
 * This implements a graphics context over a block of memory (i.e. a simple
 * software renderer).
 */

#include <as.h>
#include <assert.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/pixel.h>
#include <io/pixelmap.h>
#include <memgfx/memgc.h>
#include <stdlib.h>
#include "../private/memgc.h"

static errno_t mem_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t mem_gc_set_color(void *, gfx_color_t *);
static errno_t mem_gc_fill_rect(void *, gfx_rect_t *);
static errno_t mem_gc_update(void *);
static errno_t mem_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t mem_gc_bitmap_destroy(void *);
static errno_t mem_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t mem_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);
static errno_t mem_gc_cursor_get_pos(void *, gfx_coord2_t *);
static errno_t mem_gc_cursor_set_pos(void *, gfx_coord2_t *);
static errno_t mem_gc_cursor_set_visible(void *, bool);
static void mem_gc_invalidate_rect(mem_gc_t *, gfx_rect_t *);

gfx_context_ops_t mem_gc_ops = {
	.set_clip_rect = mem_gc_set_clip_rect,
	.set_color = mem_gc_set_color,
	.fill_rect = mem_gc_fill_rect,
	.update = mem_gc_update,
	.bitmap_create = mem_gc_bitmap_create,
	.bitmap_destroy = mem_gc_bitmap_destroy,
	.bitmap_render = mem_gc_bitmap_render,
	.bitmap_get_alloc = mem_gc_bitmap_get_alloc,
	.cursor_get_pos = mem_gc_cursor_get_pos,
	.cursor_set_pos = mem_gc_cursor_set_pos,
	.cursor_set_visible = mem_gc_cursor_set_visible
};

/** Set clipping rectangle on memory GC.
 *
 * @param arg Memory GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;

	if (rect != NULL)
		gfx_rect_clip(rect, &mgc->rect, &mgc->clip_rect);
	else
		mgc->clip_rect = mgc->rect;

	return EOK;
}

/** Set color on memory GC.
 *
 * Set drawing color on memory GC.
 *
 * @param arg Memory GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_set_color(void *arg, gfx_color_t *color)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;
	uint16_t r, g, b;
	uint8_t attr;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	gfx_color_get_ega(color, &attr);
	mgc->color = PIXEL(attr, r >> 8, g >> 8, b >> 8);
	return EOK;
}

/** Fill rectangle on memory GC.
 *
 * @param arg Memory GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;
	gfx_rect_t crect;
	gfx_coord_t x, y;
	pixelmap_t pixelmap;

	/* Make sure we have a sorted, clipped rectangle */
	gfx_rect_clip(rect, &mgc->clip_rect, &crect);

	assert(mgc->rect.p0.x == 0);
	assert(mgc->rect.p0.y == 0);
	assert(mgc->alloc.pitch == mgc->rect.p1.x * (int)sizeof(uint32_t));
	pixelmap.width = mgc->rect.p1.x;
	pixelmap.height = mgc->rect.p1.y;
	pixelmap.data = mgc->alloc.pixels;

	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			pixelmap_put_pixel(&pixelmap, x, y, mgc->color);
		}
	}

	mem_gc_invalidate_rect(mgc, &crect);
	return EOK;
}

/** Update memory GC.
 *
 * @param arg Memory GC
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_update(void *arg)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;

	mgc->cb->update(mgc->cb_arg);
	return EOK;
}

/** Create memory GC.
 *
 * Create graphics context for rendering into a block of memory.
 *
 * @param rect Bounding rectangle
 * @param alloc Allocation info
 * @param cb Pointer to memory GC callbacks
 * @param cb_arg Argument to callback functions
 * @param rgc Place to store pointer to new memory GC
 *
 * @return EOK on success or an error code
 */
errno_t mem_gc_create(gfx_rect_t *rect, gfx_bitmap_alloc_t *alloc,
    mem_gc_cb_t *cb, void *cb_arg, mem_gc_t **rgc)
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
	mgc->clip_rect = *rect;
	mgc->alloc = *alloc;

	mgc->cb = cb;
	mgc->cb_arg = cb_arg;

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
 * @param mgc Memory GC
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

/** Retarget memory GC to a different block of memory.
 *
 * @param mgc Memory GC
 * @param rect New bounding rectangle
 * @param alloc Allocation info of the new block
 */
void mem_gc_retarget(mem_gc_t *mgc, gfx_rect_t *rect,
    gfx_bitmap_alloc_t *alloc)
{
	mgc->rect = *rect;
	mgc->clip_rect = *rect;
	mgc->alloc = *alloc;
}

/** Get generic graphic context from memory GC.
 *
 * @param mgc Memory GC
 * @return Graphic context
 */
gfx_context_t *mem_gc_get_ctx(mem_gc_t *mgc)
{
	return mgc->gc;
}

static void mem_gc_invalidate_rect(mem_gc_t *mgc, gfx_rect_t *rect)
{
	mgc->cb->invalidate(mgc->cb_arg, rect);
}

/** Create bitmap in memory GC.
 *
 * @param arg Memory GC
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

	/* Check that we support all requested flags */
	if ((params->flags & ~(bmpf_color_key | bmpf_colorize |
	    bmpf_direct_output)) != 0)
		return ENOTSUP;

	mbm = calloc(1, sizeof(mem_gc_bitmap_t));
	if (mbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	mbm->rect = params->rect;
	mbm->flags = params->flags;
	mbm->key_color = params->key_color;

	if ((params->flags & bmpf_direct_output) != 0) {
		/* Caller cannot specify allocation for direct output */
		if (alloc != NULL) {
			rc = EINVAL;
			goto error;
		}

		/* Bounding rectangle must be within GC bounding rectangle */
		if (!gfx_rect_is_inside(&mbm->rect, &mgc->rect)) {
			rc = EINVAL;
			goto error;
		}

		mbm->alloc = mgc->alloc;

		/* Don't free pixel array when destroying bitmap */
		mbm->myalloc = false;
	} else if (alloc == NULL) {
#if 0
		/*
		 * TODO: If the bitmap is not required to be sharable,
		 * we could allocate it with a simple malloc.
		 * Need to have a bitmap flag specifying that the
		 * allocation should be sharable. IPC GC could
		 * automatically add this flag
		 */
		mbm->alloc.pitch = dim.x * sizeof(uint32_t);
		mbm->alloc.off0 = 0;
		mbm->alloc.pixels = malloc(mbm->alloc.pitch * dim.y);
		mbm->myalloc = true;

		if (mbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}
#endif
		mbm->alloc.pitch = dim.x * sizeof(uint32_t);
		mbm->alloc.off0 = 0;
		mbm->alloc.pixels = as_area_create(AS_AREA_ANY,
		    dim.x * dim.y * sizeof(uint32_t), AS_AREA_READ |
		    AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
		mbm->myalloc = true;

		if (mbm->alloc.pixels == AS_MAP_FAILED) {
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
	if (mbm->myalloc) {
#if 0
		/* TODO: if we alloc allocating the bitmap with malloc */
		free(mbm->alloc.pixels);
#endif
		as_area_destroy(mbm->alloc.pixels);
	}

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
	gfx_rect_t crect;
	gfx_coord2_t offs;
	gfx_coord_t x, y;
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

	/* Clip destination rectangle */
	gfx_rect_clip(&drect, &mbm->mgc->clip_rect, &crect);

	assert(mbm->alloc.pitch == (mbm->rect.p1.x - mbm->rect.p0.x) *
	    (int)sizeof(uint32_t));
	smap.width = mbm->rect.p1.x - mbm->rect.p0.x;
	smap.height = mbm->rect.p1.y - mbm->rect.p0.y;
	smap.data = mbm->alloc.pixels;

	assert(mbm->mgc->rect.p0.x == 0);
	assert(mbm->mgc->rect.p0.y == 0);
	assert(mbm->mgc->alloc.pitch == mbm->mgc->rect.p1.x * (int)sizeof(uint32_t));
	dmap.width = mbm->mgc->rect.p1.x;
	dmap.height = mbm->mgc->rect.p1.y;
	dmap.data = mbm->mgc->alloc.pixels;

	if ((mbm->flags & bmpf_direct_output) != 0) {
		/* Nothing to do */
	} else if ((mbm->flags & bmpf_color_key) == 0) {
		/* Simple copy */
		for (y = crect.p0.y; y < crect.p1.y; y++) {
			for (x = crect.p0.x; x < crect.p1.x; x++) {
				pixel = pixelmap_get_pixel(&smap,
				    x - mbm->rect.p0.x - offs.x,
				    y - mbm->rect.p0.y - offs.y);
				pixelmap_put_pixel(&dmap, x, y, pixel);
			}
		}
	} else if ((mbm->flags & bmpf_colorize) == 0) {
		/* Color key */
		for (y = crect.p0.y; y < crect.p1.y; y++) {
			for (x = crect.p0.x; x < crect.p1.x; x++) {
				pixel = pixelmap_get_pixel(&smap,
				    x - mbm->rect.p0.x - offs.x,
				    y - mbm->rect.p0.y - offs.y);
				if (pixel != mbm->key_color)
					pixelmap_put_pixel(&dmap, x, y, pixel);
			}
		}
	} else {
		/* Color key & colorization */
		for (y = crect.p0.y; y < crect.p1.y; y++) {
			for (x = crect.p0.x; x < crect.p1.x; x++) {
				pixel = pixelmap_get_pixel(&smap,
				    x - mbm->rect.p0.x - offs.x,
				    y - mbm->rect.p0.y - offs.y);
				if (pixel != mbm->key_color)
					pixelmap_put_pixel(&dmap, x, y,
					    mbm->mgc->color);
			}
		}
	}

	mem_gc_invalidate_rect(mbm->mgc, &crect);
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

/** Get cursor position on memory GC.
 *
 * @param arg Memory GC
 * @param pos Place to store position
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_cursor_get_pos(void *arg, gfx_coord2_t *pos)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;

	if (mgc->cb->cursor_get_pos != NULL)
		return mgc->cb->cursor_get_pos(mgc->cb_arg, pos);
	else
		return ENOTSUP;
}

/** Set cursor position on memory GC.
 *
 * @param arg Memory GC
 * @param pos New position
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_cursor_set_pos(void *arg, gfx_coord2_t *pos)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;

	if (mgc->cb->cursor_set_pos != NULL)
		return mgc->cb->cursor_set_pos(mgc->cb_arg, pos);
	else
		return ENOTSUP;
}

/** Set cursor visibility on memory GC.
 *
 * @param arg Memory GC
 * @param visible @c true iff cursor should be made visible
 *
 * @return EOK on success or an error code
 */
static errno_t mem_gc_cursor_set_visible(void *arg, bool visible)
{
	mem_gc_t *mgc = (mem_gc_t *) arg;

	if (mgc->cb->cursor_set_visible != NULL)
		return mgc->cb->cursor_set_visible(mgc->cb_arg, visible);
	else
		return ENOTSUP;
}

/** @}
 */
