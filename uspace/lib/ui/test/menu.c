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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <str.h>
#include <ui/control.h>
#include <ui/menu.h>
#include <ui/menubar.h>
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
