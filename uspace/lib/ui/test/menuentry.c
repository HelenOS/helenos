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
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/menuentry.h"

PCUT_INIT;

PCUT_TEST_SUITE(menuentry);

typedef struct {
	bool activated;
} test_resp_t;

static void test_entry_cb(ui_menu_entry_t *, void *);

/** Create and destroy menu entry */
PCUT_TEST(create_destroy)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	/* Just for sake of test. Menu entry is destroyed along with menu */
	ui_menu_entry_destroy(mentry);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Create and destroy separator menu entry */
PCUT_TEST(create_sep_destroy)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_sep_create(menu, &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	/* Just for sake of test. Menu entry is destroyed along with menu */
	ui_menu_entry_destroy(mentry);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_bar_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_bar_destroy(NULL);
}

/** ui_menu_entry_set_cb() .. */
PCUT_TEST(set_cb)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);

	resp.activated = false;
	ui_menu_entry_cb(mentry);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_entry_first() / ui_menu_entry_next() iterate over entries */
PCUT_TEST(first_next)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *entry1 = NULL;
	ui_menu_entry_t *entry2 = NULL;
	ui_menu_entry_t *e;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &entry1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry1);

	rc = ui_menu_entry_create(menu, "Bar", "F2", &entry2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry2);

	e = ui_menu_entry_first(menu);
	PCUT_ASSERT_EQUALS(entry1, e);

	e = ui_menu_entry_next(e);
	PCUT_ASSERT_EQUALS(entry2, e);

	e = ui_menu_entry_next(e);
	PCUT_ASSERT_NULL(e);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_entry_last() / ui_menu_entry_prev() iterate over entries in reverse */
PCUT_TEST(last_prev)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *entry1 = NULL;
	ui_menu_entry_t *entry2 = NULL;
	ui_menu_entry_t *e;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &entry1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry1);

	rc = ui_menu_entry_create(menu, "Bar", "F2", &entry2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry2);

	e = ui_menu_entry_last(menu);
	PCUT_ASSERT_EQUALS(entry2, e);

	e = ui_menu_entry_prev(e);
	PCUT_ASSERT_EQUALS(entry1, e);

	e = ui_menu_entry_prev(e);
	PCUT_ASSERT_NULL(e);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_entry_column_widths() / ui_menu_entry_height() */
PCUT_TEST(widths_height)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord_t caption_w;
	gfx_coord_t shortcut_w;
	gfx_coord_t width;
	gfx_coord_t height;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_column_widths(mentry, &caption_w, &shortcut_w);
	PCUT_ASSERT_INT_EQUALS(11, caption_w);
	PCUT_ASSERT_INT_EQUALS(10, shortcut_w);

	width = ui_menu_entry_calc_width(menu, caption_w, shortcut_w);
	PCUT_ASSERT_INT_EQUALS(4 + 11 + 8 + 10 + 4, width);

	height = ui_menu_entry_height(mentry);
	PCUT_ASSERT_INT_EQUALS(13 + 8, height);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint menu entry */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_entry_paint(mentry, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_entry_selectable() returns correct value based on entry type */
PCUT_TEST(selectable)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	/* Selectable entry */

	rc = ui_menu_entry_create(menu, "Foo", "F1", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	PCUT_ASSERT_TRUE(ui_menu_entry_selectable(mentry));

	ui_menu_entry_destroy(mentry);

	/* Non-selectable separator entry */

	rc = ui_menu_entry_sep_create(menu, &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	PCUT_ASSERT_FALSE(ui_menu_entry_selectable(mentry));

	ui_menu_entry_destroy(mentry);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press and release activates menu entry */
PCUT_TEST(press_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press, leave and release does not activate entry */
PCUT_TEST(press_leave_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_leave(mentry, &pos);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press, leave, enter and release activates menu entry */
PCUT_TEST(press_leave_enter_release)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_leave(mentry, &pos);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_enter(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_entry_activate() activates menu entry */
PCUT_TEST(activate)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_rect_t prect;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(resp.activated);
	ui_menu_entry_activate(mentry);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press event inside menu entry */
PCUT_TEST(pos_press_inside)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 4;
	event.vpos = 4;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press event outside menu entry */
PCUT_TEST(pos_press_outside)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 40;
	event.vpos = 20;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_FALSE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Position event moving out of menu entry */
PCUT_TEST(pos_move_out)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	event.type = POS_UPDATE;
	event.hpos = 40;
	event.vpos = 20;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Position event moving inside menu entry */
PCUT_TEST(pos_move_in)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	gfx_rect_t prect;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_dd_create(mbar, "Test", NULL, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", "Y", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	rc = ui_menu_open(menu, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = POS_UPDATE;
	event.hpos = 4;
	event.vpos = 4;

	pos.x = 0;
	pos.y = 0;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_FALSE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

static void test_entry_cb(ui_menu_entry_t *mentry, void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->activated = true;
}

PCUT_EXPORT(menuentry);
