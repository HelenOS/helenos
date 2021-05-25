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

/** Calling ui_menu_bar_select() with the same menu twice deselects it */
PCUT_TEST(select_same)
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

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 0;
	rect.p1.y = 0;
	ui_menu_bar_select(mbar, &rect, menu);
	PCUT_ASSERT_EQUALS(menu, mbar->selected);

	/* Selecting again should unselect the menu */
	ui_menu_bar_select(mbar, &rect, menu);
	PCUT_ASSERT_NULL(mbar->selected);

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

	rc = ui_menu_create(mbar, "Test 1", &menu1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu1);

	rc = ui_menu_create(mbar, "Test 2", &menu2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu2);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 0;
	rect.p1.y = 0;
	ui_menu_bar_select(mbar, &rect, menu1);
	PCUT_ASSERT_EQUALS(menu1, mbar->selected);

	/* Selecting different menu should select it */
	ui_menu_bar_select(mbar, &rect, menu2);
	PCUT_ASSERT_EQUALS(menu2, mbar->selected);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(menubar);
