/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/menubar.h"

PCUT_INIT;

PCUT_TEST_SUITE(menubar);

/** Create and destroy menu bar */
PCUT_TEST(create_destroy)
{
	ui_menu_bar_t *mbar = NULL;
	errno_t rc;

	rc = ui_menu_bar_create(NULL, NULL, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	ui_menu_bar_destroy(mbar);
}

/** ui_menu_bar_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_bar_destroy(NULL);
}

/** ui_menu_bar_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_menu_bar_t *mbar = NULL;
	ui_control_t *control;
	errno_t rc;

	rc = ui_menu_bar_create(NULL, NULL, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	control = ui_menu_bar_ctl(mbar);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set menu bar rectangle sets internal field */
PCUT_TEST(set_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	gfx_rect_t rect;

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

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_menu_bar_set_rect(mbar, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, mbar->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, mbar->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, mbar->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, mbar->rect.p1.y);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint menu bar */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
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

	rc = ui_menu_bar_paint(mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Deliver menu bar keyboard event */
PCUT_TEST(kbd_event)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_evclaim_t claimed;
	kbd_event_t event;
	gfx_rect_t rect;
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

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 50;
	rect.p1.y = 25;
	ui_menu_bar_set_rect(mbar, &rect);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	event.type = KEY_PRESS;
	event.key = KC_F10;
	event.mods = 0;
	claimed = ui_menu_bar_kbd_event(mbar, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_bar_press_accel() opens the corresponding menu */
PCUT_TEST(press_accel)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	gfx_rect_t rect;
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

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 50;
	rect.p1.y = 25;
	ui_menu_bar_set_rect(mbar, &rect);

	rc = ui_menu_create(mbar, "~T~est", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	PCUT_ASSERT_FALSE(ui_menu_is_open(menu));

	ui_menu_bar_press_accel(mbar, 't');

	PCUT_ASSERT_TRUE(ui_menu_is_open(menu));

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press event on menu bar entry selects menu */
PCUT_TEST(pos_event_select)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_evclaim_t claimed;
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

	rc = ui_menu_bar_create(ui, window, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 50;
	rect.p1.y = 25;
	ui_menu_bar_set_rect(mbar, &rect);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	event.type = POS_PRESS;
	event.hpos = 4;
	event.vpos = 4;
	claimed = ui_menu_bar_pos_event(mbar, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	/* Clicking the menu bar entry should select menu */
	PCUT_ASSERT_EQUALS(menu, mbar->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Calling ui_menu_bar_select() with another menu selects it */
PCUT_TEST(select_different)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu1 = NULL;
	ui_menu_t *menu2 = NULL;
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

	rc = ui_menu_create(mbar, "Test 2", &menu2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu2);

	ui_menu_bar_select(mbar, menu1, true);
	PCUT_ASSERT_EQUALS(menu1, mbar->selected);

	/* Selecting different menu should select it */
	ui_menu_bar_select(mbar, menu2, true);
	PCUT_ASSERT_EQUALS(menu2, mbar->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_bar_activate() activates/deactivates menu bar */
PCUT_TEST(activate_deactivate)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
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

	ui_menu_bar_activate(mbar);
	PCUT_ASSERT_EQUALS(menu, mbar->selected);

	ui_menu_bar_deactivate(mbar);
	PCUT_ASSERT_NULL(mbar->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(menubar);
