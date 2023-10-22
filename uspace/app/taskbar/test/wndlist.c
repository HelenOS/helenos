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

#include <errno.h>
#include <loc.h>
#include <pcut/pcut.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <wndmgt_srv.h>
#include "../wndlist.h"

PCUT_INIT;

PCUT_TEST_SUITE(taskbar);

static void test_wndmgt_conn(ipc_call_t *, void *);

static errno_t test_get_window_list(void *, wndmgt_window_list_t **);
static errno_t test_get_window_info(void *, sysarg_t, wndmgt_window_info_t **);

static wndmgt_ops_t test_wndmgt_srv_ops = {
	.get_window_list = test_get_window_list,
	.get_window_info = test_get_window_info
};

static const char *test_wndmgt_server = "test-wndlist-wm";
static const char *test_wndmgt_svc = "test/wndlist-wm";

/* Test creating and destroying window list */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/* Test setting window list rectangle */
PCUT_TEST(set_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	gfx_rect_t rect;
	wndlist_t *wndlist;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	wndlist_set_rect(wndlist, &rect);
	PCUT_ASSERT_INT_EQUALS(1, wndlist->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, wndlist->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, wndlist->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, wndlist->rect.p1.y);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test opening WM service */
PCUT_TEST(open_wm)
{
	errno_t rc;
	service_id_t sid;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	loc_srv_t *srv;

	/* Set up a test WM service */

	async_set_fallback_port_handler(test_wndmgt_conn, NULL);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create window list and connect it to our test service */

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_open_wm(wndlist, test_wndmgt_svc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Test appending new entry */
PCUT_TEST(append)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 123, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);
	PCUT_ASSERT_INT_EQUALS(123, entry->wnd_id);
	PCUT_ASSERT_NOT_NULL(entry->button);
	PCUT_ASSERT_TRUE(ui_pbutton_get_light(entry->button));

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test removing entry */
PCUT_TEST(remove)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);
	PCUT_ASSERT_INT_EQUALS(1, entry->wnd_id);

	rc = wndlist_remove(wndlist, entry, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);
	PCUT_ASSERT_INT_EQUALS(2, entry->wnd_id);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test updating entry */
PCUT_TEST(update)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);
	PCUT_ASSERT_INT_EQUALS(1, entry->wnd_id);
	PCUT_ASSERT_NOT_NULL(entry->button);
	PCUT_ASSERT_TRUE(ui_pbutton_get_light(entry->button));

	rc = wndlist_update(wndlist, entry, "Bar", false);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, entry->wnd_id);
	PCUT_ASSERT_NOT_NULL(entry->button);
	PCUT_ASSERT_FALSE(ui_pbutton_get_light(entry->button));

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test setting entry rectangle */
PCUT_TEST(set_entry_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 123, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);

	/* Set entry rectangle */
	wndlist_set_entry_rect(wndlist, entry);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test finding entry by window ID */
PCUT_TEST(entry_by_id)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_entry_by_id(wndlist, 1);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_INT_EQUALS(1, entry->wnd_id);

	entry = wndlist_entry_by_id(wndlist, 2);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_INT_EQUALS(2, entry->wnd_id);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test wndlist_first() / wndlist_next() */
PCUT_TEST(first_next)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_first(wndlist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_INT_EQUALS(1, entry->wnd_id);

	entry = wndlist_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_INT_EQUALS(2, entry->wnd_id);

	entry = wndlist_next(entry);
	PCUT_ASSERT_NULL(entry);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test wndlist_last() */
PCUT_TEST(last)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	wndlist_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = wndlist_last(wndlist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_INT_EQUALS(2, entry->wnd_id);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test wndlist_count() */
PCUT_TEST(count)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;
	size_t count;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	count = wndlist_count(wndlist);
	PCUT_ASSERT_INT_EQUALS(0, count);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	count = wndlist_count(wndlist);
	PCUT_ASSERT_INT_EQUALS(1, count);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	count = wndlist_count(wndlist);
	PCUT_ASSERT_INT_EQUALS(2, count);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test repainting window list */
PCUT_TEST(repaint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_fixed_t *fixed = NULL;
	wndlist_t *wndlist;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_fixed_create(&fixed);
	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = wndlist_create(window, fixed, &wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 1, "Foo", true, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_append(wndlist, 2, "Bar", false, true);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndlist_repaint(wndlist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	wndlist_destroy(wndlist);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test window management service connection. */
static void test_wndmgt_conn(ipc_call_t *icall, void *arg)
{
	wndmgt_srv_t srv;

	/* Set up protocol structure */
	wndmgt_srv_initialize(&srv);
	srv.ops = &test_wndmgt_srv_ops;
	srv.arg = arg;

	/* Handle connection */
	wndmgt_conn(icall, &srv);
}

static errno_t test_get_window_list(void *arg, wndmgt_window_list_t **rlist)
{
	wndmgt_window_list_t *wlist;

	(void)arg;

	wlist = calloc(1, sizeof(wndmgt_window_list_t));
	if (wlist == NULL)
		return ENOMEM;

	wlist->nwindows = 1;
	wlist->windows = calloc(1, sizeof(sysarg_t));
	if (wlist->windows == NULL) {
		free(wlist);
		return ENOMEM;
	}

	wlist->windows[0] = 42;
	*rlist = wlist;
	return EOK;
}

static errno_t test_get_window_info(void *arg, sysarg_t wnd_id,
    wndmgt_window_info_t **rinfo)
{
	wndmgt_window_info_t *winfo;

	(void)arg;

	winfo = calloc(1, sizeof(wndmgt_window_info_t));
	if (winfo == NULL)
		return ENOMEM;

	winfo->caption = str_dup("Hello");
	if (winfo->caption == NULL) {
		free(winfo);
		return ENOMEM;
	}

	*rinfo = winfo;
	return EOK;
}

PCUT_EXPORT(wndlist);
