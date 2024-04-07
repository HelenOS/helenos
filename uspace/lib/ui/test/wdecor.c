/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include "../private/wdecor.h"

PCUT_INIT;

PCUT_TEST_SUITE(wdecor);

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

static void test_wdecor_sysmenu_open(ui_wdecor_t *, void *, sysarg_t);
static void test_wdecor_sysmenu_left(ui_wdecor_t *, void *, sysarg_t);
static void test_wdecor_sysmenu_right(ui_wdecor_t *, void *, sysarg_t);
static void test_wdecor_sysmenu_accel(ui_wdecor_t *, void *, char32_t,
    sysarg_t);
static void test_wdecor_minimize(ui_wdecor_t *, void *);
static void test_wdecor_maximize(ui_wdecor_t *, void *);
static void test_wdecor_unmaximize(ui_wdecor_t *, void *);
static void test_wdecor_close(ui_wdecor_t *, void *);
static void test_wdecor_move(ui_wdecor_t *, void *, gfx_coord2_t *, sysarg_t);
static void test_wdecor_resize(ui_wdecor_t *, void *, ui_wdecor_rsztype_t,
    gfx_coord2_t *, sysarg_t);
static void test_wdecor_set_cursor(ui_wdecor_t *, void *, ui_stock_cursor_t);

static ui_wdecor_cb_t test_wdecor_cb = {
	.sysmenu_open = test_wdecor_sysmenu_open,
	.sysmenu_left = test_wdecor_sysmenu_left,
	.sysmenu_right = test_wdecor_sysmenu_right,
	.sysmenu_accel = test_wdecor_sysmenu_accel,
	.minimize = test_wdecor_minimize,
	.maximize = test_wdecor_maximize,
	.unmaximize = test_wdecor_unmaximize,
	.close = test_wdecor_close,
	.move = test_wdecor_move,
	.resize = test_wdecor_resize,
	.set_cursor = test_wdecor_set_cursor
};

static ui_wdecor_cb_t dummy_wdecor_cb = {
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
	bool sysmenu_open;
	bool sysmenu_left;
	bool sysmenu_right;
	bool sysmenu_accel;
	bool minimize;
	bool maximize;
	bool unmaximize;
	bool close;
	bool move;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	sysarg_t idev_id;
	char32_t accel;
	bool resize;
	ui_wdecor_rsztype_t rsztype;
	bool set_cursor;
	ui_stock_cursor_t cursor;
} test_cb_resp_t;

/** Create and destroy window decoration */
PCUT_TEST(create_destroy)
{
	ui_wdecor_t *wdecor = NULL;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wdecor);

	ui_wdecor_destroy(wdecor);
}

/** ui_wdecor_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_wdecor_destroy(NULL);
}

/** Set window decoration rectangle sets internal field */
PCUT_TEST(set_rect)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_wdecor_set_rect(wdecor, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, wdecor->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, wdecor->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, wdecor->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, wdecor->rect.p1.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set window decoration active sets internal field */
PCUT_TEST(set_active)
{
	ui_wdecor_t *wdecor;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_set_active(wdecor, false);
	PCUT_ASSERT_FALSE(wdecor->active);

	ui_wdecor_set_active(wdecor, true);
	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_destroy(wdecor);
}

/** Set window decoration maximized sets internal field */
PCUT_TEST(set_maximized)
{
	ui_wdecor_t *wdecor;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_set_maximized(wdecor, false);
	PCUT_ASSERT_FALSE(wdecor->maximized);

	ui_wdecor_set_maximized(wdecor, true);
	PCUT_ASSERT_TRUE(wdecor->maximized);

	ui_wdecor_destroy(wdecor);
}

/** Setting system menu handle as active/inactive */
PCUT_TEST(sysmenu_hdl_set_active)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(wdecor->sysmenu_hdl_active);
	ui_wdecor_sysmenu_hdl_set_active(wdecor, true);
	PCUT_ASSERT_TRUE(wdecor->sysmenu_hdl_active);
	ui_wdecor_sysmenu_hdl_set_active(wdecor, false);
	PCUT_ASSERT_FALSE(wdecor->sysmenu_hdl_active);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint system menu handle */
PCUT_TEST(sysmenu_hdl_paint)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	ui_wdecor_geom_t geom;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wdecor_get_geom(wdecor, &geom);
	rc = ui_wdecor_sysmenu_hdl_paint(wdecor, &geom.sysmenu_hdl_rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint window decoration */
PCUT_TEST(paint)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_wdecor_paint(wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_wdecor_sysmenu_open() */
PCUT_TEST(sysmenu_open)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Sysmenu open callback with no callbacks set */
	ui_wdecor_sysmenu_open(wdecor, 42);

	/* Sysmenu open callback with sysmenu callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_sysmenu_open(wdecor, 42);

	/* Sysmenu open callback with real callback set */
	resp.sysmenu_open = false;
	resp.idev_id = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_sysmenu_open(wdecor, 42);
	PCUT_ASSERT_TRUE(resp.sysmenu_open);
	PCUT_ASSERT_INT_EQUALS(42, resp.idev_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_sysmenu_left() */
PCUT_TEST(sysmenu_left)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Sysmenu left callback with no callbacks set */
	ui_wdecor_sysmenu_left(wdecor, 42);

	/* Sysmenu left callback with sysmenu callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_sysmenu_left(wdecor, 42);

	/* Sysmenu left callback with real callback set */
	resp.sysmenu_left = false;
	resp.idev_id = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_sysmenu_left(wdecor, 42);
	PCUT_ASSERT_TRUE(resp.sysmenu_left);
	PCUT_ASSERT_INT_EQUALS(42, resp.idev_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_sysmenu_right() */
PCUT_TEST(sysmenu_right)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Sysmenu right callback with no callbacks set */
	ui_wdecor_sysmenu_right(wdecor, 42);

	/* Sysmenu right callback with sysmenu callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_sysmenu_right(wdecor, 42);

	/* Sysmenu right callback with real callback set */
	resp.sysmenu_right = false;
	resp.idev_id = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_sysmenu_right(wdecor, 42);
	PCUT_ASSERT_TRUE(resp.sysmenu_right);
	PCUT_ASSERT_INT_EQUALS(42, resp.idev_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_sysmenu_accel() */
PCUT_TEST(sysmenu_accel)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Sysmenu accelerator callback with no callbacks set */
	ui_wdecor_sysmenu_accel(wdecor, 'a', 42);

	/* Sysmenu accelerator callback with sysmenu callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_sysmenu_accel(wdecor, 'a', 42);

	/* Sysmenu accelerator callback with real callback set */
	resp.sysmenu_accel = false;
	resp.idev_id = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_sysmenu_accel(wdecor, 'a', 42);
	PCUT_ASSERT_TRUE(resp.sysmenu_accel);
	PCUT_ASSERT_INT_EQUALS('a', resp.accel);
	PCUT_ASSERT_INT_EQUALS(42, resp.idev_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_minimize() */
PCUT_TEST(minimize)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Minimize callback with no callbacks set */
	ui_wdecor_minimize(wdecor);

	/* Minimize callback with minimize callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_minimize(wdecor);

	/* Minimize callback with real callback set */
	resp.minimize = false;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_minimize(wdecor);
	PCUT_ASSERT_TRUE(resp.minimize);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_maximize() */
PCUT_TEST(maximize)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Maximize callback with no callbacks set */
	ui_wdecor_maximize(wdecor);

	/* Maxmimize callback with maximize callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_maximize(wdecor);

	/* Maximize callback with real callback set */
	resp.maximize = false;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_maximize(wdecor);
	PCUT_ASSERT_TRUE(resp.maximize);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_unmaximize() */
PCUT_TEST(unmaximize)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Unmaximize callback with no callbacks set */
	ui_wdecor_unmaximize(wdecor);

	/* Unmaximize callback with unmaximize callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_unmaximize(wdecor);

	/* Unmaximize callback with real callback set */
	resp.unmaximize = false;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_unmaximize(wdecor);
	PCUT_ASSERT_TRUE(resp.unmaximize);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_close() */
PCUT_TEST(close)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Close callback with no callbacks set */
	ui_wdecor_close(wdecor);

	/* Close callback with close callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_close(wdecor);

	/* Close callback with real callback set */
	resp.close = false;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_close(wdecor);
	PCUT_ASSERT_TRUE(resp.close);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_move() */
PCUT_TEST(move)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;
	gfx_coord2_t pos;
	sysarg_t pos_id;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 3;
	pos.y = 4;
	pos_id = 5;

	/* Move callback with no callbacks set */
	ui_wdecor_move(wdecor, &pos, pos_id);

	/* Move callback with move callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_move(wdecor, &pos, pos_id);

	/* Move callback with real callback set */
	resp.move = false;
	resp.pos.x = 0;
	resp.pos.y = 0;
	resp.pos_id = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_move(wdecor, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.move);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.pos_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_resize() */
PCUT_TEST(resize)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;
	ui_wdecor_rsztype_t rsztype;
	gfx_coord2_t pos;
	sysarg_t pos_id;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rsztype = ui_wr_bottom;
	pos.x = 3;
	pos.y = 4;
	pos_id = 5;

	/* Resize callback with no callbacks set */
	ui_wdecor_resize(wdecor, rsztype, &pos, pos_id);

	/* Resize callback with move callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_resize(wdecor, rsztype, &pos, pos_id);

	/* Resize callback with real callback set */
	resp.resize = false;
	resp.rsztype = ui_wr_none;
	resp.pos.x = 0;
	resp.pos.y = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_resize(wdecor, rsztype, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.resize);
	PCUT_ASSERT_INT_EQUALS(rsztype, resp.rsztype);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.pos_id);

	ui_wdecor_destroy(wdecor);
}

/** Test ui_wdecor_set_cursor() */
PCUT_TEST(set_cursor)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;
	ui_stock_cursor_t cursor;

	rc = ui_wdecor_create(NULL, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	cursor = ui_curs_size_uldr;

	/* Set cursor callback with no callbacks set */
	ui_wdecor_set_cursor(wdecor, cursor);

	/* Set cursor callback with move callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_set_cursor(wdecor, cursor);

	/* Set cursor callback with real callback set */
	resp.set_cursor = false;
	resp.cursor = ui_curs_arrow;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_set_cursor(wdecor, cursor);
	PCUT_ASSERT_TRUE(resp.set_cursor);
	PCUT_ASSERT_INT_EQUALS(cursor, resp.cursor);

	ui_wdecor_destroy(wdecor);
}

/** Clicking the close button generates close callback */
PCUT_TEST(close_btn_clicked)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	test_cb_resp_t resp;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	resp.close = false;

	ui_pbutton_clicked(wdecor->btn_close);
	PCUT_ASSERT_TRUE(resp.close);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Button press on title bar generates move callback */
PCUT_TEST(pos_event_move)
{
	errno_t rc;
	gfx_rect_t rect;
	pos_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	resp.move = false;
	resp.pos.x = 0;
	resp.pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 50;
	event.vpos = 25;
	ui_wdecor_pos_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.move);
	PCUT_ASSERT_INT_EQUALS(event.hpos, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(event.vpos, resp.pos.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Pressing F10 generates sysmenu event.
 *
 * Note that in a window with menu bar the menu bar would claim F10
 * so it would never be delivered to window decoration.
 */
PCUT_TEST(kbd_f10_sysmenu)
{
	errno_t rc;
	gfx_rect_t rect;
	kbd_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	resp.sysmenu_open = false;

	event.type = KEY_PRESS;
	event.mods = 0;
	event.key = KC_F10;
	event.kbd_id = 42;
	ui_wdecor_kbd_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.sysmenu_open);
	PCUT_ASSERT_INT_EQUALS(event.kbd_id, resp.idev_id);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Down key with active sysmenu handle generates sysmenu open event */
PCUT_TEST(kbd_down_sysmenu)
{
	errno_t rc;
	gfx_rect_t rect;
	kbd_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	ui_wdecor_sysmenu_hdl_set_active(wdecor, true);

	resp.sysmenu_open = false;

	event.type = KEY_PRESS;
	event.mods = 0;
	event.key = KC_DOWN;
	event.kbd_id = 42;
	ui_wdecor_kbd_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.sysmenu_open);
	PCUT_ASSERT_INT_EQUALS(event.kbd_id, resp.idev_id);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Left key with active sysmenu handle generates sysmenu left event */
PCUT_TEST(kbd_left_sysmenu)
{
	errno_t rc;
	gfx_rect_t rect;
	kbd_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	ui_wdecor_sysmenu_hdl_set_active(wdecor, true);

	resp.sysmenu_left = false;

	event.type = KEY_PRESS;
	event.mods = 0;
	event.key = KC_LEFT;
	event.kbd_id = 42;
	ui_wdecor_kbd_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.sysmenu_left);
	PCUT_ASSERT_INT_EQUALS(event.kbd_id, resp.idev_id);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Right key with active sysmenu handle generates sysmenu right event */
PCUT_TEST(kbd_right_sysmenu)
{
	errno_t rc;
	gfx_rect_t rect;
	kbd_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	ui_wdecor_sysmenu_hdl_set_active(wdecor, true);

	resp.sysmenu_right = false;

	event.type = KEY_PRESS;
	event.mods = 0;
	event.key = KC_RIGHT;
	event.kbd_id = 42;
	ui_wdecor_kbd_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.sysmenu_right);
	PCUT_ASSERT_INT_EQUALS(event.kbd_id, resp.idev_id);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Character key with active sysmenu handle generates sysmenu accel event */
PCUT_TEST(kbd_accel_sysmenu)
{
	errno_t rc;
	gfx_rect_t rect;
	kbd_event_t event;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	test_cb_resp_t resp;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	ui_wdecor_sysmenu_hdl_set_active(wdecor, true);

	resp.sysmenu_accel = false;

	event.type = KEY_PRESS;
	event.mods = 0;
	event.key = KC_A;
	event.c = 'a';
	event.kbd_id = 42;
	ui_wdecor_kbd_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.sysmenu_accel);
	PCUT_ASSERT_INT_EQUALS(event.c, resp.accel);
	PCUT_ASSERT_INT_EQUALS(event.kbd_id, resp.idev_id);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_wdecor_get_geom() with ui_wds_none produces the correct geometry */
PCUT_TEST(get_geom_none)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_geom_t geom;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);
	ui_wdecor_get_geom(wdecor, &geom);

	PCUT_ASSERT_INT_EQUALS(10, geom.interior_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20, geom.interior_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(100, geom.interior_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(200, geom.interior_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(10, geom.app_area_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20, geom.app_area_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(100, geom.app_area_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(200, geom.app_area_rect.p1.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_wdecor_get_geom() with ui_wds_frame produces the correct geometry */
PCUT_TEST(get_geom_frame)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_geom_t geom;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_frame, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);
	ui_wdecor_get_geom(wdecor, &geom);

	PCUT_ASSERT_INT_EQUALS(14, geom.interior_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.interior_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.interior_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.interior_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.title_bar_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.caption_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.app_area_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.app_area_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.app_area_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.app_area_rect.p1.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_wdecor_get_geom() with ui_wds_frame | ui_wds_titlebar */
PCUT_TEST(get_geom_frame_titlebar)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_geom_t geom;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_frame | ui_wds_titlebar,
	    &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);
	ui_wdecor_get_geom(wdecor, &geom);

	PCUT_ASSERT_INT_EQUALS(14, geom.interior_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.interior_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.interior_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.interior_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.title_bar_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.title_bar_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.title_bar_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.title_bar_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.sysmenu_hdl_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(18, geom.caption_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.caption_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(91, geom.caption_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.caption_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_min_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_close_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.app_area_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.app_area_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.app_area_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.app_area_rect.p1.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_wdecor_get_geom() with ui_wds_decorated produces the correct geometry */
PCUT_TEST(get_geom_decorated)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_geom_t geom;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_decorated, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);
	ui_wdecor_get_geom(wdecor, &geom);

	PCUT_ASSERT_INT_EQUALS(14, geom.interior_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.interior_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.interior_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.interior_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.title_bar_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.title_bar_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.title_bar_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.title_bar_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(15, geom.sysmenu_hdl_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(25, geom.sysmenu_hdl_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(35, geom.sysmenu_hdl_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(45, geom.sysmenu_hdl_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(38, geom.caption_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(24, geom.caption_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(51, geom.caption_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.caption_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(55, geom.btn_min_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(25, geom.btn_min_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(75, geom.btn_min_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(45, geom.btn_min_rect.p1.y);

	/* Maximize button is not in ui_wds_decorated */
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.btn_max_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(75, geom.btn_close_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(25, geom.btn_close_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(95, geom.btn_close_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(45, geom.btn_close_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.app_area_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.app_area_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.app_area_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.app_area_rect.p1.y);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_wdecor_rect_from_app() correctly converts application to window rect */
PCUT_TEST(rect_from_app)
{
	errno_t rc;
	ui_t *ui = NULL;
	gfx_rect_t arect;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	arect.p0.x = 14;
	arect.p0.y = 46;
	arect.p1.x = 96;
	arect.p1.y = 196;

	ui_wdecor_rect_from_app(ui, ui_wds_none, &arect, &rect);

	PCUT_ASSERT_INT_EQUALS(14, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(46, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, rect.p1.y);

	ui_wdecor_rect_from_app(ui, ui_wds_frame, &arect, &rect);

	PCUT_ASSERT_INT_EQUALS(10, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(42, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(100, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(200, rect.p1.y);

	ui_wdecor_rect_from_app(ui, ui_wds_decorated, &arect, &rect);

	PCUT_ASSERT_INT_EQUALS(10, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(100, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(200, rect.p1.y);

	ui_destroy(ui);
}

/** Test ui_wdecor_get_rsztype() */
PCUT_TEST(get_rsztype)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_rsztype_t rsztype;
	gfx_coord2_t pos;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_resizable, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	/* Outside of the window */
	pos.x = 0;
	pos.y = -1;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_none, rsztype);

	/* Middle of the window */
	pos.x = 50;
	pos.y = 100;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_none, rsztype);

	/* Top-left corner, but not on edge */
	pos.x = 20;
	pos.y = 30;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_none, rsztype);

	/* Top-left corner on top edge */
	pos.x = 20;
	pos.y = 20;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_top_left, rsztype);

	/* Top-left corner on left edge */
	pos.x = 10;
	pos.y = 30;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_top_left, rsztype);

	/* Top-right corner on top edge */
	pos.x = 90;
	pos.y = 20;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_top_right, rsztype);

	/* Top-right corner on right edge */
	pos.x = 99;
	pos.y = 30;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_top_right, rsztype);

	/* Top edge */
	pos.x = 50;
	pos.y = 20;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_top, rsztype);

	/* Bottom edge */
	pos.x = 50;
	pos.y = 199;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_bottom, rsztype);

	/* Left edge */
	pos.x = 10;
	pos.y = 100;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_left, rsztype);

	/* Right edge */
	pos.x = 99;
	pos.y = 100;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_right, rsztype);

	ui_wdecor_destroy(wdecor);

	/* Non-resizable window */

	rc = ui_wdecor_create(resource, "Hello", ui_wds_none, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	pos.x = 10;
	pos.y = 20;
	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	PCUT_ASSERT_EQUALS(ui_wr_none, rsztype);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_wdecor_cursor_from_rsztype() */
PCUT_TEST(cursor_from_rsztype)
{
	PCUT_ASSERT_EQUALS(ui_curs_arrow,
	    ui_wdecor_cursor_from_rsztype(ui_wr_none));
	PCUT_ASSERT_EQUALS(ui_curs_size_ud,
	    ui_wdecor_cursor_from_rsztype(ui_wr_top));
	PCUT_ASSERT_EQUALS(ui_curs_size_ud,
	    ui_wdecor_cursor_from_rsztype(ui_wr_bottom));
	PCUT_ASSERT_EQUALS(ui_curs_size_lr,
	    ui_wdecor_cursor_from_rsztype(ui_wr_left));
	PCUT_ASSERT_EQUALS(ui_curs_size_lr,
	    ui_wdecor_cursor_from_rsztype(ui_wr_right));
	PCUT_ASSERT_EQUALS(ui_curs_size_uldr,
	    ui_wdecor_cursor_from_rsztype(ui_wr_top_left));
	PCUT_ASSERT_EQUALS(ui_curs_size_uldr,
	    ui_wdecor_cursor_from_rsztype(ui_wr_bottom_right));
	PCUT_ASSERT_EQUALS(ui_curs_size_urdl,
	    ui_wdecor_cursor_from_rsztype(ui_wr_top_right));
	PCUT_ASSERT_EQUALS(ui_curs_size_urdl,
	    ui_wdecor_cursor_from_rsztype(ui_wr_bottom_left));
}

/** Test ui_wdecor_frame_pos_event() */
PCUT_TEST(frame_pos_event)
{
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	test_cb_resp_t resp;
	pos_event_t event;
	errno_t rc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", ui_wds_resizable, &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);

	/* Release on window border should do nothing */
	resp.resize = false;
	event.type = POS_RELEASE;
	event.hpos = 10;
	event.vpos = 10;
	ui_wdecor_frame_pos_event(wdecor, &event);
	PCUT_ASSERT_FALSE(resp.resize);

	/* Press in the middle of the window should do nothing */
	resp.resize = false;
	event.type = POS_PRESS;
	event.hpos = 50;
	event.vpos = 100;
	ui_wdecor_frame_pos_event(wdecor, &event);
	PCUT_ASSERT_FALSE(resp.resize);

	/* Press on window border should cause resize to be called */
	resp.resize = false;
	event.type = POS_PRESS;
	event.hpos = 10;
	event.vpos = 20;
	ui_wdecor_frame_pos_event(wdecor, &event);
	PCUT_ASSERT_TRUE(resp.resize);

	ui_wdecor_destroy(wdecor);
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

static void test_wdecor_sysmenu_open(ui_wdecor_t *wdecor, void *arg,
    sysarg_t idev_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->sysmenu_open = true;
	resp->idev_id = idev_id;
}

static void test_wdecor_sysmenu_left(ui_wdecor_t *wdecor, void *arg,
    sysarg_t idev_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->sysmenu_left = true;
	resp->idev_id = idev_id;
}

static void test_wdecor_sysmenu_right(ui_wdecor_t *wdecor, void *arg,
    sysarg_t idev_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->sysmenu_right = true;
	resp->idev_id = idev_id;
}

static void test_wdecor_sysmenu_accel(ui_wdecor_t *wdecor, void *arg,
    char32_t accel, sysarg_t idev_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->sysmenu_accel = true;
	resp->accel = accel;
	resp->idev_id = idev_id;
}

static void test_wdecor_minimize(ui_wdecor_t *wdecor, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->minimize = true;
}

static void test_wdecor_maximize(ui_wdecor_t *wdecor, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->maximize = true;
}

static void test_wdecor_unmaximize(ui_wdecor_t *wdecor, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->unmaximize = true;
}

static void test_wdecor_close(ui_wdecor_t *wdecor, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

static void test_wdecor_move(ui_wdecor_t *wdecor, void *arg, gfx_coord2_t *pos,
    sysarg_t pos_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->move = true;
	resp->pos = *pos;
	resp->pos_id = pos_id;
}

static void test_wdecor_resize(ui_wdecor_t *wdecor, void *arg,
    ui_wdecor_rsztype_t rsztype, gfx_coord2_t *pos, sysarg_t pos_id)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->resize = true;
	resp->rsztype = rsztype;
	resp->pos = *pos;
	resp->pos_id = pos_id;
}

static void test_wdecor_set_cursor(ui_wdecor_t *wdecor, void *arg,
    ui_stock_cursor_t cursor)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->set_cursor = true;
	resp->cursor = cursor;
}

PCUT_EXPORT(wdecor);
