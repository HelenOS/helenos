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
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <pcut/pcut.h>
#include <stdio.h>
#include <ui/ui.h>
#include <vfs/vfs.h>
#include "../panel.h"

PCUT_INIT;

PCUT_TEST_SUITE(panel);

/** Create and destroy panel. */
PCUT_TEST(create_destroy)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
}

/** Test panel_entry_paint() */
PCUT_TEST(entry_paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_paint(panel_first(panel), 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test panel_paint() */
PCUT_TEST(paint)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_paint(panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_ctl() returns a valid UI control */
PCUT_TEST(ctl)
{
	panel_t *panel;
	ui_control_t *control;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = panel_ctl(panel);
	PCUT_ASSERT_NOT_NULL(control);

	panel_destroy(panel);
}

/** Test panel_kbd_event() */
PCUT_TEST(kbd_event)
{
	panel_t *panel;
	ui_evclaim_t claimed;
	kbd_event_t event;
	errno_t rc;

	/* Active panel should claim events */

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	event.c = '\0';

	claimed = panel_kbd_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	panel_destroy(panel);

	/* Inactive panel should not claim events */

	rc = panel_create(NULL, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ENTER;
	event.mods = 0;
	event.c = '\0';

	claimed = panel_kbd_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	panel_destroy(panel);
}

/** Test panel_pos_event() */
PCUT_TEST(pos_event)
{
	panel_t *panel;
	ui_evclaim_t claimed;
	pos_event_t event;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.pos_id = 0;
	event.type = POS_PRESS;
	event.btn_num = 1;
	event.hpos = 0;
	event.vpos = 0;

	claimed = panel_pos_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	panel_destroy(panel);
}

/** panel_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	panel_set_rect(panel, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, panel->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, panel->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, panel->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, panel->rect.p1.y);

	panel_destroy(panel);
}

/** panel_page_size() returns correct size */
PCUT_TEST(page_size)
{
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	panel_set_rect(panel, &rect);

	/* NOTE If page size changes, we have problems elsewhere in the tests */
	PCUT_ASSERT_INT_EQUALS(18, panel_page_size(panel));

	panel_destroy(panel);
}

/** panel_is_active() returns panel activity state */
PCUT_TEST(is_active)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(panel_is_active(panel));
	panel_destroy(panel);

	rc = panel_create(NULL, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(panel_is_active(panel));
	panel_destroy(panel);
}

/** panel_activate() activates panel */
PCUT_TEST(activate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, false, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(panel_is_active(panel));
	panel_activate(panel);
	PCUT_ASSERT_TRUE(panel_is_active(panel));

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_deactivate() deactivates panel */
PCUT_TEST(deactivate)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(panel_is_active(panel));
	panel_deactivate(panel);
	PCUT_ASSERT_FALSE(panel_is_active(panel));

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_entry_append() appends new entry */
PCUT_TEST(entry_append)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&panel->entries));

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&panel->entries));

	panel_destroy(panel);
}

/** panel_entry_delete() deletes entry */
PCUT_TEST(entry_delete)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&panel->entries));

	entry = panel_first(panel);
	panel_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&panel->entries));

	entry = panel_first(panel);
	panel_entry_delete(entry);

	PCUT_ASSERT_INT_EQUALS(0, list_count(&panel->entries));

	panel_destroy(panel);
}

/** panel_clear_entries() removes all entries from panel */
PCUT_TEST(clear_entries)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&panel->entries));

	panel_clear_entries(panel);
	PCUT_ASSERT_INT_EQUALS(0, list_count(&panel->entries));

	panel_destroy(panel);
}

/** panel_read_dir() reads the contents of a directory */
PCUT_TEST(read_dir)
{
	panel_t *panel;
	panel_entry_t *entry;
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

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_read_dir(panel, p);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&panel->entries));

	entry = panel_first(panel);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	// PCUT_ASSERT_INT_EQUALS(1, entry->size);

	panel_destroy(panel);

	rv = remove(fname);
	PCUT_ASSERT_INT_EQUALS(0, rv);

	rv = remove(p);
	PCUT_ASSERT_INT_EQUALS(0, rv);
	free(fname);
}

/** panel_first() returns valid entry or @c NULL as appropriate */
PCUT_TEST(first)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = panel_first(panel);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = panel_first(panel);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	/* Add another entry */
	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should still get the first entry */
	entry = panel_first(panel);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	panel_destroy(panel);
}

/** panel_last() returns valid entry or @c NULL as appropriate */
PCUT_TEST(last)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	entry = panel_last(panel);
	PCUT_ASSERT_NULL(entry);

	/* Add one entry */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting it */
	entry = panel_last(panel);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	/* Add another entry */
	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* We should get new entry now */
	entry = panel_last(panel);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, entry->size);

	panel_destroy(panel);
}

/** panel_next() returns the next entry or @c NULL as appropriate */
PCUT_TEST(next)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add one entry */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its successor */
	entry = panel_first(panel);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = panel_next(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the successor of first entry again */
	entry = panel_first(panel);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = panel_next(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("b", entry->name);
	PCUT_ASSERT_INT_EQUALS(2, entry->size);

	panel_destroy(panel);
}

/** panel_prev() returns the previous entry or @c NULL as appropriate */
PCUT_TEST(prev)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add one entry */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now try getting its predecessor */
	entry = panel_last(panel);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = panel_prev(entry);
	PCUT_ASSERT_NULL(entry);

	/* Add another entry */
	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try getting the predecessor of the new entry */
	entry = panel_last(panel);
	PCUT_ASSERT_NOT_NULL(entry);

	entry = panel_prev(entry);
	PCUT_ASSERT_NOT_NULL(entry);
	PCUT_ASSERT_STR_EQUALS("a", entry->name);
	PCUT_ASSERT_INT_EQUALS(1, entry->size);

	panel_destroy(panel);
}

/** panel_cursor_move() ... */
PCUT_TEST(cursor_move)
{
}

/** panel_cursor_up() moves cursor one entry up */
PCUT_TEST(cursor_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add tree entries (more than page size, which is 2) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	panel->cursor = panel_last(panel);
	panel->cursor_idx = 2;
	panel->page = panel_prev(panel->cursor);
	panel->page_idx = 1;

	/* Move cursor one entry up */
	panel_cursor_up(panel);

	/* Cursor and page start should now both be at the second entry */
	PCUT_ASSERT_STR_EQUALS("b", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(1, panel->page_idx);

	/* Move cursor one entry up. This should scroll up. */
	panel_cursor_up(panel);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	panel_cursor_up(panel);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_cursor_down() moves cursor one entry down */
PCUT_TEST(cursor_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add tree entries (more than page size, which is 2) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	panel->cursor = panel_first(panel);
	panel->cursor_idx = 0;
	panel->page = panel->cursor;
	panel->page_idx = 0;

	/* Move cursor one entry down */
	panel_cursor_down(panel);

	/* Cursor should now be at the second entry, page stays the same */
	PCUT_ASSERT_STR_EQUALS("b", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel_first(panel), panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	/* Move cursor one entry down. This should scroll down. */
	panel_cursor_down(panel);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	panel_cursor_down(panel);

	/* Cursor should still be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_cursor_top() moves cursor to the first entry */
PCUT_TEST(cursor_top)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add tree entries (more than page size, which is 2) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	panel->cursor = panel_last(panel);
	panel->cursor_idx = 2;
	panel->page = panel_prev(panel->cursor);
	panel->page_idx = 1;

	/* Move cursor to the top. This should scroll up. */
	panel_cursor_top(panel);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_cursor_bottom() moves cursor to the last entry */
PCUT_TEST(cursor_bottom)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add tree entries (more than page size, which is 2) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page start to the first entry */
	panel->cursor = panel_first(panel);
	panel->cursor_idx = 0;
	panel->page = panel->cursor;
	panel->page_idx = 0;

	/* Move cursor to the bottom. This should scroll down. */
	panel_cursor_bottom(panel);

	/* Cursor should now be at the third and page at the second entry. */
	PCUT_ASSERT_STR_EQUALS("c", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_page_up() moves one page up */
PCUT_TEST(page_up)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add five entries (2 full pages, one partial) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "d", 4);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "e", 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor to the last entry and page start to the next-to-last entry */
	panel->cursor = panel_last(panel);
	panel->cursor_idx = 4;
	panel->page = panel_prev(panel->cursor);
	panel->page_idx = 3;

	/* Move one page up */
	panel_page_up(panel);

	/* Page should now start at second entry and cursor at third */
	PCUT_ASSERT_STR_EQUALS("c", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("b", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(2, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(1, panel->page_idx);

	/* Move one page up again. */
	panel_page_up(panel);

	/* Cursor and page start should now both be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	/* Moving further up should do nothing (we are at the top). */
	panel_page_up(panel);

	/* Cursor and page start should still be at the first entry */
	PCUT_ASSERT_STR_EQUALS("a", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(1, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(0, panel->cursor_idx);
	PCUT_ASSERT_EQUALS(panel->cursor, panel->page);
	PCUT_ASSERT_INT_EQUALS(0, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** panel_page_up() moves one page down */
PCUT_TEST(page_down)
{
	ui_t *ui;
	ui_window_t *window;
	ui_wnd_params_t params;
	panel_t *panel;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Test";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_create(window, true, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 10;
	rect.p1.y = 4; // XXX Assuming this makes page size 2
	panel_set_rect(panel, &rect);

	PCUT_ASSERT_INT_EQUALS(2, panel_page_size(panel));

	/* Add five entries (2 full pages, one partial) */
	rc = panel_entry_append(panel, "a", 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "b", 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "c", 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "d", 4);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = panel_entry_append(panel, "e", 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Cursor and page to the first entry */
	panel->cursor = panel_first(panel);
	panel->cursor_idx = 0;
	panel->page = panel->cursor;
	panel->page_idx = 0;

	/* Move one page down */
	panel_page_down(panel);

	/* Page and cursor should point to the third entry */
	PCUT_ASSERT_STR_EQUALS("c", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("c", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(3, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(2, panel->page_idx);

	/* Move one page down again. */
	panel_page_down(panel);

	/* Cursor should point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(4, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(3, panel->page_idx);

	/* Moving further down should do nothing (we are at the bottom). */
	panel_page_down(panel);

	/* Cursor should still point to last and page to next-to-last entry */
	PCUT_ASSERT_STR_EQUALS("e", panel->cursor->name);
	PCUT_ASSERT_INT_EQUALS(5, panel->cursor->size);
	PCUT_ASSERT_INT_EQUALS(4, panel->cursor_idx);
	PCUT_ASSERT_STR_EQUALS("d", panel->page->name);
	PCUT_ASSERT_INT_EQUALS(4, panel->page->size);
	PCUT_ASSERT_INT_EQUALS(3, panel->page_idx);

	panel_destroy(panel);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(panel);
