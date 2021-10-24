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
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/cursor.h>
#include <gfx/render.h>
#include <mem.h>
#include <memgfx/xlategc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(xlategc);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_update(void *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);
static errno_t testgc_cursor_get_pos(void *, gfx_coord2_t *);
static errno_t testgc_cursor_set_pos(void *, gfx_coord2_t *);
static errno_t testgc_cursor_set_visible(void *, bool);

static gfx_context_ops_t testgc_ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.update = testgc_update,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc,
	.cursor_get_pos = testgc_cursor_get_pos,
	.cursor_set_pos = testgc_cursor_set_pos,
	.cursor_set_visible = testgc_cursor_set_visible
};

typedef struct {
	/** Return code to return */
	errno_t rc;
	/** True if set_clip_rect was called */
	bool set_clip_rect_called;
	/** Argument to set_clip_rect */
	gfx_rect_t set_clip_rect_rect;
	/** True if set_color was called */
	bool set_color_called;
	/** Argument to set_color (red) */
	uint16_t set_color_r;
	/** Argument to set_color (green) */
	uint16_t set_color_g;
	/** Argument to set_color (blue) */
	uint16_t set_color_b;
	/** True if fill_rect was called */
	bool fill_rect_called;
	/** Argument to fill_rect */
	gfx_rect_t fill_rect_rect;
	/** True if bitmap_create was called */
	bool bitmap_create_called;
	/** Bitmap parameters passed to bitmap_create */
	gfx_bitmap_params_t bitmap_create_params;
	/** Allocation info passed to bitmap_create */
	gfx_bitmap_alloc_t bitmap_create_alloc;
	/** Cookie to store in the created bitmap */
	uint32_t bitmap_create_cookie;
	/** True if bitmap_destroy was called */
	bool bitmap_destroy_called;
	/** Cookie retrieved from bitmap by bitmap_destroy */
	uint32_t bitmap_destroy_cookie;
	/** True if bitmap_render was called */
	bool bitmap_render_called;
	/** Source rectangle passed to bitmap_render */
	gfx_rect_t bitmap_render_srect;
	/** Offset passed to bitmap_render */
	gfx_coord2_t bitmap_render_off;
	/** Cookie retrieved from bitmap by bitmap_render */
	uint32_t bitmap_render_cookie;
	/** True if bitmap_get_alloc was called */
	bool bitmap_get_alloc_called;
	/** Cookie retrieved from bitmap by bitmap_get_alloc */
	uint32_t bitmap_get_alloc_cookie;
	/** Allocation info to return from bitmap_get_alloc */
	gfx_bitmap_alloc_t bitmap_get_alloc_alloc;
	/** True if update was called */
	bool update_called;
	/** True if cursor_get_pos was called */
	bool cursor_get_pos_called;
	/** Position to return from cursor_get_pos */
	gfx_coord2_t cursor_get_pos_pos;
	/** True if cursor_set_pos was called */
	bool cursor_set_pos_called;
	/** Position passed to cursor_set_pos */
	gfx_coord2_t cursor_set_pos_pos;
	/** True if cursor_set_visible was called */
	bool cursor_set_visible_called;
	/** Value passed to cursor_set_visible */
	bool cursor_set_visible_vis;
} test_gc_t;

typedef struct {
	test_gc_t *gc;
	/** Cookie passed around for verification */
	uint32_t cookie;
} test_gc_bitmap_t;

/** Test creating and deleting a translation GC */
PCUT_TEST(create_delete)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test setting a clipping rectangle in translation GC */
PCUT_TEST(set_clip_rect)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	test_gc.rc = EOK;
	rc = gfx_set_clip_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_gc.set_clip_rect_called);
	PCUT_ASSERT_INT_EQUALS(11, test_gc.set_clip_rect_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(22, test_gc.set_clip_rect_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(13, test_gc.set_clip_rect_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(24, test_gc.set_clip_rect_rect.p1.y);

	test_gc.rc = EIO;
	rc = gfx_set_clip_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test setting color in translation GC */
PCUT_TEST(set_color)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_color_t *color;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	test_gc.rc = EOK;
	rc = gfx_set_color(xgc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_gc.set_color_called);
	PCUT_ASSERT_INT_EQUALS(1, test_gc.set_color_r);
	PCUT_ASSERT_INT_EQUALS(2, test_gc.set_color_g);
	PCUT_ASSERT_INT_EQUALS(3, test_gc.set_color_b);

	test_gc.rc = EIO;
	rc = gfx_set_color(xgc, color);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test filling a rectangle in translation GC */
PCUT_TEST(fill_rect)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	test_gc.rc = EOK;
	rc = gfx_fill_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_gc.fill_rect_called);
	PCUT_ASSERT_INT_EQUALS(11, test_gc.fill_rect_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(22, test_gc.fill_rect_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(13, test_gc.fill_rect_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(24, test_gc.fill_rect_rect.p1.y);

	test_gc.rc = EIO;
	rc = gfx_fill_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test updating a translation GC */
PCUT_TEST(update)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_rect_t rect;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	test_gc.rc = EOK;
	rc = gfx_set_clip_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_gc.set_clip_rect_called);
	PCUT_ASSERT_INT_EQUALS(11, test_gc.set_clip_rect_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(22, test_gc.set_clip_rect_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(13, test_gc.set_clip_rect_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(24, test_gc.set_clip_rect_rect.p1.y);

	test_gc.rc = EIO;
	rc = gfx_set_clip_rect(xgc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test creating bitmap in translating GC */
PCUT_TEST(bitmap_create)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t off;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	params.flags = bmpf_direct_output;
	params.key_color = 0x112233;

	test_gc.rc = EOK;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(test_gc.bitmap_create_called);
	PCUT_ASSERT_INT_EQUALS(1, test_gc.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, test_gc.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, test_gc.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, test_gc.bitmap_create_params.rect.p1.y);

	test_gc.rc = EOK;
	gfx_bitmap_destroy(bitmap);

	test_gc.rc = EIO;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test destroying bitmap in translating GC */
PCUT_TEST(bitmap_destroy)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t off;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	params.flags = bmpf_direct_output;
	params.key_color = 0x112233;

	test_gc.rc = EOK;
	test_gc.bitmap_create_cookie = 0x12345678;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(1, test_gc.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, test_gc.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, test_gc.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, test_gc.bitmap_create_params.rect.p1.y);
	PCUT_ASSERT_INT_EQUALS(bmpf_direct_output,
	    test_gc.bitmap_create_params.flags);
	PCUT_ASSERT_INT_EQUALS(0x112233,
	    test_gc.bitmap_create_params.key_color);

	test_gc.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.bitmap_destroy_called);
	PCUT_ASSERT_INT_EQUALS(0x12345678, test_gc.bitmap_destroy_cookie);

	test_gc.rc = EOK;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	test_gc.rc = EIO;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test rendering bitmap in translation GC */
PCUT_TEST(bitmap_render)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t srect;
	gfx_coord2_t off;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	test_gc.rc = EOK;
	test_gc.bitmap_create_cookie = 0x12345678;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	srect.p0.x = 5;
	srect.p0.y = 6;
	srect.p1.x = 7;
	srect.p1.y = 8;

	off.x = 100;
	off.y = 200;

	test_gc.rc = EOK;
	rc = gfx_bitmap_render(bitmap, &srect, &off);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.bitmap_render_called);
	PCUT_ASSERT_INT_EQUALS(0x12345678, test_gc.bitmap_render_cookie);
	PCUT_ASSERT_INT_EQUALS(5, test_gc.bitmap_render_srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(6, test_gc.bitmap_render_srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(7, test_gc.bitmap_render_srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, test_gc.bitmap_render_srect.p1.y);
	PCUT_ASSERT_INT_EQUALS(110, test_gc.bitmap_render_off.x);
	PCUT_ASSERT_INT_EQUALS(220, test_gc.bitmap_render_off.y);

	test_gc.rc = EIO;
	rc = gfx_bitmap_render(bitmap, &srect, &off);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	test_gc.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test getting bitmap allocation info in translation GC */
PCUT_TEST(bitmap_get_alloc)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_alloc_t galloc;
	gfx_coord2_t off;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	test_gc.rc = EOK;
	test_gc.bitmap_create_cookie = 0x12345678;
	rc = gfx_bitmap_create(xgc, &params, &alloc, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	test_gc.bitmap_get_alloc_alloc.pitch = 42;
	test_gc.bitmap_get_alloc_alloc.off0 = 43;
	test_gc.bitmap_get_alloc_alloc.pixels = malloc(1);
	PCUT_ASSERT_NOT_NULL(test_gc.bitmap_get_alloc_alloc.pixels);

	test_gc.rc = EOK;
	rc = gfx_bitmap_get_alloc(bitmap, &galloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.bitmap_get_alloc_called);
	PCUT_ASSERT_INT_EQUALS(42, galloc.pitch);
	PCUT_ASSERT_INT_EQUALS(43, galloc.off0);
	PCUT_ASSERT_EQUALS(test_gc.bitmap_get_alloc_alloc.pixels, galloc.pixels);

	test_gc.rc = EIO;
	rc = gfx_bitmap_get_alloc(bitmap, &galloc);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	test_gc.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	free(test_gc.bitmap_get_alloc_alloc.pixels);
	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test getting cursor position in translation GC */
PCUT_TEST(cursor_get_pos)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_coord2_t off;
	gfx_coord2_t gpos;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	test_gc.cursor_get_pos_pos.x = 13;
	test_gc.cursor_get_pos_pos.y = 24;
	test_gc.rc = EOK;
	rc = gfx_cursor_get_pos(xgc, &gpos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.cursor_get_pos_called);

	/*
	 * Note that translation GC performs inverse translation when getting
	 * cursor coordinates.
	 */
	PCUT_ASSERT_INT_EQUALS(3, gpos.x);
	PCUT_ASSERT_INT_EQUALS(4, gpos.y);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test setting cursor position in translation GC */
PCUT_TEST(cursor_set_pos)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_coord2_t off;
	gfx_coord2_t pos;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	pos.x = 3;
	pos.y = 4;

	test_gc.rc = EOK;
	rc = gfx_cursor_set_pos(xgc, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.cursor_set_pos_called);
	PCUT_ASSERT_INT_EQUALS(13, test_gc.cursor_set_pos_pos.x);
	PCUT_ASSERT_INT_EQUALS(24, test_gc.cursor_set_pos_pos.y);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Test setting cursor visibility in translation GC */
PCUT_TEST(cursor_set_visible)
{
	test_gc_t test_gc;
	gfx_context_t *tgc;
	xlate_gc_t *xlategc;
	gfx_context_t *xgc;
	gfx_coord2_t off;
	errno_t rc;

	rc = gfx_context_new(&testgc_ops, &test_gc, &tgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	off.x = 10;
	off.y = 20;
	rc = xlate_gc_create(&off, tgc, &xlategc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	xgc = xlate_gc_get_ctx(xlategc);

	memset(&test_gc, 0, sizeof(test_gc));

	test_gc.rc = EOK;
	rc = gfx_cursor_set_visible(xgc, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.cursor_set_visible_called);
	PCUT_ASSERT_TRUE(test_gc.cursor_set_visible_vis);

	memset(&test_gc, 0, sizeof(test_gc));

	test_gc.rc = EOK;
	rc = gfx_cursor_set_visible(xgc, false);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(test_gc.cursor_set_visible_called);
	PCUT_ASSERT_FALSE(test_gc.cursor_set_visible_vis);

	memset(&test_gc, 0, sizeof(test_gc));

	test_gc.rc = EIO;
	rc = gfx_cursor_set_visible(xgc, false);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);
	PCUT_ASSERT_TRUE(test_gc.cursor_set_visible_called);
	PCUT_ASSERT_FALSE(test_gc.cursor_set_visible_vis);

	xlate_gc_delete(xlategc);
	gfx_context_delete(tgc);
}

/** Set clipping rectangle in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param rect Rectangle
 * @return EOK on success or an error code
 */
static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->set_clip_rect_called = true;
	test_gc->set_clip_rect_rect = *rect;

	return test_gc->rc;
}

/** Set drawing color in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param color Color
 * @return EOK on success or an error code
 */
static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->set_color_called = true;
	gfx_color_get_rgb_i16(color, &test_gc->set_color_r,
	    &test_gc->set_color_g, &test_gc->set_color_b);

	return test_gc->rc;
}

/** Fill rectangle in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param rect Rectangle
 * @return EOK on success or an error code
 */
static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->fill_rect_called = true;
	test_gc->fill_rect_rect = *rect;

	return test_gc->rc;
}

/** Update test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @return EOK on success or an error code
 */
static errno_t testgc_update(void *arg)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->update_called = true;
	return test_gc->rc;
}

/** Create bitmap in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param params Bitmap parameters
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbitmap Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
static errno_t testgc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbitmap)
{
	test_gc_t *test_gc = (test_gc_t *)arg;
	test_gc_bitmap_t *tbitmap;

	test_gc->bitmap_create_called = true;
	if (params != NULL)
		test_gc->bitmap_create_params = *params;
	if (alloc != NULL)
		test_gc->bitmap_create_alloc = *alloc;

	if (test_gc->rc != EOK)
		return test_gc->rc;

	tbitmap = calloc(1, sizeof(test_gc_bitmap_t));
	if (tbitmap == NULL)
		return ENOMEM;

	tbitmap->gc = test_gc;
	tbitmap->cookie = test_gc->bitmap_create_cookie;
	*rbitmap = (void *)tbitmap;
	return EOK;
}

/** Destroy bitmap in test GC.
 *
 * @param bitmap Bitmap
 * @return EOK on success or an error code
 */
static errno_t testgc_bitmap_destroy(void *bitmap)
{
	test_gc_bitmap_t *tbitmap = (test_gc_bitmap_t *)bitmap;
	test_gc_t *test_gc = tbitmap->gc;

	test_gc->bitmap_destroy_called = true;
	test_gc->bitmap_destroy_cookie = tbitmap->cookie;
	return test_gc->rc;
}

/** Render bitmap in test GC.
 *
 * @param arg Bitmap
 * @param srect Source rectangle (or @c NULL)
 * @param off Offset (or @c NULL)
 * @return EOK on success or an error code
 */
static errno_t testgc_bitmap_render(void *bitmap, gfx_rect_t *srect,
    gfx_coord2_t *off)
{
	test_gc_bitmap_t *tbitmap = (test_gc_bitmap_t *)bitmap;
	test_gc_t *test_gc = tbitmap->gc;

	test_gc->bitmap_render_called = true;
	test_gc->bitmap_render_cookie = tbitmap->cookie;
	if (srect != NULL)
		test_gc->bitmap_render_srect = *srect;
	if (off != NULL)
		test_gc->bitmap_render_off = *off;
	return test_gc->rc;
}

/** Get bitmap allocation info in test GC.
 *
 * @param bitmap Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t testgc_bitmap_get_alloc(void *bitmap, gfx_bitmap_alloc_t *alloc)
{
	test_gc_bitmap_t *tbitmap = (test_gc_bitmap_t *)bitmap;
	test_gc_t *test_gc = tbitmap->gc;

	test_gc->bitmap_get_alloc_called = true;
	test_gc->bitmap_get_alloc_cookie = tbitmap->cookie;

	if (test_gc->rc != EOK)
		return test_gc->rc;

	*alloc = test_gc->bitmap_get_alloc_alloc;
	return EOK;
}

/** Get cursor position in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param pos Place to store cursor position
 * @return EOK on success or an error code
 */
static errno_t testgc_cursor_get_pos(void *arg, gfx_coord2_t *pos)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->cursor_get_pos_called = true;

	if (test_gc->rc != EOK)
		return test_gc->rc;

	*pos = test_gc->cursor_get_pos_pos;
	return EOK;
}

/** Set cursor position in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param pos Cursor position
 * @return EOK on success or an error code
 */
static errno_t testgc_cursor_set_pos(void *arg, gfx_coord2_t *pos)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->cursor_set_pos_called = true;
	test_gc->cursor_set_pos_pos = *pos;

	return test_gc->rc;
}

/** Set cursor visibility in test GC.
 *
 * @param arg Argument (test_gc_t *)
 * @param visible @c true iff cursor should be visible
 * @return EOK on success or an error code
 */
static errno_t testgc_cursor_set_visible(void *arg, bool visible)
{
	test_gc_t *test_gc = (test_gc_t *)arg;

	test_gc->cursor_set_visible_called = true;
	test_gc->cursor_set_visible_vis = visible;

	return test_gc->rc;
}

PCUT_EXPORT(xlategc);
