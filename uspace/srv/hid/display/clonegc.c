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

/** @addtogroup display
 * @{
 */
/**
 * @file Cloning graphics context
 *
 * This implements a graphics context that duplicates rendering to a number
 * of GCs.
 */

#include <assert.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <stdlib.h>
#include "clonegc.h"

static errno_t ds_clonegc_set_clip_rect(void *, gfx_rect_t *);
static errno_t ds_clonegc_set_color(void *, gfx_color_t *);
static errno_t ds_clonegc_fill_rect(void *, gfx_rect_t *);
static errno_t ds_clonegc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t ds_clonegc_bitmap_destroy(void *);
static errno_t ds_clonegc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t ds_clonegc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static ds_clonegc_output_t *ds_clonegc_first_output(ds_clonegc_t *);
static ds_clonegc_output_t *ds_clonegc_next_output(ds_clonegc_output_t *);
static ds_clonegc_bitmap_t *ds_clonegc_first_bitmap(ds_clonegc_t *);
static ds_clonegc_bitmap_t *ds_clonegc_next_bitmap(ds_clonegc_bitmap_t *);
static ds_clonegc_outbitmap_t *ds_clonegc_bitmap_first_obm(ds_clonegc_bitmap_t *);
static ds_clonegc_outbitmap_t *ds_clonegc_bitmap_next_obm(ds_clonegc_outbitmap_t *);
static errno_t ds_clonegc_outbitmap_create(ds_clonegc_output_t *,
    ds_clonegc_bitmap_t *, gfx_bitmap_t *);
static errno_t ds_clonegc_outbitmap_destroy(ds_clonegc_outbitmap_t *);
static errno_t ds_clonegc_bitmap_add_output(ds_clonegc_bitmap_t *,
    ds_clonegc_output_t *);

gfx_context_ops_t ds_clonegc_ops = {
	.set_clip_rect = ds_clonegc_set_clip_rect,
	.set_color = ds_clonegc_set_color,
	.fill_rect = ds_clonegc_fill_rect,
	.bitmap_create = ds_clonegc_bitmap_create,
	.bitmap_destroy = ds_clonegc_bitmap_destroy,
	.bitmap_render = ds_clonegc_bitmap_render,
	.bitmap_get_alloc = ds_clonegc_bitmap_get_alloc
};

/** Set clipping rectangle on clone GC.
 *
 * @param arg Clone GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	ds_clonegc_t *cgc = (ds_clonegc_t *)arg;
	ds_clonegc_output_t *output;
	errno_t rc;

	output = ds_clonegc_first_output(cgc);
	while (output != NULL) {
		rc = gfx_set_clip_rect(output->gc, rect);
		if (rc != EOK)
			return rc;

		output = ds_clonegc_next_output(output);
	}

	return EOK;
}

/** Set color on clone GC.
 *
 * Set drawing color on memory GC.
 *
 * @param arg Clone GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_set_color(void *arg, gfx_color_t *color)
{
	ds_clonegc_t *cgc = (ds_clonegc_t *)arg;
	ds_clonegc_output_t *output;
	errno_t rc;

	output = ds_clonegc_first_output(cgc);
	while (output != NULL) {
		rc = gfx_set_color(output->gc, color);
		if (rc != EOK)
			return rc;

		output = ds_clonegc_next_output(output);
	}

	return EOK;
}

/** Fill rectangle on clone GC.
 *
 * @param arg Clone GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_fill_rect(void *arg, gfx_rect_t *rect)
{
	ds_clonegc_t *cgc = (ds_clonegc_t *)arg;
	ds_clonegc_output_t *output;
	errno_t rc;

	output = ds_clonegc_first_output(cgc);
	while (output != NULL) {
		rc = gfx_fill_rect(output->gc, rect);
		if (rc != EOK)
			return rc;

		output = ds_clonegc_next_output(output);
	}

	return EOK;
}

/** Create cloning GC.
 *
 * Create graphics context for copying rendering into several GCs.
 *
 * @param outgc Primary output GC
 * @param rgc Place to store pointer to new clone GC
 *
 * @return EOK on success or an error code
 */
errno_t ds_clonegc_create(gfx_context_t *outgc, ds_clonegc_t **rgc)
{
	ds_clonegc_t *cgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	cgc = calloc(1, sizeof(ds_clonegc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&ds_clonegc_ops, cgc, &gc);
	if (rc != EOK)
		goto error;

	cgc->gc = gc;
	list_initialize(&cgc->outputs);
	list_initialize(&cgc->bitmaps);

	if (outgc != NULL) {
		rc = ds_clonegc_add_output(cgc, outgc);
		if (rc != EOK)
			goto error;
	}

	*rgc = cgc;
	return EOK;
error:
	if (cgc != NULL)
		free(cgc);
	if (gc != NULL)
		gfx_context_delete(gc);
	return rc;
}

/** Delete cloning GC.
 *
 * @param cgc Cloning GC
 */
errno_t ds_clonegc_delete(ds_clonegc_t *cgc)
{
	errno_t rc;

	rc = gfx_context_delete(cgc->gc);
	if (rc != EOK)
		return rc;

	free(cgc);
	return EOK;
}

/** Add new output to cloning GC.
 *
 * @param cgc Cloning GC
 * @param outgc New output GC
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_clonegc_add_output(ds_clonegc_t *cgc, gfx_context_t *outgc)
{
	ds_clonegc_output_t *output;
	ds_clonegc_bitmap_t *cbm;
	errno_t rc;

	output = calloc(1, sizeof(ds_clonegc_output_t));
	if (output == NULL)
		return ENOMEM;

	output->clonegc = cgc;
	list_append(&output->loutputs, &cgc->outputs);
	output->gc = outgc;
	list_initialize(&output->obitmaps);

	/* Extend each existing bitmap to the new output */
	cbm = ds_clonegc_first_bitmap(cgc);
	while (cbm != NULL) {
		rc = ds_clonegc_bitmap_add_output(cbm, output);
		if (rc != EOK)
			goto error;

		cbm = ds_clonegc_next_bitmap(cbm);
	}

	return EOK;
error:
	list_remove(&output->loutputs);
	free(output);
	return rc;
}

/** Get generic graphic context from clone GC.
 *
 * @param cgc Clone GC
 * @return Graphic context
 */
gfx_context_t *ds_clonegc_get_ctx(ds_clonegc_t *cgc)
{
	return cgc->gc;
}

/** Create bitmap in clone GC.
 *
 * @param arg Clone GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t ds_clonegc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	ds_clonegc_t *cgc = (ds_clonegc_t *)arg;
	ds_clonegc_bitmap_t *cbm = NULL;
	ds_clonegc_output_t *output;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	cbm = calloc(1, sizeof(ds_clonegc_bitmap_t));
	if (cbm == NULL)
		return ENOMEM;

	list_initialize(&cbm->obitmaps);
	cbm->clonegc = cgc;
	cbm->params = *params;

	output = ds_clonegc_first_output(cgc);
	assert(output != NULL);

	/* Create the first output bitmap */

	rc = gfx_bitmap_create(output->gc, params, alloc, &bitmap);
	if (rc != EOK)
		goto error;

	rc = gfx_bitmap_get_alloc(bitmap, &cbm->alloc);
	if (rc != EOK)
		goto error;

	rc = ds_clonegc_outbitmap_create(output, cbm, bitmap);
	if (rc != EOK)
		goto error;

	bitmap = NULL;

	/* Create all other output bitmaps as copies */
	output = ds_clonegc_next_output(output);
	while (output != NULL) {
		rc = ds_clonegc_bitmap_add_output(cbm, output);
		if (rc != EOK)
			goto error;

		output = ds_clonegc_next_output(output);
	}

	list_append(&cbm->lbitmaps, &cgc->bitmaps);
	*rbm = (void *)cbm;
	return EOK;
error:
	if (bitmap != NULL)
		gfx_bitmap_destroy(bitmap);
	if (cbm != NULL)
		ds_clonegc_bitmap_destroy(cbm);
	return rc;
}

/** Destroy bitmap in memory GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_bitmap_destroy(void *bm)
{
	ds_clonegc_bitmap_t *cbm = (ds_clonegc_bitmap_t *)bm;
	ds_clonegc_outbitmap_t *outbm;
	errno_t rc;

	outbm = ds_clonegc_bitmap_first_obm(cbm);
	while (outbm != NULL) {
		rc = ds_clonegc_outbitmap_destroy(outbm);
		if (rc != EOK)
			return rc;

		outbm = ds_clonegc_bitmap_first_obm(cbm);
	}

	list_remove(&cbm->lbitmaps);
	free(cbm);
	return EOK;
}

/** Render bitmap in memory GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	ds_clonegc_bitmap_t *cbm = (ds_clonegc_bitmap_t *)bm;
	ds_clonegc_outbitmap_t *outbm;
	errno_t rc;

	outbm = ds_clonegc_bitmap_first_obm(cbm);
	while (outbm != NULL) {
		rc = gfx_bitmap_render(outbm->obitmap, srect0, offs0);
		if (rc != EOK)
			return rc;

		outbm = ds_clonegc_bitmap_next_obm(outbm);
	}

	return EOK;
}

/** Get allocation info for bitmap in memory GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	ds_clonegc_bitmap_t *cbm = (ds_clonegc_bitmap_t *)bm;

	*alloc = cbm->alloc;
	return EOK;
}

/** Get first clone GC output.
 *
 * @param cgc Clone GC
 * @return First output or @c NULL if there are none
 */
static ds_clonegc_output_t *ds_clonegc_first_output(ds_clonegc_t *cgc)
{
	link_t *link;

	link = list_first(&cgc->outputs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_output_t, loutputs);
}

/** Get next clone GC output.
 *
 * @param cur Current output
 * @return Next output or @c NULL if @a cur is the last
 */
static ds_clonegc_output_t *ds_clonegc_next_output(ds_clonegc_output_t *cur)
{
	link_t *link;

	link = list_next(&cur->loutputs, &cur->clonegc->outputs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_output_t, loutputs);
}

/** Get first clone GC bitmap.
 *
 * @param cgc Clone GC
 * @return First bitmap or @c NULL if there are none
 */
static ds_clonegc_bitmap_t *ds_clonegc_first_bitmap(ds_clonegc_t *cgc)
{
	link_t *link;

	link = list_first(&cgc->bitmaps);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_bitmap_t, lbitmaps);
}

/** Get next clone GC bitmap.
 *
 * @param cur Current bitmap
 * @return Next bitmap or @c NULL if @a cur is the last
 */
static ds_clonegc_bitmap_t *ds_clonegc_next_bitmap(ds_clonegc_bitmap_t *cur)
{
	link_t *link;

	link = list_next(&cur->lbitmaps, &cur->clonegc->bitmaps);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_bitmap_t, lbitmaps);
}

/** Get first output bitmap of a clone GC bitmap.
 *
 * @param cbm Clone GC bitmap
 * @return First output bitmap or @c NULL if there are none
 */
static ds_clonegc_outbitmap_t *ds_clonegc_bitmap_first_obm(ds_clonegc_bitmap_t *cbm)
{
	link_t *link;

	link = list_first(&cbm->obitmaps);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_outbitmap_t, lbbitmaps);
}

/** Get next output bitmap of a clone GC bitmap.
 *
 * @param cur Current output bitmap
 * @return Next output bitmap or @c NULL if @a cur is the last
 */
static ds_clonegc_outbitmap_t *ds_clonegc_bitmap_next_obm(ds_clonegc_outbitmap_t *cur)
{
	link_t *link;

	link = list_next(&cur->lbbitmaps, &cur->bitmap->obitmaps);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_clonegc_outbitmap_t, lbbitmaps);
}

/** Create clone GC output bitmap.
 *
 * Create a new entry in the output x bitmap matrix.
 *
 * @param output Clone GC output
 * @param cbm Clone GC bitmap
 * @param obitmap Output bitmap
 */
static errno_t ds_clonegc_outbitmap_create(ds_clonegc_output_t *output,
    ds_clonegc_bitmap_t *cbm, gfx_bitmap_t *obitmap)
{
	ds_clonegc_outbitmap_t *outbm;

	outbm = calloc(1, sizeof(ds_clonegc_outbitmap_t));
	if (outbm == NULL)
		return ENOMEM;

	outbm->output = output;
	outbm->bitmap = cbm;
	list_append(&outbm->lobitmaps, &output->obitmaps);
	list_append(&outbm->lbbitmaps, &cbm->obitmaps);
	outbm->obitmap = obitmap;

	return EOK;
}

/** Destroy clone GC output bitmap.
 *
 * @param outbm Output bitmap
 * @return EOK on success or an error code
 */
static errno_t ds_clonegc_outbitmap_destroy(ds_clonegc_outbitmap_t *outbm)
{
	errno_t rc;

	rc = gfx_bitmap_destroy(outbm->obitmap);
	if (rc != EOK)
		return rc;

	list_remove(&outbm->lobitmaps);
	list_remove(&outbm->lbbitmaps);
	free(outbm);
	return EOK;
}

/** Extend clone GC bitmap to new output.
 *
 * @param cbm Clone GC bitmap
 * @param output Clone GC output
 */
static errno_t ds_clonegc_bitmap_add_output(ds_clonegc_bitmap_t *cbm,
    ds_clonegc_output_t *output)
{
	gfx_bitmap_t *obitmap = NULL;
	errno_t rc;

	rc = gfx_bitmap_create(output->gc, &cbm->params, &cbm->alloc, &obitmap);
	if (rc != EOK)
		goto error;

	rc = ds_clonegc_outbitmap_create(output, cbm, obitmap);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (obitmap != NULL)
		gfx_bitmap_destroy(obitmap);
	return rc;
}

/** @}
 */
