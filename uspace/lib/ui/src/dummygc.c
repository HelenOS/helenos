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

/** @addtogroup libui
 * @{
 */
/**
 * @file Dummy graphic context
 */

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../private/dummygc.h"

static errno_t dummygc_set_clip_rect(void *, gfx_rect_t *);
static errno_t dummygc_set_color(void *, gfx_color_t *);
static errno_t dummygc_fill_rect(void *, gfx_rect_t *);
static errno_t dummygc_update(void *);
static errno_t dummygc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t dummygc_bitmap_destroy(void *);
static errno_t dummygc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t dummygc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

/** Dummy GC operations */
gfx_context_ops_t dummygc_ops = {
	.set_clip_rect = dummygc_set_clip_rect,
	.set_color = dummygc_set_color,
	.fill_rect = dummygc_fill_rect,
	.update = dummygc_update,
	.bitmap_create = dummygc_bitmap_create,
	.bitmap_destroy = dummygc_bitmap_destroy,
	.bitmap_render = dummygc_bitmap_render,
	.bitmap_get_alloc = dummygc_bitmap_get_alloc
};

/** Create dummy GC.
 *
 * @param rdgc Place to store pointer to new dummy GC
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t dummygc_create(dummy_gc_t **rdgc)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	errno_t rc;

	dgc = calloc(1, sizeof(dummy_gc_t));
	if (dgc == NULL)
		return ENOMEM;

	rc = gfx_context_new(&dummygc_ops, dgc, &gc);
	if (rc != EOK) {
		free(dgc);
		return rc;
	}

	dgc->gc = gc;
	*rdgc = dgc;
	return EOK;
}

/** Destroy dummy GC.
 *
 * @param dgc Dummy GC
 */
void dummygc_destroy(dummy_gc_t *dgc)
{
	gfx_context_delete(dgc->gc);
	free(dgc);
}

/** Get generic graphic context from dummy GC.
 *
 * @param dgc Dummy GC
 * @return Graphic context
 */
gfx_context_t *dummygc_get_ctx(dummy_gc_t *dgc)
{
	return dgc->gc;
}

/** Set clipping rectangle on dummy GC
 *
 * @param arg Argument (dummy_gc_t)
 * @param rect Rectangle
 * @return EOK on success or an error code
 */
static errno_t dummygc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

/** Set color on dummy GC
 *
 * @param arg Argument (dummy_gc_t)
 * @param color Color
 * @return EOK on success or an error code
 */
static errno_t dummygc_set_color(void *arg, gfx_color_t *color)
{
	(void) arg;
	(void) color;
	return EOK;
}

/** Fill rectangle on dummy GC
 *
 * @param arg Argument (dummy_gc_t)
 * @param rect Rectangle
 * @return EOK on success or an error code
 */
static errno_t dummygc_fill_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

/** Update dummy GC
 *
 * @param arg Argument (dummy_gc_t)
 * @return EOK on success or an error code
 */
static errno_t dummygc_update(void *arg)
{
	(void) arg;
	return EOK;
}

/** Create bitmap on dummy GC
 *
 * @param arg Argument (dummy_gc_t)
 * @param params Bitmap parameters
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
static errno_t dummygc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	dummy_gc_t *dgc = (dummy_gc_t *) arg;
	dummygc_bitmap_t *tbm;

	tbm = calloc(1, sizeof(dummygc_bitmap_t));
	if (tbm == NULL)
		return ENOMEM;

	if (alloc == NULL) {
		tbm->alloc.pitch = (params->rect.p1.x - params->rect.p0.x) *
		    sizeof(uint32_t);
		tbm->alloc.off0 = 0;
		tbm->alloc.pixels = calloc(sizeof(uint32_t),
		    (params->rect.p1.x - params->rect.p0.x) *
		    (params->rect.p1.y - params->rect.p0.y));
		tbm->myalloc = true;
		if (tbm->alloc.pixels == NULL) {
			free(tbm);
			return ENOMEM;
		}
	} else {
		tbm->alloc = *alloc;
	}

	tbm->dgc = dgc;
	dgc->bm_created = true;
	dgc->bm_params = *params;
	dgc->bm_pixels = tbm->alloc.pixels;
	*rbm = (void *)tbm;
	return EOK;
}

/** Destroy bitmap on dummy GC
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t dummygc_bitmap_destroy(void *bm)
{
	dummygc_bitmap_t *tbm = (dummygc_bitmap_t *)bm;
	if (tbm->myalloc)
		free(tbm->alloc.pixels);
	tbm->dgc->bm_destroyed = true;
	free(tbm);
	return EOK;
}

/** Render bitmap on dummy GC
 *
 * @param bm Bitmap
 * @param srect Source rectangle or @c NULL
 * @param offs Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t dummygc_bitmap_render(void *bm, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	dummygc_bitmap_t *tbm = (dummygc_bitmap_t *)bm;

	tbm->dgc->bm_rendered = true;

	tbm->dgc->bm_srect.p0.x = 0;
	tbm->dgc->bm_srect.p0.y = 0;
	tbm->dgc->bm_srect.p1.x = 0;
	tbm->dgc->bm_srect.p1.y = 0;

	tbm->dgc->bm_offs.x = 0;
	tbm->dgc->bm_offs.y = 0;

	if (srect != NULL)
		tbm->dgc->bm_srect = *srect;

	if (offs != NULL)
		tbm->dgc->bm_offs = *offs;

	return EOK;
}

/** Get bitmap allocation info on dummy GC
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t dummygc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	dummygc_bitmap_t *tbm = (dummygc_bitmap_t *)bm;
	*alloc = tbm->alloc;
	tbm->dgc->bm_got_alloc = true;
	return EOK;
}

/** @}
 */
