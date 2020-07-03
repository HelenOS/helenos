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

#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/pixelmap.h>
#include <mem.h>
#include <memgfx/memgc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(memgfx);

static void test_update_rect(void *arg, gfx_rect_t *rect);

typedef struct {
	/** True if update was called */
	bool update_called;
	/** Update rectangle */
	gfx_rect_t rect;
} test_update_t;

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
	test_update_t update;
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

	rc = mem_gc_create(&rect, &alloc, test_update_rect, &update, &mgc);
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

	memset(&update, 0, sizeof(update));

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

	/* Check that the update rect is equal to the filled rect */
	PCUT_ASSERT_TRUE(update.update_called);
	PCUT_ASSERT_INT_EQUALS(frect.p0.x, update.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(frect.p0.y, update.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(frect.p1.x, update.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(frect.p1.y, update.rect.p1.y);

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
	test_update_t update;
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

	rc = mem_gc_create(&rect, &alloc, test_update_rect, &update, &mgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = mem_gc_get_ctx(mgc);
	PCUT_ASSERT_NOT_NULL(gc);

	/* Create bitmap */

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

	memset(&update, 0, sizeof(update));

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

	/* Check that the update rect is equal to the filled rect */
	PCUT_ASSERT_TRUE(update.update_called);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.x, update.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p0.y, update.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.x, update.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(params.rect.p1.y, update.rect.p1.y);

	/* TODO: Check clipping once memgc can support pitch != width etc. */

	mem_gc_delete(mgc);
	free(alloc.pixels);
}

/** Called by memory GC when a rectangle is updated. */
static void test_update_rect(void *arg, gfx_rect_t *rect)
{
	test_update_t *update = (test_update_t *)arg;

	update->update_called = true;
	update->rect = *rect;
}

PCUT_EXPORT(memgfx);
