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
#include <ui/tab.h>
#include <ui/tabset.h>
#include <ui/testctl.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/resource.h"
#include "../private/tab.h"
#include "../private/tabset.h"

PCUT_INIT;

PCUT_TEST_SUITE(tab);

/** Create and destroy tab */
PCUT_TEST(create_destroy)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	/*
	 * Normally we don't need to destroy a tab explicitly, it will
	 * be destroyed along with tab bar, but here we'll test destroying
	 * it explicitly.
	 */
	ui_tab_destroy(tab);
	ui_tab_set_destroy(tabset);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Destroy tab implicitly by destroying the tab set */
PCUT_TEST(implicit_destroy)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	/* Let the tab be destroyed as part of destroying tab set */
	ui_tab_set_destroy(tabset);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_tab_destroy(NULL);
}

/** ui_tab_first() / ui_tab_next() iterate over tabs */
PCUT_TEST(first_next)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab1 = NULL;
	ui_tab_t *tab2 = NULL;
	ui_tab_t *t;
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
	PCUT_ASSERT_NOT_NULL(tab1);

	rc = ui_tab_create(tabset, "Test 2", &tab2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab2);

	t = ui_tab_first(tabset);
	PCUT_ASSERT_EQUALS(tab1, t);

	t = ui_tab_next(t);
	PCUT_ASSERT_EQUALS(tab2, t);

	t = ui_tab_next(t);
	PCUT_ASSERT_NULL(t);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_last() / ui_tab_prev() iterate over tabs in reverse */
PCUT_TEST(last_prev)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab1 = NULL;
	ui_tab_t *tab2 = NULL;
	ui_tab_t *t;
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
	PCUT_ASSERT_NOT_NULL(tab1);

	rc = ui_tab_create(tabset, "Test 2", &tab2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab2);

	t = ui_tab_last(tabset);
	PCUT_ASSERT_EQUALS(tab2, t);

	t = ui_tab_prev(t);
	PCUT_ASSERT_EQUALS(tab1, t);

	t = ui_tab_prev(t);
	PCUT_ASSERT_NULL(t);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_is_selected() correctly returns tab state */
PCUT_TEST(is_selected)
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
	PCUT_ASSERT_TRUE(ui_tab_is_selected(tab1));

	rc = ui_tab_create(tabset, "Test 2", &tab2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* After adding a second tab the first should still be selected */
	PCUT_ASSERT_TRUE(ui_tab_is_selected(tab1));
	PCUT_ASSERT_FALSE(ui_tab_is_selected(tab2));

	/* Select second tab */
	ui_tab_set_select(tabset, tab2);

	/* Now the second tab should be selected */
	PCUT_ASSERT_FALSE(ui_tab_is_selected(tab1));
	PCUT_ASSERT_TRUE(ui_tab_is_selected(tab2));

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_add() adds control to tab */
PCUT_TEST(add)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add test control to tab */
	ui_tab_add(tab, ui_test_ctl_ctl(testctl));

	resp.destroy = false;

	ui_tab_set_destroy(tabset);

	/* Destroying the tab should have destroyed the control as well */
	PCUT_ASSERT_TRUE(resp.destroy);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_remove() removes control to tab */
PCUT_TEST(remove)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add test control to tab */
	ui_tab_add(tab, ui_test_ctl_ctl(testctl));

	/* Rmove control from tab */
	ui_tab_remove(tab, ui_test_ctl_ctl(testctl));

	resp.destroy = false;

	ui_tab_set_destroy(tabset);

	/* Destroying the tab should NOT have destroyed the control */
	PCUT_ASSERT_FALSE(resp.destroy);

	ui_test_ctl_destroy(testctl);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Paint tab */
PCUT_TEST(paint)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_tab_paint(tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_kbd_event() delivers keyboard event */
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
	claimed = ui_tab_kbd_event(tab, &event);
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

/** ui_tab_pos_event() delivers position event */
PCUT_TEST(pos_event)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	gfx_rect_t rect;
	ui_tab_t *tab = NULL;
	ui_evclaim_t claimed;
	pos_event_t event;
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

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 100;
	rect.p1.y = 200;
	ui_tab_set_set_rect(tabset, &rect);

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Set up response */
	ui_tab_add(tab, ui_test_ctl_ctl(testctl));
	resp.claim = ui_claimed;
	resp.pos = false;

	/* Send position event */
	event.type = POS_PRESS;
	event.hpos = 10;
	event.vpos = 40;
	claimed = ui_tab_pos_event(tab, &event);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	/* Make sure event was delivered */
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_EQUALS(event.type, resp.pevent.type);
	PCUT_ASSERT_EQUALS(event.hpos, resp.pevent.hpos);
	PCUT_ASSERT_EQUALS(event.vpos, resp.pevent.vpos);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_handle_width() and ui_tab_handle_height() return dimensions */
PCUT_TEST(handle_width_height)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
	gfx_coord_t w, h;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	w = ui_tab_handle_width(tab);
	h = ui_tab_handle_height(tab);

	PCUT_ASSERT_INT_EQUALS(50, w);
	PCUT_ASSERT_INT_EQUALS(25, h);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Computing tab geometry */
PCUT_TEST(get_geom)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
	ui_tab_geom_t geom;
	gfx_rect_t rect;
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

	rect.p0.x = 1000;
	rect.p0.y = 2000;
	rect.p1.x = 1100;
	rect.p1.y = 2200;
	ui_tab_set_set_rect(tabset, &rect);

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	ui_tab_get_geom(tab, &geom);

	PCUT_ASSERT_INT_EQUALS(1006, geom.handle.p0.x);
	PCUT_ASSERT_INT_EQUALS(2000, geom.handle.p0.y);
	PCUT_ASSERT_INT_EQUALS(1056, geom.handle.p1.x);
	PCUT_ASSERT_INT_EQUALS(2027, geom.handle.p1.y);

	PCUT_ASSERT_INT_EQUALS(1006, geom.handle_area.p0.x);
	PCUT_ASSERT_INT_EQUALS(2000, geom.handle_area.p0.y);
	PCUT_ASSERT_INT_EQUALS(1056, geom.handle_area.p1.x);
	PCUT_ASSERT_INT_EQUALS(2027, geom.handle_area.p1.y);

	PCUT_ASSERT_INT_EQUALS(1000, geom.body.p0.x);
	PCUT_ASSERT_INT_EQUALS(2025, geom.body.p0.y);
	PCUT_ASSERT_INT_EQUALS(1100, geom.body.p1.x);
	PCUT_ASSERT_INT_EQUALS(2200, geom.body.p1.y);

	PCUT_ASSERT_INT_EQUALS(1014, geom.text_pos.x);
	PCUT_ASSERT_INT_EQUALS(2007, geom.text_pos.y);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_patint_handle_frame() */
PCUT_TEST(paint_handle_frame)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	gfx_context_t *gc;
	gfx_rect_t rect;
	gfx_rect_t irect;
	gfx_coord_t chamfer;
	gfx_color_t *hi_color;
	gfx_color_t *sh_color;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);
	gc = ui_window_get_gc(window);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	chamfer = 4;

	hi_color = res->wnd_highlight_color;
	sh_color = res->wnd_shadow_color;

	rc = ui_tab_paint_handle_frame(gc, &rect, chamfer, hi_color, sh_color,
	    true, &irect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_tab_paint_handle_frame(gc, &rect, chamfer, hi_color, sh_color,
	    false, &irect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_paint_body_frame() */
PCUT_TEST(paint_body_frame)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_tab_paint_body_frame(tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_paint_frame() */
PCUT_TEST(paint_frame)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	rc = ui_tab_paint_frame(tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_tab_get_res() returns the resource */
PCUT_TEST(get_res)
{
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_resource_t *res;
	ui_tab_set_t *tabset = NULL;
	ui_tab_t *tab = NULL;
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

	rc = ui_tab_create(tabset, "Test", &tab);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tab);

	PCUT_ASSERT_EQUALS(res, ui_tab_get_res(tab));

	ui_tab_set_destroy(tabset);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(tab);
