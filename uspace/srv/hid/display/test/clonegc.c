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

#include <errno.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <pcut/pcut.h>
#include <stdio.h>

#include "../clonegc.h"

PCUT_INIT;

PCUT_TEST_SUITE(clonegc);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

typedef struct {
	/** Error code to return */
	errno_t rc;

	bool set_clip_rect_called;
	gfx_rect_t *set_clip_rect_rect;

	bool set_color_called;
	gfx_color_t *set_color_color;

	bool fill_rect_called;
	gfx_rect_t *fill_rect_rect;

	bool bm_created;
	bool bm_destroyed;
	gfx_bitmap_params_t bm_params;
	void *bm_pixels;
	gfx_rect_t bm_srect;
	gfx_coord2_t bm_offs;
	bool bm_rendered;
	bool bm_got_alloc;
} test_gc_t;

typedef struct {
	test_gc_t *tgc;
	gfx_bitmap_alloc_t alloc;
	bool myalloc;
} testgc_bitmap_t;

enum {
	alloc_pitch = 42,
	alloc_off0 = 33
};

/** Test creating and deleting clone GC */
PCUT_TEST(create_delete)
{
	ds_clonegc_t *cgc;
	errno_t rc;

	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set clipping rectangle with two output GCs */
PCUT_TEST(set_clip_rect)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_rect_t rect;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add two output GCs */

	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	/* Set clipping rectangle returning error */

	tgc1.set_clip_rect_called = false;
	tgc2.set_clip_rect_called = false;
	tgc1.rc = EINVAL;
	tgc2.rc = EINVAL;

	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);

	PCUT_ASSERT_TRUE(tgc1.set_clip_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc1.set_clip_rect_rect);
	PCUT_ASSERT_FALSE(tgc2.set_clip_rect_called);

	/* Set clipping rectangle returning success for all outputs */
	tgc1.set_clip_rect_called = false;
	tgc2.set_clip_rect_called = false;
	tgc1.rc = EOK;
	tgc2.rc = EOK;

	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.set_clip_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc1.set_clip_rect_rect);
	PCUT_ASSERT_TRUE(tgc2.set_clip_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc2.set_clip_rect_rect);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test set color operation with two output GCs */
PCUT_TEST(set_color)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_color_t *color;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add two output GCs */

	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_color_new_rgb_i16(0xaaaa, 0xbbbb, 0xcccc, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Set color returning error */

	tgc1.set_color_called = false;
	tgc2.set_color_called = false;
	tgc1.rc = EINVAL;
	tgc2.rc = EINVAL;

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);

	PCUT_ASSERT_TRUE(tgc1.set_color_called);
	PCUT_ASSERT_EQUALS(color, tgc1.set_color_color);
	PCUT_ASSERT_FALSE(tgc2.set_color_called);

	/* Set color returning success for all outputs */
	tgc1.set_color_called = false;
	tgc2.set_color_called = false;
	tgc1.rc = EOK;
	tgc2.rc = EOK;

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.set_color_called);
	PCUT_ASSERT_EQUALS(color, tgc1.set_color_color);
	PCUT_ASSERT_TRUE(tgc2.set_color_called);
	PCUT_ASSERT_EQUALS(color, tgc2.set_color_color);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
}

/** Fill rectangle operation with two output GCs */
PCUT_TEST(fill_rect)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_rect_t rect;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add two output GCs */

	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	/* Fill rectangle returning error */

	tgc1.fill_rect_called = false;
	tgc2.fill_rect_called = false;
	tgc1.rc = EINVAL;
	tgc2.rc = EINVAL;

	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EINVAL, rc);

	PCUT_ASSERT_TRUE(tgc1.fill_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc1.fill_rect_rect);
	PCUT_ASSERT_FALSE(tgc2.fill_rect_called);

	/* Fill rectangle returning success for all outputs */
	tgc1.fill_rect_called = false;
	tgc2.fill_rect_called = false;
	tgc1.rc = EOK;
	tgc2.rc = EOK;

	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.fill_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc1.fill_rect_rect);
	PCUT_ASSERT_TRUE(tgc2.fill_rect_called);
	PCUT_ASSERT_EQUALS(&rect, tgc2.fill_rect_rect);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Operations on regular bitmap with two output GCs, callee allocation */
PCUT_TEST(bitmap_twogc_callee_alloc)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add two output GCs */

	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create bitmap with NULL allocation */
	tgc1.bm_created = false;
	tgc2.bm_created = false;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc1.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc1.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc1.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc1.bm_params.rect.p1.y);

	PCUT_ASSERT_TRUE(tgc2.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc2.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc2.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc2.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc2.bm_params.rect.p1.y);

	/* Get allocation */

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(alloc_pitch, alloc.pitch);
	PCUT_ASSERT_EQUALS(tgc1.bm_pixels, alloc.pixels);
	PCUT_ASSERT_EQUALS(tgc2.bm_pixels, alloc.pixels);

	/* Render bitmap */
	tgc1.bm_rendered = false;
	tgc1.bm_rendered = false;
	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	off.x = 50;
	off.y = 60;
	rc = gfx_bitmap_render(bitmap, &rect, &off);
	PCUT_ASSERT_TRUE(tgc1.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc1.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc1.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc1.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc1.bm_srect.p1.y);
	PCUT_ASSERT_TRUE(tgc2.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc2.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc2.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc2.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc2.bm_srect.p1.y);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Operations on regular bitmap with two output GCs, caller allocation */
PCUT_TEST(bitmap_twogc_caller_alloc)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_alloc_t galloc;
	void *pixels;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add two output GCs */
	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create bitmap with caller allocation */
	tgc1.bm_created = false;
	tgc2.bm_created = false;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	pixels = calloc(2 * 2, sizeof(uint32_t));
	PCUT_ASSERT_NOT_NULL(pixels);

	alloc.pitch = 8;
	alloc.off0 = 0;
	alloc.pixels = pixels;

	rc = gfx_bitmap_create(gc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc1.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc1.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc1.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc1.bm_params.rect.p1.y);

	PCUT_ASSERT_TRUE(tgc2.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc2.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc2.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc2.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc2.bm_params.rect.p1.y);

	/* Get allocation */
	rc = gfx_bitmap_get_alloc(bitmap, &galloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(alloc.pitch, galloc.pitch);
	PCUT_ASSERT_INT_EQUALS(alloc.off0, galloc.off0);
	PCUT_ASSERT_EQUALS(alloc.pixels, galloc.pixels);
	PCUT_ASSERT_EQUALS(tgc1.bm_pixels, galloc.pixels);
	PCUT_ASSERT_EQUALS(tgc2.bm_pixels, galloc.pixels);

	/* Render bitmap */
	tgc1.bm_rendered = false;
	tgc1.bm_rendered = false;
	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	off.x = 50;
	off.y = 60;
	rc = gfx_bitmap_render(bitmap, &rect, &off);
	PCUT_ASSERT_TRUE(tgc1.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc1.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc1.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc1.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc1.bm_srect.p1.y);
	PCUT_ASSERT_TRUE(tgc2.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc2.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc2.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc2.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc2.bm_srect.p1.y);

	free(pixels);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Create bitmap, then add second GC, callee allocation */
PCUT_TEST(bitmap_addgc_callee_alloc)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add one output GC */
	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create bitmap with NULL allocation */
	tgc1.bm_created = false;
	tgc2.bm_created = false;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc1.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc1.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc1.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc1.bm_params.rect.p1.y);

	PCUT_ASSERT_FALSE(tgc2.bm_created);

	/* Get allocation */
	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(alloc_pitch, alloc.pitch);
	PCUT_ASSERT_EQUALS(tgc1.bm_pixels, alloc.pixels);

	/* Add second output GC */
	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Render bitmap */
	tgc1.bm_rendered = false;
	tgc1.bm_rendered = false;
	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	off.x = 50;
	off.y = 60;
	rc = gfx_bitmap_render(bitmap, &rect, &off);
	PCUT_ASSERT_TRUE(tgc1.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc1.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc1.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc1.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc1.bm_srect.p1.y);
	PCUT_ASSERT_TRUE(tgc2.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc2.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc2.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc2.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc2.bm_srect.p1.y);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Create bitmap, then add second GC, caller allocation */
PCUT_TEST(bitmap_addgc_caller_alloc)
{
	ds_clonegc_t *cgc;
	gfx_context_t *gc;
	test_gc_t tgc1;
	gfx_context_t *gc1;
	test_gc_t tgc2;
	gfx_context_t *gc2;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_alloc_t galloc;
	void *pixels;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	/* Create clone GC */
	rc = ds_clonegc_create(NULL, &cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ds_clonegc_get_ctx(cgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Add one output GC */
	rc = gfx_context_new(&ops, &tgc1, &gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create bitmap with caller allocation */
	tgc1.bm_created = false;
	tgc2.bm_created = false;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	pixels = calloc(2 * 2, sizeof(uint32_t));
	PCUT_ASSERT_NOT_NULL(pixels);

	alloc.pitch = 8;
	alloc.off0 = 0;
	alloc.pixels = pixels;

	rc = gfx_bitmap_create(gc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc1.bm_created);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, tgc1.bm_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, tgc1.bm_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, tgc1.bm_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, tgc1.bm_params.rect.p1.y);

	PCUT_ASSERT_FALSE(tgc2.bm_created);

	/* Get allocation */
	rc = gfx_bitmap_get_alloc(bitmap, &galloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(alloc.pitch, galloc.pitch);
	PCUT_ASSERT_INT_EQUALS(alloc.off0, galloc.off0);
	PCUT_ASSERT_EQUALS(alloc.pixels, galloc.pixels);
	PCUT_ASSERT_EQUALS(tgc1.bm_pixels, galloc.pixels);

	/* Add second output GC */
	rc = gfx_context_new(&ops, &tgc2, &gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_clonegc_add_output(cgc, gc2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Render bitmap */
	tgc1.bm_rendered = false;
	tgc1.bm_rendered = false;
	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	off.x = 50;
	off.y = 60;
	rc = gfx_bitmap_render(bitmap, &rect, &off);
	PCUT_ASSERT_TRUE(tgc1.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc1.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc1.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc1.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc1.bm_srect.p1.y);
	PCUT_ASSERT_TRUE(tgc2.bm_rendered);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc2.bm_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc2.bm_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc2.bm_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc2.bm_srect.p1.y);

	free(pixels);

	rc = ds_clonegc_delete(cgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->set_clip_rect_called = true;
	tgc->set_clip_rect_rect = rect;

	return tgc->rc;
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->set_color_called = true;
	tgc->set_color_color = color;

	return tgc->rc;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->fill_rect_called = true;
	tgc->fill_rect_rect = rect;

	return tgc->rc;
}

static errno_t testgc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	test_gc_t *tgc = (test_gc_t *) arg;
	testgc_bitmap_t *tbm;

	tbm = calloc(1, sizeof(testgc_bitmap_t));
	if (tbm == NULL)
		return ENOMEM;

	if (alloc == NULL) {
		tbm->alloc.pitch = alloc_pitch;
		tbm->alloc.off0 = alloc_off0;
		tbm->alloc.pixels = calloc(1, 420);
		tbm->myalloc = true;
		if (tbm->alloc.pixels == NULL) {
			free(tbm);
			return ENOMEM;
		}
	} else {
		tbm->alloc = *alloc;
	}

	tbm->tgc = tgc;
	tgc->bm_created = true;
	tgc->bm_params = *params;
	tgc->bm_pixels = tbm->alloc.pixels;
	*rbm = (void *)tbm;
	return EOK;
}

static errno_t testgc_bitmap_destroy(void *bm)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	if (tbm->myalloc)
		free(tbm->alloc.pixels);
	tbm->tgc->bm_destroyed = true;
	free(tbm);
	return EOK;
}

static errno_t testgc_bitmap_render(void *bm, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	tbm->tgc->bm_rendered = true;
	tbm->tgc->bm_srect = *srect;
	tbm->tgc->bm_offs = *offs;
	return EOK;
}

static errno_t testgc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	*alloc = tbm->alloc;
	tbm->tgc->bm_got_alloc = true;
	return EOK;
}

PCUT_EXPORT(clonegc);
