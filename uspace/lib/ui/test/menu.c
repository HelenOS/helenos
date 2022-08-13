/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Create and destroy menu */
PCUT_TEST(create_destroy)
{
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	errno_t rc;

	rc = ui_menu_bar_create(NULL, NULL, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	/*
	 * Normally we don't need to destroy a menu explicitly, it will
	 * be destroyed along with menu bar, but here we'll test destroying
	 * it explicitly.
	 */
	ui_menu_destroy(menu);
	ui_menu_bar_destroy(mbar);
}

/** ui_menu_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_destroy(NULL);
}

/** ui_menu_first() / ui_menu_next() iterate over menus */
PCUT_TEST(first_next)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu1 = NULL;
	ui_menu_t *menu2 = NULL;
	ui_menu_t *m;
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

	rc = ui_menu_create(mbar, "Test 1", &menu1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu1);

	rc = ui_menu_create(mbar, "Test 1", &menu2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu2);

	m = ui_menu_first(mbar);
	PCUT_ASSERT_EQUALS(menu1, m);

	m = ui_menu_next(m);
	PCUT_ASSERT_EQUALS(menu2, m);

	m = ui_menu_next(m);
	PCUT_ASSERT_NULL(m);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_last() / ui_menu_prev() iterate over menus in reverse */
PCUT_TEST(last_prev)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu1 = NULL;
	ui_menu_t *menu2 = NULL;
	ui_menu_t *m;
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

	rc = ui_menu_create(mbar, "Test 1", &menu1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu1);

	rc = ui_menu_create(mbar, "Test 1", &menu2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu2);

	m = ui_menu_last(mbar);
	PCUT_ASSERT_EQUALS(menu2, m);

	m = ui_menu_prev(m);
	PCUT_ASSERT_EQUALS(menu1, m);

	m = ui_menu_prev(m);
	PCUT_ASSERT_NULL(m);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_caption() returns the menu's caption */
PCUT_TEST(caption)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	const char *caption;
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

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	caption = ui_menu_caption(menu);
	PCUT_ASSERT_NOT_NULL(caption);

	PCUT_ASSERT_INT_EQUALS(0, str_cmp(caption, "Test"));

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_get_rect() returns outer menu rectangle */
PCUT_TEST(get_rect)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	const char *caption;
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

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	caption = ui_menu_caption(menu);
	PCUT_ASSERT_NOT_NULL(caption);

	pos.x = 0;
	pos.y = 0;
	ui_menu_get_rect(menu, &pos, &rect);

	PCUT_ASSERT_INT_EQUALS(0, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(16, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, rect.p1.y);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Open and close menu with ui_menu_open() / ui_menu_close() */
PCUT_TEST(open_close)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Open and close */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_close(menu);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_is_open() correctly returns menu state */
PCUT_TEST(is_open)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_FALSE(open);

	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_TRUE(open);

	ui_menu_close(menu);

	open = ui_menu_is_open(menu);
	PCUT_ASSERT_FALSE(open);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint background in graphics mode */
PCUT_TEST(paint_bg_gfx)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint_bg_gfx(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint background in text mode */
PCUT_TEST(paint_bg_text)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint_bg_text(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint menu */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to paint it */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_up() with empty menu does nothing */
PCUT_TEST(up_empty)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_up(menu);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_up() moves one entry up, skips separators, wraps around */
PCUT_TEST(up)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
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
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* When menu is open, the first entry is selected */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_up(menu);

	/* Now we've wrapped around to the last entry */
	PCUT_ASSERT_EQUALS(mentry3, menu->selected);

	ui_menu_up(menu);

	/* mentry2 is a separator and was skipped */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_down() with empty menu does nothing */
PCUT_TEST(down_empty)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Menu needs to be open to be able to move around it */
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_down(menu);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_down() moves one entry down, skips separators, wraps around */
PCUT_TEST(down)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
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
	rc = ui_menu_open(menu, &prect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* When menu is open, the first entry is selected */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_down(menu);

	/* mentry2 is a separator and was skipped */
	PCUT_ASSERT_EQUALS(mentry3, menu->selected);

	ui_menu_up(menu);

	/* Now we've wrapped around to the first entry */
	PCUT_ASSERT_EQUALS(mentry1, menu->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_pos_event() inside menu is claimed */
PCUT_TEST(pos_event_inside)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	pos.x = 0;
	pos.y = 0;
	event.type = POS_PRESS;
	event.hpos = 0;
	event.vpos = 0;
	claimed = ui_menu_pos_event(menu, &pos, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Computing menu geometry */
PCUT_TEST(get_geom)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
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

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(menu);
