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
#include <ui/menudd.h>
#include <ui/menubar.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/menu.h"
#include "../private/menubar.h"

PCUT_INIT;

PCUT_TEST_SUITE(menudd);

/** Create and destroy menu drop-down */
PCUT_TEST(create_destroy)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd = NULL;
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

	rc = ui_menu_dd_create(mbar, "Test", &mdd, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd);
	PCUT_ASSERT_NOT_NULL(menu);

	/*
	 * Normally we don't need to destroy a menu drop-down explicitly,
	 * it will be destroyed along with menu bar, but here we'll test
	 * destroying it explicitly.
	 */
	ui_menu_dd_destroy(mdd);
	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_dd_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_dd_destroy(NULL);
}

/** ui_menu_dd_first() / ui_menu_dd_next() iterate over menu drop-downs */
PCUT_TEST(first_next)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd1 = NULL;
	ui_menu_dd_t *mdd2 = NULL;
	ui_menu_dd_t *m;
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

	rc = ui_menu_dd_create(mbar, "Test 1", &mdd1, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd1);

	rc = ui_menu_dd_create(mbar, "Test 1", &mdd2, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd2);

	m = ui_menu_dd_first(mbar);
	PCUT_ASSERT_EQUALS(mdd1, m);

	m = ui_menu_dd_next(m);
	PCUT_ASSERT_EQUALS(mdd2, m);

	m = ui_menu_dd_next(m);
	PCUT_ASSERT_NULL(m);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_dd_last() / ui_menu_dd_prev() iterate over menu drop-downs
 * in reverse.
 */
PCUT_TEST(last_prev)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd1 = NULL;
	ui_menu_dd_t *mdd2 = NULL;
	ui_menu_dd_t *m;
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

	rc = ui_menu_dd_create(mbar, "Test 1", &mdd1, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd1);

	rc = ui_menu_dd_create(mbar, "Test 1", &mdd2, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd2);

	m = ui_menu_dd_last(mbar);
	PCUT_ASSERT_EQUALS(mdd2, m);

	m = ui_menu_dd_prev(m);
	PCUT_ASSERT_EQUALS(mdd1, m);

	m = ui_menu_dd_prev(m);
	PCUT_ASSERT_NULL(m);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_dd_caption() returns the drop down's caption */
PCUT_TEST(caption)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd = NULL;
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

	rc = ui_menu_dd_create(mbar, "Test", &mdd, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd);

	caption = ui_menu_dd_caption(mdd);
	PCUT_ASSERT_NOT_NULL(caption);

	PCUT_ASSERT_INT_EQUALS(0, str_cmp(caption, "Test"));

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_dd_get_accel() returns the accelerator character */
PCUT_TEST(get_accel)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd = NULL;
	char32_t accel;
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

	rc = ui_menu_dd_create(mbar, "~T~est", &mdd, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd);

	accel = ui_menu_dd_get_accel(mdd);
	printf("accel='%c' (%d)\n", (char)accel, (int)accel);
	PCUT_ASSERT_EQUALS((char32_t)'t', accel);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Open and close menu drop-down with ui_menu_dd_open() / ui_menu_dd_close() */
PCUT_TEST(open_close)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd = NULL;
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

	rc = ui_menu_dd_create(mbar, "Test", &mdd, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	/* Open and close */
	rc = ui_menu_dd_open(mdd, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_dd_close(mdd);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_menu_dd_is_open() correctly returns menu drop-down state */
PCUT_TEST(is_open)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_dd_t *mdd = NULL;
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

	rc = ui_menu_dd_create(mbar, "Test", &mdd, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mdd);

	prect.p0.x = 0;
	prect.p0.y = 0;
	prect.p1.x = 0;
	prect.p1.y = 0;

	open = ui_menu_dd_is_open(mdd);
	PCUT_ASSERT_FALSE(open);

	rc = ui_menu_dd_open(mdd, &prect, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	open = ui_menu_dd_is_open(mdd);
	PCUT_ASSERT_TRUE(open);

	ui_menu_dd_close(mdd);

	open = ui_menu_dd_is_open(mdd);
	PCUT_ASSERT_FALSE(open);

	ui_menu_bar_destroy(mbar);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(menudd);
