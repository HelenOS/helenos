/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/pbutton.h"
#include "../private/scrollbar.h"

PCUT_INIT;

PCUT_TEST_SUITE(scrollbar);

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
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(scrollbar);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_scrollbar_destroy(NULL);
}

/** ui_scrollbar_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar = NULL;
	ui_control_t *control;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(scrollbar);

	control = ui_scrollbar_ctl(scrollbar);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Set scrollbar rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar = NULL;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint scrollbar in graphics mode */
PCUT_TEST(paint_gfx)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_scrollbar_paint_gfx(scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint scrollbar in text mode */
PCUT_TEST(paint_text_horiz)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 1;
	rect.p1.x = 10;
	rect.p1.y = 2;
	ui_scrollbar_set_rect(scrollbar, &rect);

	rc = ui_scrollbar_paint_text(scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_get_geom() returns scrollbar geometry */
PCUT_TEST(get_geom)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_scrollbar_geom_t geom;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	ui_scrollbar_get_geom(scrollbar, &geom);
	PCUT_ASSERT_INT_EQUALS(11, geom.up_btn_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(11, geom.up_btn_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(99, geom.down_btn_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(29, geom.down_btn_rect.p1.y);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_trough_length() gives correct scrollbar trough length */
PCUT_TEST(trough_length)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t length;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	length = ui_scrollbar_trough_length(scrollbar);

	/* Total length minus buttons */
	PCUT_ASSERT_INT_EQUALS(110 - 10 - 2 * 21, length);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_move_length() gives correct scrollbar move length */
PCUT_TEST(move_length)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t length;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->thumb_held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	length = ui_scrollbar_move_length(scrollbar);

	/* Total length minus buttons minus default thumb length */
	PCUT_ASSERT_INT_EQUALS(110 - 10 - 2 * 21 - 21, length);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_get_pos() returns scrollbar position */
PCUT_TEST(get_pos)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t pos;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_set_thumb_length() sets thumb length */
PCUT_TEST(set_thumb_length)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_set_pos() sets thumb position */
PCUT_TEST(set_pos)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	gfx_coord_t pos;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	/* The value is clipped to the maximum possible position (37) */
	PCUT_ASSERT_INT_EQUALS(37, pos);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press and release scrollbar thumb */
PCUT_TEST(thumb_press_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press, update and release scrollbar */
PCUT_TEST(thumb_press_update_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press and release upper trough */
PCUT_TEST(upper_trough_press_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	resp.page_up = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);

	PCUT_ASSERT_FALSE(scrollbar->upper_trough_held);

	ui_scrollbar_upper_trough_press(scrollbar);
	PCUT_ASSERT_TRUE(scrollbar->upper_trough_held);
	PCUT_ASSERT_TRUE(resp.page_up);

	/* Position does not matter here */
	pos.x = 11;
	pos.y = 22;

	ui_scrollbar_release(scrollbar, &pos);
	PCUT_ASSERT_FALSE(scrollbar->upper_trough_held);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press and release lower trough */
PCUT_TEST(lower_trough_press_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	resp.page_down = false;
	ui_scrollbar_set_cb(scrollbar, &test_scrollbar_cb, &resp);

	PCUT_ASSERT_FALSE(scrollbar->lower_trough_held);

	ui_scrollbar_lower_trough_press(scrollbar);
	PCUT_ASSERT_TRUE(scrollbar->lower_trough_held);
	PCUT_ASSERT_TRUE(resp.page_down);

	/* Position does not matter here */
	pos.x = 11;
	pos.y = 22;

	ui_scrollbar_release(scrollbar, &pos);
	PCUT_ASSERT_FALSE(scrollbar->lower_trough_held);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Updating state of troughs when cursor or thumb moves */
PCUT_TEST(troughs_update)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_scrollbar_t *scrollbar;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_scrollbar_set_rect(scrollbar, &rect);

	PCUT_ASSERT_FALSE(scrollbar->lower_trough_inside);

	pos.x = 60;
	pos.y = 22;

	ui_scrollbar_troughs_update(scrollbar, &pos);
	PCUT_ASSERT_TRUE(scrollbar->lower_trough_inside);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_up() delivers up event */
PCUT_TEST(up)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_down() delivers down event */
PCUT_TEST(down)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_page_up() delivers page up event */
PCUT_TEST(page_up)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_page_down() delivers page down event */
PCUT_TEST(page_down)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_moved() delivers moved event */
PCUT_TEST(moved)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_pos_event() detects thumb press/release */
PCUT_TEST(pos_event_press_release_thumb)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_pos_event() detects up button press/release */
PCUT_TEST(pos_event_press_release_up_btn)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	PCUT_ASSERT_TRUE(scrollbar->up_btn->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_pos_event() detects upper trough press/release */
PCUT_TEST(pos_event_press_release_upper_trough)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->upper_trough_held);

	rect.p0.x = 20;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	/* Need to move thumb so that upper trough can be accessed */
	ui_scrollbar_set_pos(scrollbar, 42);

	/* Press inside upper trough is claimed and depresses it */
	event.type = POS_PRESS;
	event.hpos = 50;
	event.vpos = 20;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_TRUE(scrollbar->upper_trough_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases upper trough */
	event.type = POS_RELEASE;
	event.hpos = 41;
	event.vpos = 32;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_FALSE(scrollbar->upper_trough_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_pos_event() detects lower trough press/release */
PCUT_TEST(pos_event_press_release_lower_trough)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(scrollbar->upper_trough_held);

	rect.p0.x = 20;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 30;
	ui_scrollbar_set_rect(scrollbar, &rect);

	/* Press inside lower trough is claimed and depresses it */
	event.type = POS_PRESS;
	event.hpos = 70;
	event.vpos = 20;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_TRUE(scrollbar->lower_trough_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases upper trough */
	event.type = POS_RELEASE;
	event.hpos = 41;
	event.vpos = 32;
	claim = ui_scrollbar_pos_event(scrollbar, &event);
	PCUT_ASSERT_FALSE(scrollbar->lower_trough_held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_scrollbar_pos_event() detects down button press/release */
PCUT_TEST(pos_event_press_relese_down_btn)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_scrollbar_t *scrollbar;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_scrollbar_create(ui, window, ui_sbd_horiz, &scrollbar);
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
	PCUT_ASSERT_TRUE(scrollbar->down_btn->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_scrollbar_destroy(scrollbar);
	ui_window_destroy(window);
	ui_destroy(ui);
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
