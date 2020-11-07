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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include "../private/wdecor.h"

PCUT_INIT;

PCUT_TEST_SUITE(wdecor);

static void test_wdecor_close(ui_wdecor_t *, void *);
static void test_wdecor_move(ui_wdecor_t *, void *, gfx_coord2_t *);

static ui_wdecor_cb_t test_wdecor_cb = {
	.close = test_wdecor_close,
	.move = test_wdecor_move
};

static ui_wdecor_cb_t dummy_wdecor_cb = {
};

typedef struct {
	bool close;
	bool move;
	gfx_coord2_t pos;
} test_cb_resp_t;

/** Create and destroy button */
PCUT_TEST(create_destroy)
{
	ui_wdecor_t *wdecor = NULL;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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
}

/** Set window decoration active sets internal field */
PCUT_TEST(set_active)
{
	ui_wdecor_t *wdecor;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_set_active(wdecor, false);
	PCUT_ASSERT_FALSE(wdecor->active);

	ui_wdecor_set_active(wdecor, true);
	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_destroy(wdecor);
}

/** Paint button */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui;
	ui_wdecor_t *wdecor;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_wdecor_create(ui, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_wdecor_paint(wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wdecor_destroy(wdecor);
	ui_destroy(ui);
}

/** Test ui_wdecor_close() */
PCUT_TEST(close)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 3;
	pos.y = 4;

	/* Move callback with no callbacks set */
	ui_wdecor_move(wdecor, &pos);

	/* Move callback with move callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_move(wdecor, &pos);

	/* Move callback with real callback set */
	resp.move = false;
	resp.pos.x = 0;
	resp.pos.y = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_move(wdecor, &pos);
	PCUT_ASSERT_TRUE(resp.move);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.pos.y);

	ui_wdecor_destroy(wdecor);
}

/** Clicking the close button generates close callback */
PCUT_TEST(close_btn_clicked)
{
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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
}

/** Button press on title bar generates move callback */
PCUT_TEST(pos_event_move)
{
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	pos_event_t event;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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
}

/** ui_wdecor_get_geom() produces the correct geometry */
PCUT_TEST(get_geom)
{
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	ui_wdecor_geom_t geom;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
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

	PCUT_ASSERT_INT_EQUALS(75, geom.btn_close_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(25, geom.btn_close_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(95, geom.btn_close_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(45, geom.btn_close_rect.p1.y);

	PCUT_ASSERT_INT_EQUALS(14, geom.app_area_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(46, geom.app_area_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(96, geom.app_area_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(196, geom.app_area_rect.p1.y);

	ui_wdecor_destroy(wdecor);
}

/** ui_wdecor_rect_from_app() correctly converts application to window rect */
PCUT_TEST(rect_from_app)
{
	gfx_rect_t arect;
	gfx_rect_t rect;

	arect.p0.x = 14;
	arect.p0.y = 46;
	arect.p1.x = 96;
	arect.p1.y = 196;

	ui_wdecor_rect_from_app(&arect, &rect);

	PCUT_ASSERT_INT_EQUALS(10, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(100, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(200, rect.p1.y);
}

static void test_wdecor_close(ui_wdecor_t *wdecor, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

static void test_wdecor_move(ui_wdecor_t *wdecor, void *arg, gfx_coord2_t *pos)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->move = true;
	resp->pos = *pos;
}

PCUT_EXPORT(wdecor);
