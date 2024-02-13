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
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <str.h>
#include <ui/ui.h>
#include <ui/list.h>
#include <ui/scrollbar.h>
#include <vfs/vfs.h>
#include "../private/list.h"

PCUT_INIT;

PCUT_TEST_SUITE(list);

/** Test response */
typedef struct {
	bool activate_req;
	ui_list_t *activate_req_list;

	bool selected;
	ui_list_entry_t *selected_entry;
} test_resp_t;

static void test_list_activate_req(ui_list_t *, void *);
static void test_list_selected(ui_list_entry_t *, void *);
static int test_list_compare(ui_list_entry_t *, ui_list_entry_t *);

static ui_list_cb_t test_cb = {
	.activate_req = test_list_activate_req,
	.selected = test_list_selected,
	.compare = test_list_compare
};

/** Create and destroy file list. */
PCUT_TEST(create_destroy)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_set_cb() sets callback */
PCUT_TEST(set_cb)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);
	PCUT_ASSERT_EQUALS(&test_cb, list->cb);
	PCUT_ASSERT_EQUALS(&resp, list->cb_arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_get_cb_arg() returns the callback argument */
PCUT_TEST(get_cb_arg)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;
	test_resp_t resp;
	void *arg;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);
	arg = ui_list_get_cb_arg(list);
	PCUT_ASSERT_EQUALS((void *)&resp, arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_height() gives the correct height */
PCUT_TEST(entry_height)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;
	gfx_coord_t height;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Font height is 13, padding: 2 (top) + 2 (bottom) */
	height = ui_list_entry_height(list);
	PCUT_ASSERT_INT_EQUALS(17, height);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_list_entry_paint() */
PCUT_TEST(entry_paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);
	attr.caption = "a";
	attr.arg = (void *)1;

	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_entry_paint(ui_list_first(list), 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_list_paint() */
PCUT_TEST(paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_paint(list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_ctl() returns a valid UI control */
PCUT_TEST(ctl)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_control_t *control;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_list_ctl(list);
	PCUT_ASSERT_NOT_NULL(control);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_list_kbd_event() */
PCUT_TEST(kbd_event)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_evclaim_t claimed;
	kbd_event_t event;
	errno_t rc;

	/* Active list should claim events */

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_list_kbd_event(list, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_list_destroy(list);

	/* Inactive list should not claim events */

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, false, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_list_kbd_event(list, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_list_pos_event() */
PCUT_TEST(pos_event)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_evclaim_t claimed;
	pos_event_t event;
	gfx_rect_t rect;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_list_set_rect(list, &rect);

	ui_list_entry_attr_init(&attr);
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = ui_list_first(list);
	list->page_idx = 0;

	event.pos_id = 0;
	event.type = POS_PRESS;
	event.btn_num = 1;

	/* Clicking on the middle entry should select it */
	event.hpos = 20;
	event.vpos = 40;

	claimed = ui_list_pos_event(list, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	PCUT_ASSERT_NOT_NULL(list->cursor);
	PCUT_ASSERT_STR_EQUALS("b", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->cursor->arg);

	/* Clicking on the top edge should do a page-up */
	event.hpos = 20;
	event.vpos = 20;
	claimed = ui_list_pos_event(list, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	PCUT_ASSERT_NOT_NULL(list->cursor);
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_list_set_rect(list, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, list->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, list->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, list->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, list->rect.p1.y);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_page_size() returns correct size */
PCUT_TEST(page_size)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_list_set_rect(list, &rect);

	/* NOTE If page size changes, we have problems elsewhere in the tests */
	PCUT_ASSERT_INT_EQUALS(11, ui_list_page_size(list));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_inside_rect() gives correct interior rectangle */
PCUT_TEST(inside_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	gfx_rect_t rect;
	gfx_rect_t irect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_list_set_rect(list, &rect);

	ui_list_inside_rect(list, &irect);
	PCUT_ASSERT_INT_EQUALS(10 + 2, irect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20 + 2, irect.p0.y);
	PCUT_ASSERT_INT_EQUALS(50 - 2 - 23, irect.p1.x);
	PCUT_ASSERT_INT_EQUALS(220 - 2, irect.p1.y);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scrollbar_rect() gives correct scrollbar rectangle */
PCUT_TEST(scrollbar_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	gfx_rect_t rect;
	gfx_rect_t srect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_list_set_rect(list, &rect);

	ui_list_scrollbar_rect(list, &srect);
	PCUT_ASSERT_INT_EQUALS(50 - 2 - 23, srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20 + 2, srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(50 - 2, srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(220 - 2, srect.p1.y);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scrollbar_update() updates scrollbar position */
PCUT_TEST(scrollbar_update)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	gfx_rect_t rect;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	gfx_coord_t pos;
	gfx_coord_t move_len;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 50;
	rect.p1.y = 38;

	ui_list_set_rect(list, &rect);

	ui_list_entry_attr_init(&attr);
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = ui_list_next(ui_list_first(list));

	list->cursor = entry;
	list->cursor_idx = 1;
	list->page = entry;
	list->page_idx = 1;

	ui_list_scrollbar_update(list);

	/* Now scrollbar thumb should be all the way down */
	move_len = ui_scrollbar_move_length(list->scrollbar);
	pos = ui_scrollbar_get_pos(list->scrollbar);
	PCUT_ASSERT_INT_EQUALS(move_len, pos);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_is_active() returns list activity state */
PCUT_TEST(is_active)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(ui_list_is_active(list));
	ui_list_destroy(list);

	rc = ui_list_create(window, false, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(ui_list_is_active(list));
	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_activate() activates list */
PCUT_TEST(activate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, false, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(ui_list_is_active(list));
	rc = ui_list_activate(list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(ui_list_is_active(list));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_deactivate() deactivates list */
PCUT_TEST(deactivate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(ui_list_is_active(list));
	ui_list_deactivate(list);
	PCUT_ASSERT_FALSE(ui_list_is_active(list));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_get_cursor() returns the current cursor position */
PCUT_TEST(get_cursor)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	ui_list_entry_t *cursor;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry and get pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	entry = NULL;
	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	/* Cursor should be at the only entry */
	cursor = ui_list_get_cursor(list);
	PCUT_ASSERT_EQUALS(entry, cursor);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_set_cursor() sets list cursor position */
PCUT_TEST(set_cursor)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *e1;
	ui_list_entry_t *e2;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry and get pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e1);

	/* Append entry and get pointer to it */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(e2);

	/* Append entry */
	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor should be at the first entry */
	PCUT_ASSERT_EQUALS(e1, list->cursor);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);

	/* Set cursor to the second entry */
	ui_list_set_cursor(list, e2);
	PCUT_ASSERT_EQUALS(e2, list->cursor);
	PCUT_ASSERT_INT_EQUALS(1, list->cursor_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_attr_init() initializes entry attribute structure */
PCUT_TEST(entry_attr_init)
{
	ui_list_entry_attr_t attr;

	ui_list_entry_attr_init(&attr);
	PCUT_ASSERT_NULL(attr.caption);
	PCUT_ASSERT_NULL(attr.arg);
	PCUT_ASSERT_NULL(attr.color);
	PCUT_ASSERT_NULL(attr.bgcolor);
}

/** ui_list_entry_append() appends new entry */
PCUT_TEST(entry_append)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry without retrieving pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&list->entries));

	/* Append entry and get pointer to it */
	attr.caption = "b";
	attr.arg = (void *)2;
	entry = NULL;
	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_EQUALS(attr.arg, entry->arg);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&list->entries));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_move_up() moves entry up */
PCUT_TEST(entry_move_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *e1, *e2, *e3;
	ui_list_entry_t *e;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Create entries */

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, &e3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e1, e);

	/* Moving first entry up should have no effect */
	ui_list_entry_move_up(e1);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e1, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e2, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e3, e);

	e = ui_list_next(e);
	PCUT_ASSERT_NULL(e);

	/* Move second entry up */
	ui_list_entry_move_up(e2);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e2, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e1, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e3, e);

	e = ui_list_next(e);
	PCUT_ASSERT_NULL(e);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_move_down() moves entry down */
PCUT_TEST(entry_move_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *e1, *e2, *e3;
	ui_list_entry_t *e;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Create entries */

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, &e1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, &e2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, &e3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e1, e);

	/* Moving last entry down should have no effect */
	ui_list_entry_move_down(e3);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e1, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e2, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e3, e);

	e = ui_list_next(e);
	PCUT_ASSERT_NULL(e);

	/* Move second-to-last entry down */
	ui_list_entry_move_down(e2);

	e = ui_list_first(list);
	PCUT_ASSERT_EQUALS(e1, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e3, e);

	e = ui_list_next(e);
	PCUT_ASSERT_EQUALS(e2, e);

	e = ui_list_next(e);
	PCUT_ASSERT_NULL(e);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_delete() deletes entry */
PCUT_TEST(entry_delete)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&list->entries));

	entry = ui_list_first(list);
	ui_list_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&list->entries));

	entry = ui_list_first(list);
	ui_list_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(0, list_count(&list->entries));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_get_arg() gets entry argument */
PCUT_TEST(entry_get_arg)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	void *arg;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry and get pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	entry = NULL;
	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	arg = ui_list_entry_get_arg(entry);
	PCUT_ASSERT_EQUALS(attr.arg, arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_get_list() returns the containing list */
PCUT_TEST(entry_get_list)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_t *elist;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry and get pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	entry = NULL;
	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	/* Get the containing list */
	elist = ui_list_entry_get_list(entry);
	PCUT_ASSERT_EQUALS(list, elist);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_set_caption() sets entry captino */
PCUT_TEST(entry_set_caption)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Append entry and get pointer to it */
	attr.caption = "a";
	attr.arg = (void *)1;
	entry = NULL;
	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	/* Change caption */
	rc = ui_list_entry_set_caption(entry, "b");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("b", entry->caption);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entries_cnt() returns the number of entries */
PCUT_TEST(entries_cnt)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(0, ui_list_entries_cnt(list));

	ui_list_entry_attr_init(&attr);

	/* Append entry */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, ui_list_entries_cnt(list));

	/* Append another entry */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_entries_cnt(list));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_sort() sorts UI list entries */
PCUT_TEST(sort)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	ui_list_entry_attr_init(&attr);

	attr.caption = "b";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "a";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_sort(list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = ui_list_first(list);
	PCUT_ASSERT_STR_EQUALS("a", entry->caption);
	PCUT_ASSERT_EQUALS((void *)2, entry->arg);

	entry = ui_list_next(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->caption);
	PCUT_ASSERT_EQUALS((void *)1, entry->arg);

	entry = ui_list_next(entry);
	PCUT_ASSERT_STR_EQUALS("c", entry->caption);
	PCUT_ASSERT_EQUALS((void *)3, entry->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_center()...XXX */
PCUT_TEST(cursor_center)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *a, *b, *c, *d, *e;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 80;

	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(3, ui_list_page_size(list));

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, &a);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, &b);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We only have two entries, but three fit onto the page */
	ui_list_cursor_center(list, b);
	PCUT_ASSERT_EQUALS(b, list->cursor);
	/* Page should start at the beginning */
	PCUT_ASSERT_EQUALS(a, list->page);
	PCUT_ASSERT_EQUALS(0, list->page_idx);

	/* Add more entries */

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, &c);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, &d);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, &e);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_cursor_center(list, c);
	PCUT_ASSERT_EQUALS(c, list->cursor);
	/*
	 * We have enough entries, c should be in the middle of the three
	 * entries on the page, i.e., page should start on 'b'.
	 */
	PCUT_ASSERT_EQUALS(b, list->page);
	PCUT_ASSERT_EQUALS(1, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_clear_entries() removes all entries from list */
PCUT_TEST(clear_entries)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "a";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&list->entries));

	ui_list_clear_entries(list);
	PCUT_ASSERT_INT_EQUALS(0, list_count(&list->entries));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_first() returns valid entry or @c NULL as appropriate */
PCUT_TEST(first)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	entry = ui_list_first(list);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = ui_list_first(list);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)entry->arg);

	/* Add another entry */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should still get the first entry */
	entry = ui_list_first(list);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)entry->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_last() returns valid entry or @c NULL as appropriate */
PCUT_TEST(last)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	entry = ui_list_last(list);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = ui_list_last(list);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)entry->arg);

	/* Add another entry */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should get new entry now */
	entry = ui_list_last(list);
	PCUT_ASSERT_NOT_NULL(entry);
	attr.caption = "b";
	attr.arg = (void *)2;
	PCUT_ASSERT_STR_EQUALS("b", entry->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)entry->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_next() returns the next entry or @c NULL as appropriate */
PCUT_TEST(next)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Add one entry */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its successor */
	entry = ui_list_first(list);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_list_next(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the successor of first entry again */
	entry = ui_list_first(list);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_list_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)entry->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_prev() returns the previous entry or @c NULL as appropriate */
PCUT_TEST(prev)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Add one entry */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its predecessor */
	entry = ui_list_last(list);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_list_prev(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the predecessor of the new entry */
	entry = ui_list_last(list);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_list_prev(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)entry->arg);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_page_nth_entry() .. */
PCUT_TEST(page_nth_entry)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *entry;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	size_t idx;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_entry_attr_init(&attr);

	/* Add some entries */
	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	list->page = ui_list_next(ui_list_first(list));
	list->page_idx = 1;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 100;
	rect.p1.y = 100;
	ui_list_set_rect(list, &rect);

	entry = ui_list_page_nth_entry(list, 0, &idx);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->caption);
	PCUT_ASSERT_INT_EQUALS(1, idx);

	entry = ui_list_page_nth_entry(list, 1, &idx);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("c", entry->caption);
	PCUT_ASSERT_INT_EQUALS(2, idx);

	entry = ui_list_page_nth_entry(list, 2, &idx);
	PCUT_ASSERT_NULL(entry);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_move() moves cursor and scrolls */
PCUT_TEST(cursor_move)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;
	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 2;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 1;

	/* Move cursor one entry up */
	ui_list_cursor_move(list, ui_list_prev(list->cursor),
	    list->cursor_idx - 1);

	/* Cursor and page start should now both be at the second entry */
	PCUT_ASSERT_STR_EQUALS("b", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Move cursor to the first entry. This should scroll up. */
	ui_list_cursor_move(list, ui_list_first(list), 0);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Move cursor to the last entry. */
	ui_list_cursor_move(list, ui_list_last(list), 2);

	/* Cursor should be on the last entry and page on the next to last */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_up() moves cursor one entry up */
PCUT_TEST(cursor_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;
	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 2;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 1;

	/* Move cursor one entry up */
	ui_list_cursor_up(list);

	/* Cursor and page start should now both be at the second entry */
	PCUT_ASSERT_STR_EQUALS("b", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Move cursor one entry up. This should scroll up. */
	ui_list_cursor_up(list);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	ui_list_cursor_up(list);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_down() moves cursor one entry down */
PCUT_TEST(cursor_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = list->cursor;
	list->page_idx = 0;

	/* Move cursor one entry down */
	ui_list_cursor_down(list);

	/* Cursor should now be at the second entry, page stays the same */
	PCUT_ASSERT_STR_EQUALS("b", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->cursor_idx);
	PCUT_ASSERT_EQUALS(ui_list_first(list), list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Move cursor one entry down. This should scroll down. */
	ui_list_cursor_down(list);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	ui_list_cursor_down(list);

	/* Cursor should still be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_top() moves cursor to the first entry */
PCUT_TEST(cursor_top)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 2;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 1;

	/* Move cursor to the top. This should scroll up. */
	ui_list_cursor_top(list);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_cursor_bottom() moves cursor to the last entry */
PCUT_TEST(cursor_bottom)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = list->cursor;
	list->page_idx = 0;

	/* Move cursor to the bottom. This should scroll down. */
	ui_list_cursor_bottom(list);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_page_up() moves one page up */
PCUT_TEST(page_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add five entries (2 full pages, one partial) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 4;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 3;

	/* Move one page up */
	ui_list_page_up(list);

	/* Page should now start at second entry and cursor at third */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Move one page up again. */
	ui_list_page_up(list);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	ui_list_page_up(list);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_EQUALS(list->cursor, list->page);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_page_up() moves one page down */
PCUT_TEST(page_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add five entries (2 full pages, one partial) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = list->cursor;
	list->page_idx = 0;

	/* Move one page down */
	ui_list_page_down(list);

	/* Page and cursor should point to the third entry */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("c", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->page_idx);

	/* Move one page down again. */
	ui_list_page_down(list);

	/* Cursor should point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(5, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(4, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(4, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(3, list->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	ui_list_page_down(list);

	/* Cursor should still point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(5, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(4, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(4, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(3, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scroll_up() scrolls up by one row */
PCUT_TEST(scroll_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry, page to the second */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 2;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 1;

	/* Scroll one entry up */
	ui_list_scroll_up(list);

	/* Page should start on the first entry, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Try scrolling one more entry up */
	ui_list_scroll_up(list);

	/* We were at the beginning, so nothing should have changed  */
	PCUT_ASSERT_STR_EQUALS("c", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scroll_down() scrolls down by one row */
PCUT_TEST(scroll_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add tree entries (more than page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = list->cursor;
	list->page_idx = 0;

	/* Scroll one entry down */
	ui_list_scroll_down(list);

	/* Page should start on the second entry, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Try scrolling one more entry down */
	ui_list_scroll_down(list);

	/* We were at the end, so nothing should have changed  */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scroll_page_up() scrolls up by one page */
PCUT_TEST(scroll_page_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry, page to the second last */
	list->cursor = ui_list_last(list);
	list->cursor_idx = 4;
	list->page = ui_list_prev(list->cursor);
	list->page_idx = 3;

	/* Scroll one page up */
	ui_list_scroll_page_up(list);

	/* Page should start on 'b', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("e", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(5, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(4, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Page up again */
	ui_list_scroll_page_up(list);

	/* Page should now be at the beginning, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("e", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(5, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(4, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	/* Page up again */
	ui_list_scroll_page_up(list);

	/* We were at the beginning, nothing should have changed */
	PCUT_ASSERT_STR_EQUALS("e", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(5, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(4, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scroll_page_up() scrolls down by one page */
PCUT_TEST(scroll_page_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = ui_list_first(list);
	list->page_idx = 0;

	/* Scroll one page down */
	ui_list_scroll_page_down(list);

	/* Page should start on 'c', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("c", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(3, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(2, list->page_idx);

	/* Page down again */
	ui_list_scroll_page_down(list);

	/* Page should now start at 'd', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(4, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(3, list->page_idx);

	/* Page down again */
	ui_list_scroll_page_down(list);

	/* We were at the end, nothing should have changed */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(4, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(3, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_scroll_pos() scrolls to a particular entry */
PCUT_TEST(scroll_pos)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_list_set_rect(list, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_list_page_size(list));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "c";
	attr.arg = (void *)3;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "d";
	attr.arg = (void *)4;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "e";
	attr.arg = (void *)5;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	list->page = ui_list_first(list);
	list->page_idx = 0;

	/* Scroll to entry 1 (one down) */
	ui_list_scroll_pos(list, 1);

	/* Page should start on 'b', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(2, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(1, list->page_idx);

	/* Scroll to entry 3 (i.e. the end) */
	ui_list_scroll_pos(list, 3);

	/* Page should now start at 'd', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", list->cursor->caption);
	PCUT_ASSERT_INT_EQUALS(1, (intptr_t)list->cursor->arg);
	PCUT_ASSERT_INT_EQUALS(0, list->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", list->page->caption);
	PCUT_ASSERT_INT_EQUALS(4, (intptr_t)list->page->arg);
	PCUT_ASSERT_INT_EQUALS(3, list->page_idx);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_activate_req() sends activation request */
PCUT_TEST(activate_req)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	resp.activate_req = false;
	resp.activate_req_list = NULL;

	ui_list_activate_req(list);
	PCUT_ASSERT_TRUE(resp.activate_req);
	PCUT_ASSERT_EQUALS(list, resp.activate_req_list);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_selected() runs selected callback */
PCUT_TEST(selected)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_attr_t attr;
	ui_list_entry_t *entry;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	ui_list_entry_attr_init(&attr);
	attr.caption = "Hello";
	attr.arg = &resp;

	rc = ui_list_entry_append(list, &attr, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.selected = false;
	resp.selected_entry = NULL;

	ui_list_selected(entry);
	PCUT_ASSERT_TRUE(resp.selected);
	PCUT_ASSERT_EQUALS(entry, resp.selected_entry);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_ptr_cmp compares two indirectly referenced entries */
PCUT_TEST(entry_ptr_cmp)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *a, *b;
	ui_list_entry_attr_t attr;
	test_resp_t resp;
	int rel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	a = ui_list_first(list);
	PCUT_ASSERT_NOT_NULL(a);
	b = ui_list_next(a);
	PCUT_ASSERT_NOT_NULL(b);

	/* a < b */
	rel = ui_list_entry_ptr_cmp(&a, &b);
	PCUT_ASSERT_TRUE(rel < 0);

	/* b > a */
	rel = ui_list_entry_ptr_cmp(&b, &a);
	PCUT_ASSERT_TRUE(rel > 0);

	/* a == a */
	rel = ui_list_entry_ptr_cmp(&a, &a);
	PCUT_ASSERT_INT_EQUALS(0, rel);

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_list_entry_get_idx() returns entry index */
PCUT_TEST(entry_get_idx)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_list_t *list;
	ui_list_entry_t *a, *b;
	ui_list_entry_attr_t attr;
	test_resp_t resp;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_list_create(window, true, &list);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_list_set_cb(list, &test_cb, &resp);

	ui_list_entry_attr_init(&attr);

	attr.caption = "a";
	attr.arg = (void *)2;
	rc = ui_list_entry_append(list, &attr, &a);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.caption = "b";
	attr.arg = (void *)1;
	rc = ui_list_entry_append(list, &attr, &b);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(0, ui_list_entry_get_idx(a));
	PCUT_ASSERT_INT_EQUALS(1, ui_list_entry_get_idx(b));

	ui_list_destroy(list);
	ui_window_destroy(window);
	ui_destroy(ui);
}

static void test_list_activate_req(ui_list_t *list, void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->activate_req = true;
	resp->activate_req_list = list;
}

static void test_list_selected(ui_list_entry_t *entry, void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->selected = true;
	resp->selected_entry = entry;
}

static int test_list_compare(ui_list_entry_t *a, ui_list_entry_t *b)
{
	return str_cmp(a->caption, b->caption);
}

PCUT_EXPORT(list);
