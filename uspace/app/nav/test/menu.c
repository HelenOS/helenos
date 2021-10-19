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

#include <errno.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include "../menu.h"

PCUT_INIT;

PCUT_TEST_SUITE(menu);

static void test_menu_file_open(void *);
static void test_menu_file_exit(void *);

static nav_menu_cb_t dummy_cb;
static nav_menu_cb_t test_cb = {
	.file_open = test_menu_file_open,
	.file_exit = test_menu_file_exit
};

/** Test response */
typedef struct {
	bool file_open;
	bool file_exit;
} test_resp_t;

/** Create and destroy menu. */
PCUT_TEST(create_destroy)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	nav_menu_t *menu;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = nav_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	nav_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** nav_menu_set_cb() */
PCUT_TEST(set_cb)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	nav_menu_t *menu;
	int foo;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = nav_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	nav_menu_set_cb(menu, &test_cb, &foo);
	PCUT_ASSERT_EQUALS(&test_cb, menu->cb);
	PCUT_ASSERT_EQUALS(&foo, menu->cb_arg);

	nav_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** File / Open callback */
PCUT_TEST(file_open)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	nav_menu_t *menu;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = nav_menu_create(window, &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Call back with no callbacks set */
	nav_menu_file_open(NULL, menu);

	/* Call back with dummy callbacks set */
	nav_menu_set_cb(menu, &dummy_cb, &resp);
	nav_menu_file_open(NULL, menu);

	/* Call back with test callbacks set */
	resp.file_open = false;
	nav_menu_set_cb(menu, &test_cb, &resp);
	nav_menu_file_open(NULL, menu);
	PCUT_ASSERT_TRUE(resp.file_open);

	nav_menu_destroy(menu);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Testing File / Open callback */
static void test_menu_file_open(void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->file_open = true;
}

/** Testing File / Exit callback */
static void test_menu_file_exit(void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->file_exit = true;
}

PCUT_EXPORT(menu);
