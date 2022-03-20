/*
 * Copyright (c) 2022 Jiri Svoboda
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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/scrollbar.h>
#include <ui/resource.h>
#include "../private/pbutton.h"
#include "../private/scrollbar.h"

PCUT_INIT;

PCUT_TEST_SUITE(scrollbar);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_update(void *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.update = testgc_update,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

static void test_scrollbar_up(ui_scrollbar_t *, void *);
static void test_scrollbar_down(ui_scrollbar_t *, void *);
static void test_scrollbar_page_up(ui_scrollbar_t *, void *);
static void test_scrollbar_page_down(ui_scrollbar_t *, void *);
static void test_scrollbar_moved(ui_scrollbar_t *, void *, gfx_coord_t);

static ui_scrollbar_cb_t test_scrollbar_cb = {
	.up = test_scrollbar_up,
	.down = test_scrollbar_down,
	.page_up = test_scrollbar_page_up,
	.page_down = test_scrollbar_page_down,
	.moved = test_scrollbar_moved
};

static ui_scrollbar_cb_t dummy_scrollbar_cb = {
};

typedef struct {
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

typedef struct {
	bool up;
	bool down;
	bool page_up;
	bool page_down;
	bool moved;
	gfx_coord_t pos;
} test_cb_resp_t;

/** Create and destroy scrollbar */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar = NULL;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(scrollbar);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_scrollbar_destroy(NULL);
}

/** ui_scrollbar_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar = NULL;
	ui_control_t *control;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(scrollbar);

	control = ui_scrollbar_ctl(scrollbar);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set scrollbar rectangle sets internal field */
PCUT_TEST(set_rect)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar = NULL;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(scrollbar);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_scrollbar_set_rect(scrollbar, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, scrollbar->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, scrollbar->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, scrollbar->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, scrollbar->rect.p1.y);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint scrollbar in graphics mode */
PCUT_TEST(paint_gfx)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_scrollbar_paint_gfx(scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint scrollbar in text mode */
PCUT_TEST(paint_text)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, true, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 1;
	rect.p1.x = 10;
	rect.p1.y = 2;
	ui_scrollbar_set_rect(scrollbar, &rect);

	rc = ui_scrollbar_paint_text(scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_through_length() gives correct scrollbar through length */
PCUT_TEST(through_length)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t length;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	length = ui_scrollbar_through_length(scrollbar);

	/* Total length minus buttons */
	PCUT_ASSERT_INT_EQUALS(110 - 10 - 2 * 20, length);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_move_length() gives correct scrollbar move length */
PCUT_TEST(move_length)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t length;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	length = ui_scrollbar_move_length(scrollbar);

	/* Total length minus buttons minus default thumb length */
	PCUT_ASSERT_INT_EQUALS(110 - 10 - 2 * 20 - 20, length);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_get_pos() returns scrollbar position */
PCUT_TEST(get_pos)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t pos;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	scrollbar->pos = 42;
	pos = ui_scrollbar_get_pos(scrollbar);
	PCUT_ASSERT_INT_EQUALS(42, pos);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_set_thumb_length() sets thumb length */
PCUT_TEST(set_thumb_length)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	ui_scrollbar_set_thumb_length(scrollbar, 42);
	PCUT_ASSERT_INT_EQUALS(42, scrollbar->thumb_len);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_set_pos() sets thumb position */
PCUT_TEST(set_pos)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t pos;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	ui_scrollbar_set_pos(scrollbar, -1);
	pos = ui_scrollbar_get_pos(scrollbar);
	/* The value is clipped to the minimum possible position (0) */
	PCUT_ASSERT_INT_EQUALS(0, pos);

	ui_scrollbar_set_pos(scrollbar, 12);
	pos = ui_scrollbar_get_pos(scrollbar);
	/* The value is set to the requested value */
	PCUT_ASSERT_INT_EQUALS(12, pos);

	ui_scrollbar_set_pos(scrollbar, 42);
	pos = ui_scrollbar_get_pos(scrollbar);
	/* The value is clipped to the maximum possible position (40) */
	PCUT_ASSERT_INT_EQUALS(40, pos);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press and release scrollbar thumb */
PCUT_TEST(thumb_press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	resp.moved = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	pos.x = 11;
	pos.y = 22;

	ui_scrollbar_thumb_press(scrollbar, &pos);
	PCUT_ASSERT_TRUE(scrollbar->thumb_held);
	PCUT_ASSERT_FALSE(resp.moved);

	pos.x = 21;
	pos.y = 32;

	ui_scrollbar_release(scrollbar, &pos);
	PCUT_ASSERT_FALSE(scrollbar->thumb_held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(10, scrollbar->pos);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, update and release scrollbar */
PCUT_TEST(press_uodate_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	resp.moved = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	pos.x = 11;
	pos.y = 22;

	ui_scrollbar_thumb_press(scrollbar, &pos);
	PCUT_ASSERT_TRUE(scrollbar->thumb_held);
	PCUT_ASSERT_FALSE(resp.moved);

	pos.x = 21;
	pos.y = 32;

	ui_scrollbar_update(scrollbar, &pos);
	PCUT_ASSERT_TRUE(scrollbar->thumb_held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(10, scrollbar->pos);

	pos.x = 31;
	pos.y = 42;

	ui_scrollbar_release(scrollbar, &pos);
	PCUT_ASSERT_FALSE(scrollbar->thumb_held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(20, scrollbar->pos);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_up() delivers up event */
PCUT_TEST(up)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Up with no callbacks set */
	ui_scrollbar_up(scrollbar);

	/* Up with callback not implementing up */
	ui_scrollbar_set_cb(scrollbar, &dummy_scrollbar_cb, NULL);
	ui_scrollbar_up(scrollbar);

	/* Up with real callback set */
	resp.up = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);
	ui_scrollbar_up(scrollbar);
	PCUT_ASSERT_TRUE(resp.up);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_down() delivers down event */
PCUT_TEST(down)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Down with no callbacks set */
	ui_scrollbar_down(scrollbar);

	/* Down with callback not implementing down */
	ui_scrollbar_set_cb(scrollbar, &dummy_scrollbar_cb, NULL);
	ui_scrollbar_down(scrollbar);

	/* Down with real callback set */
	resp.down = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);
	ui_scrollbar_down(scrollbar);
	PCUT_ASSERT_TRUE(resp.down);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_page_up() delivers page up event */
PCUT_TEST(page_up)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Page up with no callbacks set */
	ui_scrollbar_page_up(scrollbar);

	/* Pge up with callback not implementing page up */
	ui_scrollbar_set_cb(scrollbar, &dummy_scrollbar_cb, NULL);
	ui_scrollbar_page_up(scrollbar);

	/* Page up with real callback set */
	resp.page_up = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);
	ui_scrollbar_page_up(scrollbar);
	PCUT_ASSERT_TRUE(resp.page_up);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_page_down() delivers page down event */
PCUT_TEST(page_down)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Page down with no callbacks set */
	ui_scrollbar_page_down(scrollbar);

	/* Page down with callback not implementing page down */
	ui_scrollbar_set_cb(scrollbar, &dummy_scrollbar_cb, NULL);
	ui_scrollbar_page_down(scrollbar);

	/* Page down with real callback set */
	resp.page_down = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);
	ui_scrollbar_page_down(scrollbar);
	PCUT_ASSERT_TRUE(resp.page_down);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_moved() delivers moved event */
PCUT_TEST(moved)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Moved with no callbacks set */
	ui_scrollbar_moved(scrollbar, 42);

	/* Moved with callback not implementing moved */
	ui_scrollbar_set_cb(scrollbar, &dummy_scrollbar_cb, NULL);
	ui_scrollbar_moved(scrollbar, 42);

	/* Moved with real callback set */
	resp.moved = false;
	resp.pos = 0;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);
	ui_scrollbar_moved(scrollbar, 42);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(42, resp.pos);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_pos_event() detects thumb press/release */
PCUT_TEST(pos_event_press_release_thumb)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 20;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	/* Press outside is not claimed and does nothing */
	event.type = POS_PRESS;
	event.hpos = 1;
	event.vpos = 2;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_FALSE(scrollbar->thumb_held);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	/* Press inside thumb is claimed and depresses it */
	event.type = POS_PRESS;
	event.hpos = 50;
	event.vpos = 20;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_TRUE(scrollbar->thumb_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases thumb */
	event.type = POS_RELEASE;
	event.hpos = 41;
	event.vpos = 32;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_FALSE(scrollbar->thumb_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_pos_event() detects up button press/release */
PCUT_TEST(pos_event_press_release_up_btn)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 20;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	/* Press inside up button is claimed and depresses it */
	event.type = POS_PRESS;
	event.hpos = 30;
	event.vpos = 20;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_TRUE(scrollbar->btn_up->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_scrollbar_pos_event() detects down button press/release */
PCUT_TEST(pos_event_press_relese_down_btn)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_scrollbar_create(resource, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 20;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	/* Press inside down button is claimed and depresses it */
	event.type = POS_PRESS;
	event.hpos = 90;
	event.vpos = 20;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_TRUE(scrollbar->btn_down->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	(void) arg;
	(void) color;
	return EOK;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

static errno_t testgc_update(void *arg)
{
	(void) arg;
	return EOK;
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
	if (srect != NULL)
		tbm->tgc->bm_srect = *srect;
	if (offs != NULL)
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

static void test_scrollbar_up(ui_scrollbar_t *scrollbar, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->up = true;
}

static void test_scrollbar_down(ui_scrollbar_t *scrollbar, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->down = true;
}

static void test_scrollbar_page_up(ui_scrollbar_t *scrollbar, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->page_up = true;
}

static void test_scrollbar_page_down(ui_scrollbar_t *scrollbar, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->page_down = true;
}

static void test_scrollbar_moved(ui_scrollbar_t *scrollbar, void *arg, gfx_coord_t pos)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->moved = true;
	resp->pos = pos;
}

PCUT_EXPORT(scrollbar);
