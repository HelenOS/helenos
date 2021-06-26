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
#include <io/pixelmap.h>
#include <mem.h>
#include <memgfx/memgc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(memgfx);

static void test_invalidate_rect(void *arg, gfx_rect_t *rect);
static void test_update(void *arg);
static errno_t test_cursor_get_pos(void *arg, gfx_coord2_t *);
static errno_t test_cursor_set_pos(void *arg, gfx_coord2_t *);
static errno_t test_cursor_set_visible(void *arg, bool);

static mem_gc_cb_t test_mem_gc_cb = {
	.invalidate = test_invalidate_rect,
	.update = test_update,
	.cursor_get_pos = test_cursor_get_pos,
	.cursor_set_pos = test_cursor_set_pos,
	.cursor_set_visible = test_cursor_set_visible
};

typedef struct {
	/** Return code to return */
	errno_t rc;
	/** True if invalidate was called */
	bool invalidate_called;
	/** Invalidate rectangle */
	gfx_rect_t inv_rect;
	/** True if update was called */
	bool update_called;
	/** True if cursor_get_pos was called */
	bool cursor_get_pos_called;
	/** Position to return from cursor_get_pos */
	gfx_coord2_t get_pos_pos;
	/** True if cursor_set_pos was called */
	bool cursor_set_pos_called;
	/** Position passed to cursor_set_pos */
	gfx_coord2_t set_pos_pos;
	/** True if cursor_set_visible was called */
	bool cursor_set_visible_called;
	/** Value passed to cursor_set_visible */
	bool set_visible_vis;
} test_resp_t;

/** Test creating and deleting a memory GC */
PCUT_TEST(create_delete)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, NULL, NULL, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test filling a rectangle in memory GC */
PCUT_TEST(fill_rect)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_rect_t frect;
	gfx_bitmap_alloc_t alloc;
	gfx_context_t *gc;
	gfx_color_t *color;
	gfx_coord2_t pos;
	pixelmap_t pixelmap;
	pixel_t pixel;
	pixel_t expected;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Fill a rectangle */

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	frect.p0.x = 2;
	frect.p0.y = 2;
	frect.p1.x = 5;
	frect.p1.y = 5;

	memset(&resp, 0, sizeof(resp));

	rc = gfx_fill_rect(gc, &frect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pixelmap.width = rect.p1.x - rect.p0.x;
	pixelmap.height = rect.p1.y - rect.p0.y;
	pixelmap.data = alloc.pixels;

	/* Check that the pixels of the rectangle are set and no others are */
	for (pos.y = rect.p0.y; pos.y < rect.p1.y; pos.y++) {
		for (pos.x = rect.p0.x; pos.x < rect.p1.x; pos.x++) {
			pixel = pixelmap_get_pixel(&pixelmap, pos.x, pos.y);
			expected = gfx_pix_inside_rect(&pos, &frect) ?
			    PIXEL(0, 255, 255, 0) : PIXEL(0, 0, 0, 0);
			PCUT_ASSERT_INT_EQUALS(expected, pixel);
		}
	}

	/* Check that the invalidate rect is equal to the filled rect */
	PCUT_ASSERT_TRUE(resp.invalidate_called);
	PCUT_ASSERT_INT_EQUALS(frect.p0.x, resp.inv_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(frect.p0.y, resp.inv_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(frect.p1.x, resp.inv_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(frect.p1.y, resp.inv_rect.p1.y);

	/* TODO: Check clipping once memgc can support pitch != width etc. */

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test rendering a bitmap in memory GC */
PCUT_TEST(bitmap_render)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	gfx_context_t *gc;
	gfx_coord2_t pos;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t balloc;
	gfx_bitmap_t *bitmap;
	pixelmap_t bpmap;
	pixelmap_t dpmap;
	pixel_t pixel;
	pixel_t expected;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Create bitmap */

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 6;
	params.rect.p1.y = 6;

	/* TODO Test client allocation */
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_bitmap_get_alloc(bitmap, &balloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	bpmap.width = params.rect.p1.x - params.rect.p0.x;
	bpmap.height = params.rect.p1.y - params.rect.p0.y;
	bpmap.data = balloc.pixels;

	/* Fill bitmap pixels with constant color */
	for (pos.y = params.rect.p0.y; pos.y < params.rect.p1.y; pos.y++) {
		for (pos.x =  params.rect.p0.x; pos.x <  params.rect.p1.x; pos.x++) {
			pixelmap_put_pixel(&bpmap, pos.x, pos.y,
			    PIXEL(0, 255, 255, 0));
		}
	}

	dpmap.width = rect.p1.x - rect.p0.x;
	dpmap.height = rect.p1.y - rect.p0.y;
	dpmap.data = alloc.pixels;

	memset(&resp, 0, sizeof(resp));

	/* Render the bitmap */
	/* TODO Test rendering sub-rectangle */
	/* TODO Test rendering with offset */
	rc = gfx_bitmap_render(bitmap, NULL, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check that the pixels of the rectangle are set and no others are */
	for (pos.y = rect.p0.y; pos.y < rect.p1.y; pos.y++) {
		for (pos.x = rect.p0.x; pos.x < rect.p1.x; pos.x++) {
			pixel = pixelmap_get_pixel(&dpmap, pos.x, pos.y);
			expected = gfx_pix_inside_rect(&pos, &params.rect) ?
			    PIXEL(0, 255, 255, 0) : PIXEL(0, 0, 0, 0);
			PCUT_ASSERT_INT_EQUALS(expected, pixel);
		}
	}

	/* Check that the invalidate rect is equal to the filled rect */
	PCUT_ASSERT_TRUE(resp.invalidate_called);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, resp.inv_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, resp.inv_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, resp.inv_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, resp.inv_rect.p1.y);

	/* TODO: Check clipping once memgc can support pitch != width etc. */

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test gfx_update() on a memory GC */
PCUT_TEST(gfx_update)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	gfx_context_t *gc;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	memset(&resp, 0, sizeof(resp));
	PCUT_ASSERT_FALSE(resp.update_called);

	gfx_update(gc);
	PCUT_ASSERT_TRUE(resp.update_called);

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test gfx_cursor_get_pos() on a memory GC */
PCUT_TEST(gfx_cursor_get_pos)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t pos;
	gfx_context_t *gc;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	memset(&resp, 0, sizeof(resp));
	resp.rc = EOK;
	resp.get_pos_pos.x = 1;
	resp.get_pos_pos.y = 2;
	PCUT_ASSERT_FALSE(resp.cursor_get_pos_called);

	rc = gfx_cursor_get_pos(gc, &pos);
	PCUT_ASSERT_TRUE(resp.cursor_get_pos_called);
	PCUT_ASSERT_INT_EQUALS(resp.get_pos_pos.x, pos.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_pos_pos.y, pos.y);

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test gfx_cursor_set_pos() on a memory GC */
PCUT_TEST(gfx_cursor_set_pos)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t pos;
	gfx_context_t *gc;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	memset(&resp, 0, sizeof(resp));
	resp.rc = EOK;
	pos.x = 1;
	pos.y = 2;
	PCUT_ASSERT_FALSE(resp.cursor_set_pos_called);

	rc = gfx_cursor_set_pos(gc, &pos);
	PCUT_ASSERT_TRUE(resp.cursor_set_pos_called);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.set_pos_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.set_pos_pos.y);

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Test gfx_cursor_set_visible() on a memory GC */
PCUT_TEST(gfx_cursor_set_visible)
{
	mem_gc_t *mgc;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	gfx_context_t *gc;
	test_resp_t resp;
	errno_t rc;

	/* Bounding rectangle for memory GC */
	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 10;

	alloc.pitch = (rect.p1.x - rect.p0.x) * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = calloc(1, alloc.pitch * (rect.p1.y - rect.p0.y));
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	rc = mem_gc_create(&rect, &alloc, &test_mem_gc_cb, &resp, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	memset(&resp, 0, sizeof(resp));
	resp.rc = EOK;
	PCUT_ASSERT_FALSE(resp.cursor_set_visible_called);

	rc = gfx_cursor_set_visible(gc, true);
	PCUT_ASSERT_TRUE(resp.cursor_set_visible_called);
	PCUT_ASSERT_TRUE(resp.set_visible_vis);

	resp.cursor_set_visible_called = false;

	rc = gfx_cursor_set_visible(gc, false);
	PCUT_ASSERT_TRUE(resp.cursor_set_visible_called);
	PCUT_ASSERT_FALSE(resp.set_visible_vis);

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Called by memory GC when a rectangle is modified. */
static void test_invalidate_rect(void *arg, gfx_rect_t *rect)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->invalidate_called = true;
	resp->inv_rect = *rect;
}

/** Called by memory GC when update is called. */
static void test_update(void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->update_called = true;
}

/** Called by memory GC when cursor_get_pos is called. */
static errno_t test_cursor_get_pos(void *arg, gfx_coord2_t *pos)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->cursor_get_pos_called = true;
	*pos = resp->get_pos_pos;
	return resp->rc;
}

/** Called by memory GC when cursor_set_pos is called. */
static errno_t test_cursor_set_pos(void *arg, gfx_coord2_t *pos)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->cursor_set_pos_called = true;
	resp->set_pos_pos = *pos;
	return resp->rc;
}

/** Called by memory GC when cursor_set_visible is called. */
static errno_t test_cursor_set_visible(void *arg, bool visible)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->cursor_set_visible_called = true;
	resp->set_visible_vis = visible;
	return resp->rc;
}

PCUT_EXPORT(memgfx);
