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

#include <errno.h>
#include <pcut/pcut.h>
#include <ui/fixed.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../tbsmenu.h"

PCUT_INIT;

PCUT_TEST_SUITE(tbsmenu);

/* Test creating and destroying taskbar start menu */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	tbsmenu_t *tbsmenu = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = tbsmenu_create(window, fixed, UI_DISPLAY_DEFAULT, &tbsmenu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tbsmenu_destroy(tbsmenu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test tbsmenu_open/close/is_open() */
PCUT_TEST(open_close_is_open)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	tbsmenu_t *tbsmenu = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = tbsmenu_create(window, fixed, UI_DISPLAY_DEFAULT, &tbsmenu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(tbsmenu_is_open(tbsmenu));
	tbsmenu_open(tbsmenu);
	PCUT_ASSERT_TRUE(tbsmenu_is_open(tbsmenu));
	tbsmenu_close(tbsmenu);
	PCUT_ASSERT_FALSE(tbsmenu_is_open(tbsmenu));

	tbsmenu_destroy(tbsmenu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(tbsmenu);
