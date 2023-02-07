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

#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/tab.h>
#include <ui/tabset.h>
#include <ui/testctl.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/tabset.h"

PCUT_INIT;

PCUT_TEST_SUITE(tabset);

/** Create and destroy tab set */
PCUT_TEST(create_destroy)
{
	ui_tab_set_t *tabset = NULL;
	errno_t rc;

	rc = ui_tab_set_create(NULL, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	ui_tab_set_destroy(tabset);
}

/** ui_tab_set_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_tab_set_destroy(NULL);
}

/** ui_tab_set_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_tab_set_t *tabset = NULL;
	ui_control_t *control;
	errno_t rc;

	rc = ui_tab_set_create(NULL, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	control = ui_tab_set_ctl(tabset);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set tab set rectangle sets internal field */
PCUT_TEST(set_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);

	rc = ui_tab_set_create(res, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_tab_set_set_rect(tabset, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tabset->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tabset->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tabset->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tabset->rect.p1.y);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint tab set */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);

	rc = ui_tab_set_create(res, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	rc = ui_tab_set_paint(tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Deliver tab set keyboard event */
PCUT_TEST(kbd_event)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
	ui_evclaim_t claimed;
	kbd_event_t event;
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);

	rc = ui_tab_set_create(res, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	/* Without anytabs, event should be unclaimed */
	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	claimed = ui_tab_set_kbd_event(tabset, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Set up response */
	ui_tab_add(tab, ui_test_ctl_ctl(testctl));
	resp.claim = ui_claimed;
	resp.kbd = false;

	/* Send keyboard event */
	event.type = KEY_PRESS;
	event.key = KC_F10;
	event.mods = 0;
	claimed = ui_tab_set_kbd_event(tabset, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	/* Make sure event was delivered */
	PCUT_ASSERT_TRUE(resp.kbd);
	PCUT_ASSERT_EQUALS(event.type, resp.kevent.type);
	PCUT_ASSERT_EQUALS(event.key, resp.kevent.key);
	PCUT_ASSERT_EQUALS(event.mods, resp.kevent.mods);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Press event on tab handle selects tab */
PCUT_TEST(pos_event_select)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab1 = NULL;
	ui_tab_t *tab2 = NULL;
	ui_evclaim_t claimed;
	pos_event_t event;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);

	rc = ui_tab_set_create(res, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	/* Without any tabs, event should be unclaimed */
	event.type = POS_PRESS;
	event.hpos = 80;
	event.vpos = 4;
	event.btn_num = 1;
	claimed = ui_tab_set_pos_event(tabset, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	rc = ui_tab_create(tabset, "Test 1", &tab1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* The first added tab should be automatically selected */
	PCUT_ASSERT_EQUALS(tab1, tabset->selected);

	rc = ui_tab_create(tabset, "Test 2", &tab2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* After adding a second tab the first should still be selected */
	PCUT_ASSERT_EQUALS(tab1, tabset->selected);

	event.type = POS_PRESS;
	event.hpos = 80;
	event.vpos = 4;
	event.btn_num = 1;
	claimed = ui_tab_set_pos_event(tabset, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	/* Clicking the second tab handle should select tab2 */
	PCUT_ASSERT_EQUALS(tab2, tabset->selected);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_set_select() selects tab */
PCUT_TEST(select)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab1 = NULL;
	ui_tab_t *tab2 = NULL;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);

	rc = ui_tab_set_create(res, &tabset);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tabset);

	rc = ui_tab_create(tabset, "Test 1", &tab1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* The first added tab should be automatically selected */
	PCUT_ASSERT_EQUALS(tab1, tabset->selected);

	rc = ui_tab_create(tabset, "Test 2", &tab2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* After adding a second tab the first should still be selected */
	PCUT_ASSERT_EQUALS(tab1, tabset->selected);

	/* Select second tab */
	ui_tab_set_select(tabset, tab2);

	/* Now the second tab should be selected */
	PCUT_ASSERT_EQUALS(tab2, tabset->selected);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(tabset);
