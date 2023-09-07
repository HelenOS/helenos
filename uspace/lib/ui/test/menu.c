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
#include <str.h>
#include <ui/control.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/menu.h"
#include "../private/menubar.h"

PCUT_INIT;

PCUT_TEST_SUITE(menu);

typedef struct {
	bool left_called;
	bool right_called;
	bool close_req_called;
	bool press_accel_called;
	ui_menu_t *menu;
	sysarg_t idev_id;
	char32_t c;
} test_resp_t;

static void testmenu_left(ui_menu_t *, void *, sysarg_t);
static void testmenu_right(ui_menu_t *, void *, sysarg_t);
static void testmenu_close_req(ui_menu_t *, void *);
static void testmenu_press_accel(ui_menu_t *, void *, char32_t, sysarg_t);

ui_menu_cb_t testmenu_cb = {
	.left = testmenu_left,
	.right = testmenu_right,
	.close_req = testmenu_close_req,
	.press_accel = testmenu_press_accel
};

ui_menu_cb_t dummy_cb = {
};

/** Create and destroy menu */
PCUT_TEST(create_destroy)
{
	ui_menu_t *menu = NULL;
	errno_t rc;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_destroy(menu);
}

/** ui_menu_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_destroy(NULL);
}

/** ui_menu_set_cb() sets the internal fields */
PCUT_TEST(set_cb)
{
	ui_menu_t *menu = NULL;
	ui_menu_cb_t cb;
	int obj;
	errno_t rc;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_set_cb(menu, &cb, (void *)&obj);
	PCUT_ASSERT_EQUALS(&cb, menu->cb);
	PCUT_ASSERT_EQUALS((void *)&obj, menu->arg);

	ui_menu_destroy(menu);
}

/** Computing menu geometry */
PCUT_TEST(get_geom)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	ui_menu_geom_t geom;
	gfx_coord2_t pos;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	pos.x = 0;
	pos.y = 0;
	ui_menu_get_geom(menu, &pos, &geom);

	PCUT_ASSERT_INT_EQUALS(0, geom.outer_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.outer_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(16, geom.outer_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, geom.outer_rect.p1.y);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(12, geom.entries_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p1.y);

	ui_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_get_res() gets the menu's resource */
PCUT_TEST(get_res)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	ui_resource_t *res;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* The menu must be open first */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	res = ui_menu_get_res(menu);
	PCUT_ASSERT_NOT_NULL(res);

	ui_menu_destroy(menu);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Open and close menu with ui_menu_open() / ui_menu_close() */
PCUT_TEST(open_close)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Open and close */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_close(menu);

	ui_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_is_open() correctly returns menu state */
PCUT_TEST(is_open)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	bool open;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_FALSE(open);

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_TRUE(open);

	ui_menu_close(menu);

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_FALSE(open);

	ui_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint background in graphics mode */
PCUT_TEST(paint_bg_gfx)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	gfx_coord2_t pos;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint_bg_gfx(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint background in text mode */
PCUT_TEST(paint_bg_text)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	gfx_coord2_t pos;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint_bg_text(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint menu */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	gfx_coord2_t pos;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_pos_event() inside menu is claimed */
PCUT_TEST(pos_event_inside)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	ui_evclaim_t claimed;
	gfx_coord2_t pos;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	pos.x = 0;
	pos.y = 0;
	event.type = POS_PRESS;
	event.hpos = 0;
	event.vpos = 0;
	claimed = ui_menu_pos_event(menu, &pos, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_up() with empty menu does nothing */
PCUT_TEST(up_empty)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_up(menu);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_up() moves one entry up, skips separators, wraps around */
PCUT_TEST(up)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry1 = NULL;
	ui_menu_entry_t *mentry2 = NULL;
	ui_menu_entry_t *mentry3 = NULL;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry1);

	rc = ui_menu_entry_sep_create(menu, &mentry2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry2);

	rc = ui_menu_entry_create(menu, "Bar", "F2", &mentry3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry3);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* When menu is open, the first entry is selected */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_up(menu);

	/* Now we've wrapped around to the last entry */
	PCUT_ASSERT_EQUALS(mentry3, menu->selected);

	ui_menu_up(menu);

	/* mentry2 is a separator and was skipped */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_down() with empty menu does nothing */
PCUT_TEST(down_empty)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_down(menu);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_down() moves one entry down, skips separators, wraps around */
PCUT_TEST(down)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry1 = NULL;
	ui_menu_entry_t *mentry2 = NULL;
	ui_menu_entry_t *mentry3 = NULL;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry1);

	rc = ui_menu_entry_sep_create(menu, &mentry2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry2);

	rc = ui_menu_entry_create(menu, "Bar", "F2", &mentry3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry3);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* When menu is open, the first entry is selected */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_down(menu);

	/* mentry2 is a separator and was skipped */
	PCUT_ASSERT_EQUALS(mentry3, menu->selected);

	ui_menu_up(menu);

	/* Now we've wrapped around to the first entry */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Sending an unhandled event does nothing. */
PCUT_TEST(send_unhandled)
{
	ui_menu_t *menu = NULL;
	errno_t rc;
	sysarg_t idev_id;
	char32_t c;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	/* Send events without setting callback */
	c = 'A';
	idev_id = 42;
	ui_menu_left(menu, idev_id);
	ui_menu_right(menu, idev_id);
	ui_menu_close_req(menu);
	ui_menu_press_accel(menu, c, idev_id);

	/* Set dummy callback structure */
	ui_menu_set_cb(menu, &dummy_cb, NULL);

	/* Send unhandled events */
	ui_menu_left(menu, idev_id);
	ui_menu_right(menu, idev_id);
	ui_menu_close_req(menu);
	ui_menu_press_accel(menu, c, idev_id);

	ui_menu_destroy(menu);
}

/** ui_menu_left() sends left event */
PCUT_TEST(left)
{
	ui_menu_t *menu = NULL;
	errno_t rc;
	test_resp_t resp;
	sysarg_t idev_id;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_set_cb(menu, &testmenu_cb, (void *)&resp);

	memset(&resp, 0, sizeof(resp));
	PCUT_ASSERT_FALSE(resp.left_called);

	idev_id = 42;
	ui_menu_left(menu, idev_id);

	PCUT_ASSERT_TRUE(resp.left_called);
	PCUT_ASSERT_EQUALS(menu, resp.menu);
	PCUT_ASSERT_INT_EQUALS(idev_id, resp.idev_id);

	ui_menu_destroy(menu);
}

/** ui_menu_right() sends right event */
PCUT_TEST(right)
{
	ui_menu_t *menu = NULL;
	errno_t rc;
	test_resp_t resp;
	sysarg_t idev_id;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_set_cb(menu, &testmenu_cb, (void *)&resp);

	memset(&resp, 0, sizeof(resp));
	PCUT_ASSERT_FALSE(resp.right_called);

	idev_id = 42;
	ui_menu_right(menu, idev_id);

	PCUT_ASSERT_TRUE(resp.right_called);
	PCUT_ASSERT_EQUALS(menu, resp.menu);
	PCUT_ASSERT_INT_EQUALS(idev_id, resp.idev_id);

	ui_menu_destroy(menu);
}

/** ui_menu_close_req() sends close_req event */
PCUT_TEST(close_req)
{
	ui_menu_t *menu = NULL;
	errno_t rc;
	test_resp_t resp;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_set_cb(menu, &testmenu_cb, (void *)&resp);

	memset(&resp, 0, sizeof(resp));
	PCUT_ASSERT_FALSE(resp.close_req_called);

	ui_menu_close_req(menu);

	PCUT_ASSERT_TRUE(resp.close_req_called);
	PCUT_ASSERT_EQUALS(menu, resp.menu);

	ui_menu_destroy(menu);
}

/** ui_menu_press_accel() sends press_accel event */
PCUT_TEST(press_accel)
{
	ui_menu_t *menu = NULL;
	errno_t rc;
	test_resp_t resp;
	char32_t c;
	sysarg_t idev_id;

	rc = ui_menu_create(NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	ui_menu_set_cb(menu, &testmenu_cb, (void *)&resp);

	memset(&resp, 0, sizeof(resp));
	PCUT_ASSERT_FALSE(resp.press_accel_called);

	c = 'A';
	idev_id = 42;
	ui_menu_press_accel(menu, c, idev_id);

	PCUT_ASSERT_TRUE(resp.press_accel_called);
	PCUT_ASSERT_EQUALS(menu, resp.menu);
	PCUT_ASSERT_EQUALS(c, resp.c);
	PCUT_ASSERT_INT_EQUALS(idev_id, resp.idev_id);

	ui_menu_destroy(menu);
}

/** Test menu left callback */
static void testmenu_left(ui_menu_t *menu, void *arg, sysarg_t idev_id)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->left_called = true;
	resp->menu = menu;
	resp->idev_id = idev_id;
}

/** Test menu right callback */
static void testmenu_right(ui_menu_t *menu, void *arg, sysarg_t idev_id)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->right_called = true;
	resp->menu = menu;
	resp->idev_id = idev_id;
}

/** Test menu close callback */
static void testmenu_close_req(ui_menu_t *menu, void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->close_req_called = true;
	resp->menu = menu;
}

/** Test menu press accel callback */
static void testmenu_press_accel(ui_menu_t *menu, void *arg,
    char32_t c, sysarg_t kbd_id)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->press_accel_called = true;
	resp->menu = menu;
	resp->c = c;
	resp->idev_id = kbd_id;
}

PCUT_EXPORT(menu);
