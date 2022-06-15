/*
 * Copyright (c) 2022 Jiri Svoboda
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
#include <ui/ui.h>
#include <ui/filelist.h>
#include <ui/scrollbar.h>
#include <vfs/vfs.h>
#include "../private/filelist.h"

PCUT_INIT;

PCUT_TEST_SUITE(file_list);

/** Test response */
typedef struct {
	bool activate_req;
	ui_file_list_t *activate_req_file_list;

	bool selected;
	ui_file_list_t *selected_file_list;
	const char *selected_fname;
} test_resp_t;

static void test_file_list_activate_req(ui_file_list_t *, void *);
static void test_file_list_selected(ui_file_list_t *, void *, const char *);

static ui_file_list_cb_t test_cb = {
	.activate_req = test_file_list_activate_req,
	.selected = test_file_list_selected
};

/** Create and destroy file list. */
PCUT_TEST(create_destroy)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_set_cb() sets callback */
PCUT_TEST(set_cb)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);
	PCUT_ASSERT_EQUALS(&test_cb, flist->cb);
	PCUT_ASSERT_EQUALS(&resp, flist->cb_arg);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_entry_height() gives the correct height */
PCUT_TEST(entry_height)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;
	gfx_coord_t height;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Font height is 13, padding: 2 (top) + 2 (bottom) */
	height = ui_file_list_entry_height(flist);
	PCUT_ASSERT_INT_EQUALS(17, height);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_file_list_entry_paint() */
PCUT_TEST(entry_paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);
	attr.name = "a";
	attr.size = 1;

	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_entry_paint(ui_file_list_first(flist), 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_file_list_paint() */
PCUT_TEST(paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_paint(flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_ctl() returns a valid UI control */
PCUT_TEST(ctl)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_control_t *control;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_file_list_ctl(flist);
	PCUT_ASSERT_NOT_NULL(control);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_file_list_kbd_event() */
PCUT_TEST(kbd_event)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_evclaim_t claimed;
	kbd_event_t event;
	errno_t rc;

	/* Active file list should claim events */

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_file_list_kbd_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_file_list_destroy(flist);

	/* Inactive file list should not claim events */

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, false, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_file_list_kbd_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_file_list_pos_event() */
PCUT_TEST(pos_event)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_evclaim_t claimed;
	pos_event_t event;
	gfx_rect_t rect;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_file_list_set_rect(flist, &rect);

	ui_file_list_entry_attr_init(&attr);
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = ui_file_list_first(flist);
	flist->page_idx = 0;

	event.pos_id = 0;
	event.type = POS_PRESS;
	event.btn_num = 1;

	/* Clicking on the middle entry should select it */
	event.hpos = 20;
	event.vpos = 40;

	claimed = ui_file_list_pos_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	PCUT_ASSERT_NOT_NULL(flist->cursor);
	PCUT_ASSERT_STR_EQUALS("b", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor->size);

	/* Clicking on the top edge should do a page-up */
	event.hpos = 20;
	event.vpos = 20;
	claimed = ui_file_list_pos_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	PCUT_ASSERT_NOT_NULL(flist->cursor);
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_file_list_set_rect(flist, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, flist->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, flist->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, flist->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, flist->rect.p1.y);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_page_size() returns correct size */
PCUT_TEST(page_size)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_file_list_set_rect(flist, &rect);

	/* NOTE If page size changes, we have problems elsewhere in the tests */
	PCUT_ASSERT_INT_EQUALS(11, ui_file_list_page_size(flist));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_inside_rect() gives correct interior rectangle */
PCUT_TEST(inside_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	gfx_rect_t rect;
	gfx_rect_t irect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_file_list_set_rect(flist, &rect);

	ui_file_list_inside_rect(flist, &irect);
	PCUT_ASSERT_INT_EQUALS(10 + 2, irect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20 + 2, irect.p0.y);
	PCUT_ASSERT_INT_EQUALS(50 - 2 - 23, irect.p1.x);
	PCUT_ASSERT_INT_EQUALS(220 - 2, irect.p1.y);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scrollbar_rect() gives correct scrollbar rectangle */
PCUT_TEST(scrollbar_rect)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	gfx_rect_t rect;
	gfx_rect_t srect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 50;
	rect.p1.y = 220;

	ui_file_list_set_rect(flist, &rect);

	ui_file_list_scrollbar_rect(flist, &srect);
	PCUT_ASSERT_INT_EQUALS(50 - 2 - 23, srect.p0.x);
	PCUT_ASSERT_INT_EQUALS(20 + 2, srect.p0.y);
	PCUT_ASSERT_INT_EQUALS(50 - 2, srect.p1.x);
	PCUT_ASSERT_INT_EQUALS(220 - 2, srect.p1.y);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scrollbar_update() updates scrollbar position */
PCUT_TEST(scrollbar_update)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	gfx_rect_t rect;
	ui_file_list_entry_attr_t attr;
	ui_file_list_entry_t *entry;
	gfx_coord_t pos;
	gfx_coord_t move_len;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 50;
	rect.p1.y = 38;

	ui_file_list_set_rect(flist, &rect);

	ui_file_list_entry_attr_init(&attr);
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = ui_file_list_next(ui_file_list_first(flist));

	flist->cursor = entry;
	flist->cursor_idx = 1;
	flist->page = entry;
	flist->page_idx = 1;

	ui_file_list_scrollbar_update(flist);

	/* Now scrollbar thumb should be all the way down */
	move_len = ui_scrollbar_move_length(flist->scrollbar);
	pos = ui_scrollbar_get_pos(flist->scrollbar);
	PCUT_ASSERT_INT_EQUALS(move_len, pos);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_is_active() returns file list activity state */
PCUT_TEST(is_active)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(ui_file_list_is_active(flist));
	ui_file_list_destroy(flist);

	rc = ui_file_list_create(window, false, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(ui_file_list_is_active(flist));
	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_activate() activates file list */
PCUT_TEST(activate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, false, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(ui_file_list_is_active(flist));
	rc = ui_file_list_activate(flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(ui_file_list_is_active(flist));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_deactivate() deactivates file list */
PCUT_TEST(deactivate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(ui_file_list_is_active(flist));
	ui_file_list_deactivate(flist);
	PCUT_ASSERT_FALSE(ui_file_list_is_active(flist));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_entry_append() appends new entry */
PCUT_TEST(entry_append)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&flist->entries));

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&flist->entries));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_entry_delete() deletes entry */
PCUT_TEST(entry_delete)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&flist->entries));

	entry = ui_file_list_first(flist);
	ui_file_list_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&flist->entries));

	entry = ui_file_list_first(flist);
	ui_file_list_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(0, list_count(&flist->entries));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_clear_entries() removes all entries from file list */
PCUT_TEST(clear_entries)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "a";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&flist->entries));

	ui_file_list_clear_entries(flist);
	PCUT_ASSERT_INT_EQUALS(0, list_count(&flist->entries));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_read_dir() reads the contents of a directory */
PCUT_TEST(read_dir)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	char buf[L_tmpnam];
	char *fname;
	char *p;
	errno_t rc;
	FILE *f;
	int rv;

	/* Create name for temporary directory */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	/* Create temporary directory */
	rc = vfs_link_path(p, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = asprintf(&fname, "%s/%s", p, "a");
	PCUT_ASSERT_TRUE(rv >= 0);

	f = fopen(fname, "wb");
	PCUT_ASSERT_NOT_NULL(f);

	rv = fprintf(f, "X");
	PCUT_ASSERT_TRUE(rv >= 0);

	rv = fclose(f);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_read_dir(flist, p);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&flist->entries));

	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("..", entry->name);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	ui_file_list_destroy(flist);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(fname);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** When moving to parent directory from a subdir, we seek to the
 * coresponding entry
 */
PCUT_TEST(read_dir_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	char buf[L_tmpnam];
	char *subdir_a;
	char *subdir_b;
	char *subdir_c;
	char *p;
	errno_t rc;
	int rv;

	/* Create name for temporary directory */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	/* Create temporary directory */
	rc = vfs_link_path(p, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create some subdirectories */

	rv = asprintf(&subdir_a, "%s/%s", p, "a");
	PCUT_ASSERT_TRUE(rv >= 0);
	rc = vfs_link_path(subdir_a, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = asprintf(&subdir_b, "%s/%s", p, "b");
	PCUT_ASSERT_TRUE(rv >= 0);
	rc = vfs_link_path(subdir_b, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = asprintf(&subdir_c, "%s/%s", p, "c");
	PCUT_ASSERT_TRUE(rv >= 0);
	rc = vfs_link_path(subdir_c, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Start in subdirectory "b" */
	rc = ui_file_list_read_dir(flist, subdir_b);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now go up (into p) */

	rc = ui_file_list_read_dir(flist, "..");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NOT_NULL(flist->cursor);
	PCUT_ASSERT_STR_EQUALS("b", flist->cursor->name);

	ui_file_list_destroy(flist);

	rv = remove(subdir_a);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(subdir_b);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(subdir_c);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(subdir_a);
	free(subdir_b);
	free(subdir_c);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_sort() sorts file list entries */
PCUT_TEST(sort)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	attr.name = "b";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "a";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_sort(flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = ui_file_list_first(flist);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, entry->size);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_STR_EQUALS("c", entry->name);
	PCUT_ASSERT_INT_EQUALS(3, entry->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_entry_ptr_cmp compares two indirectly referenced entries */
PCUT_TEST(entry_ptr_cmp)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *a, *b;
	ui_file_list_entry_attr_t attr;
	int rel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	a = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(a);
	b = ui_file_list_next(a);
	PCUT_ASSERT_NOT_NULL(b);

	/* a < b */
	rel = ui_file_list_entry_ptr_cmp(&a, &b);
	PCUT_ASSERT_TRUE(rel < 0);

	/* b > a */
	rel = ui_file_list_entry_ptr_cmp(&b, &a);
	PCUT_ASSERT_TRUE(rel > 0);

	/* a == a */
	rel = ui_file_list_entry_ptr_cmp(&a, &a);
	PCUT_ASSERT_INT_EQUALS(0, rel);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_first() returns valid entry or @c NULL as appropriate */
PCUT_TEST(first)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	/* Add another entry */
	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should still get the first entry */
	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_last() returns valid entry or @c NULL as appropriate */
PCUT_TEST(last)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	entry = ui_file_list_last(flist);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = ui_file_list_last(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	/* Add another entry */
	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should get new entry now */
	entry = ui_file_list_last(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	attr.name = "b";
	attr.size = 2;
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, entry->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_next() returns the next entry or @c NULL as appropriate */
PCUT_TEST(next)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	/* Add one entry */
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its successor */
	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the successor of first entry again */
	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, entry->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_prev() returns the previous entry or @c NULL as appropriate */
PCUT_TEST(prev)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	/* Add one entry */
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its predecessor */
	entry = ui_file_list_last(flist);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_file_list_prev(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the predecessor of the new entry */
	entry = ui_file_list_last(flist);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = ui_file_list_prev(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_page_nth_entry() .. */
PCUT_TEST(page_nth_entry)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	size_t idx;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_entry_attr_init(&attr);

	/* Add some entries */
	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	flist->page = ui_file_list_next(ui_file_list_first(flist));
	flist->page_idx = 1;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 100;
	rect.p1.y = 100;
	ui_file_list_set_rect(flist, &rect);

	entry = ui_file_list_page_nth_entry(flist, 0, &idx);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, idx);

	entry = ui_file_list_page_nth_entry(flist, 1, &idx);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("c", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, idx);

	entry = ui_file_list_page_nth_entry(flist, 2, &idx);
	PCUT_ASSERT_NULL(entry);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_cursor_move() moves cursor and scrolls */
PCUT_TEST(cursor_move)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;
	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 2;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 1;

	/* Move cursor one entry up */
	ui_file_list_cursor_move(flist, ui_file_list_prev(flist->cursor),
	    flist->cursor_idx - 1);

	/* Cursor and page start should now both be at the second entry */
	PCUT_ASSERT_STR_EQUALS("b", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Move cursor to the first entry. This should scroll up. */
	ui_file_list_cursor_move(flist, ui_file_list_first(flist), 0);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Move cursor to the last entry. */
	ui_file_list_cursor_move(flist, ui_file_list_last(flist), 2);

	/* Cursor should be on the last entry and page on the next to last */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_cursor_up() moves cursor one entry up */
PCUT_TEST(cursor_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;
	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 2;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 1;

	/* Move cursor one entry up */
	ui_file_list_cursor_up(flist);

	/* Cursor and page start should now both be at the second entry */
	PCUT_ASSERT_STR_EQUALS("b", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Move cursor one entry up. This should scroll up. */
	ui_file_list_cursor_up(flist);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	ui_file_list_cursor_up(flist);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_cursor_down() moves cursor one entry down */
PCUT_TEST(cursor_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = flist->cursor;
	flist->page_idx = 0;

	/* Move cursor one entry down */
	ui_file_list_cursor_down(flist);

	/* Cursor should now be at the second entry, page stays the same */
	PCUT_ASSERT_STR_EQUALS("b", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(ui_file_list_first(flist), flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Move cursor one entry down. This should scroll down. */
	ui_file_list_cursor_down(flist);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	ui_file_list_cursor_down(flist);

	/* Cursor should still be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_cursor_top() moves cursor to the first entry */
PCUT_TEST(cursor_top)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 2;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 1;

	/* Move cursor to the top. This should scroll up. */
	ui_file_list_cursor_top(flist);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_cursor_bottom() moves cursor to the last entry */
PCUT_TEST(cursor_bottom)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = flist->cursor;
	flist->page_idx = 0;

	/* Move cursor to the bottom. This should scroll down. */
	ui_file_list_cursor_bottom(flist);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_page_up() moves one page up */
PCUT_TEST(page_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add five entries (2 full pages, one partial) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "d";
	attr.size = 4;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "e";
	attr.size = 5;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 4;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 3;

	/* Move one page up */
	ui_file_list_page_up(flist);

	/* Page should now start at second entry and cursor at third */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Move one page up again. */
	ui_file_list_page_up(flist);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	ui_file_list_page_up(flist);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_EQUALS(flist->cursor, flist->page);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_page_up() moves one page down */
PCUT_TEST(page_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add five entries (2 full pages, one partial) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "d";
	attr.size = 4;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "e";
	attr.size = 5;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = flist->cursor;
	flist->page_idx = 0;

	/* Move one page down */
	ui_file_list_page_down(flist);

	/* Page and cursor should point to the third entry */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("c", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->page_idx);

	/* Move one page down again. */
	ui_file_list_page_down(flist);

	/* Cursor should point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(4, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(3, flist->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	ui_file_list_page_down(flist);

	/* Cursor should still point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(4, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(3, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scroll_up() scrolls up by one row */
PCUT_TEST(scroll_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry, page to the second */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 2;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 1;

	/* Scroll one entry up */
	ui_file_list_scroll_up(flist);

	/* Page should start on the first entry, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Try scrolling one more entry up */
	ui_file_list_scroll_up(flist);

	/* We were at the beginning, so nothing should have changed  */
	PCUT_ASSERT_STR_EQUALS("c", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scroll_down() scrolls down by one row */
PCUT_TEST(scroll_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add tree entries (more than page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = flist->cursor;
	flist->page_idx = 0;

	/* Scroll one entry down */
	ui_file_list_scroll_down(flist);

	/* Page should start on the second entry, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Try scrolling one more entry down */
	ui_file_list_scroll_down(flist);

	/* We were at the end, so nothing should have changed  */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scroll_page_up() scrolls up by one page */
PCUT_TEST(scroll_page_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "d";
	attr.size = 4;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "e";
	attr.size = 5;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry, page to the second last */
	flist->cursor = ui_file_list_last(flist);
	flist->cursor_idx = 4;
	flist->page = ui_file_list_prev(flist->cursor);
	flist->page_idx = 3;

	/* Scroll one page up */
	ui_file_list_scroll_page_up(flist);

	/* Page should start on 'b', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("e", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Page up again */
	ui_file_list_scroll_page_up(flist);

	/* Page should now be at the beginning, cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("e", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	/* Page up again */
	ui_file_list_scroll_page_up(flist);

	/* We were at the beginning, nothing should have changed */
	PCUT_ASSERT_STR_EQUALS("e", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("a", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_scroll_page_up() scrolls down by one page */
PCUT_TEST(scroll_page_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "d";
	attr.size = 4;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "e";
	attr.size = 5;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = ui_file_list_first(flist);
	flist->page_idx = 0;

	/* Scroll one page down */
	ui_file_list_scroll_page_down(flist);

	/* Page should start on 'c', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("c", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(3, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(2, flist->page_idx);

	/* Page down again */
	ui_file_list_scroll_page_down(flist);

	/* Page should now start at 'd', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(4, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(3, flist->page_idx);

	/* Page down again */
	ui_file_list_scroll_page_down(flist);

	/* We were at the end, nothing should have changed */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(4, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(3, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_TEST(scroll_pos)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 38; /* Assuming this makes page size 2 */
	ui_file_list_set_rect(flist, &rect);

	PCUT_ASSERT_INT_EQUALS(2, ui_file_list_page_size(flist));

	/* Add five entries (more than twice the page size, which is 2) */

	ui_file_list_entry_attr_init(&attr);

	attr.name = "a";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "b";
	attr.size = 2;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "c";
	attr.size = 3;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "d";
	attr.size = 4;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	attr.name = "e";
	attr.size = 5;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = ui_file_list_first(flist);
	flist->page_idx = 0;

	/* Scroll to entry 1 (one down) */
	ui_file_list_scroll_pos(flist, 1);

	/* Page should start on 'b', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(2, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(1, flist->page_idx);

	/* Scroll to entry 3 (i.e. the end) */
	ui_file_list_scroll_pos(flist, 3);

	/* Page should now start at 'd', cursor unchanged */
	PCUT_ASSERT_STR_EQUALS("a", flist->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, flist->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, flist->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", flist->page->name);
	PCUT_ASSERT_INT_EQUALS(4, flist->page->size);
	PCUT_ASSERT_INT_EQUALS(3, flist->page_idx);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_open_dir() opens a directory entry */
PCUT_TEST(open_dir)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	char buf[L_tmpnam];
	char *sdname;
	char *p;
	errno_t rc;
	int rv;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Create name for temporary directory */
	p = tmpnam(buf);
	PCUT_ASSERT_NOT_NULL(p);

	/* Create temporary directory */
	rc = vfs_link_path(p, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rv = asprintf(&sdname, "%s/%s", p, "a");
	PCUT_ASSERT_TRUE(rv >= 0);

	/* Create sub-directory */
	rc = vfs_link_path(sdname, KIND_DIRECTORY, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_read_dir(flist, p);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS(p, flist->dir);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&flist->entries));

	entry = ui_file_list_first(flist);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("..", entry->name);

	entry = ui_file_list_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_TRUE(entry->isdir);

	rc = ui_file_list_open_dir(flist, entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS(sdname, flist->dir);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);

	rv = remove(sdname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	free(sdname);
}

/** ui_file_list_open_file() runs selected callback */
PCUT_TEST(open_file)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);

	attr.name = "hello.txt";
	attr.size = 1;
	rc = ui_file_list_entry_append(flist, &attr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.selected = false;
	resp.selected_file_list = NULL;
	resp.selected_fname = NULL;

	ui_file_list_open_file(flist, ui_file_list_first(flist));
	PCUT_ASSERT_TRUE(resp.selected);
	PCUT_ASSERT_EQUALS(flist, resp.selected_file_list);
	PCUT_ASSERT_STR_EQUALS("hello.txt", resp.selected_fname);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_activate_req() sends activation request */
PCUT_TEST(activate_req)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);

	resp.activate_req = false;
	resp.activate_req_file_list = NULL;

	ui_file_list_activate_req(flist);
	PCUT_ASSERT_TRUE(resp.activate_req);
	PCUT_ASSERT_EQUALS(flist, resp.activate_req_file_list);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_selected() runs selected callback */
PCUT_TEST(selected)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_file_list_create(window, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);

	resp.selected = false;
	resp.selected_file_list = NULL;
	resp.selected_fname = NULL;

	ui_file_list_selected(flist, "hello.txt");
	PCUT_ASSERT_TRUE(resp.selected);
	PCUT_ASSERT_EQUALS(flist, resp.selected_file_list);
	PCUT_ASSERT_STR_EQUALS("hello.txt", resp.selected_fname);

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

static void test_file_list_activate_req(ui_file_list_t *flist, void *arg)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->activate_req = true;
	resp->activate_req_file_list = flist;
}

static void test_file_list_selected(ui_file_list_t *flist, void *arg,
    const char *fname)
{
	test_resp_t *resp = (test_resp_t *)arg;

	resp->selected = true;
	resp->selected_file_list = flist;
	resp->selected_fname = fname;
}

PCUT_EXPORT(file_list);
