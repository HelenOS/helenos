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

#include <clipboard.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/entry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/entry.h"

PCUT_INIT;

PCUT_TEST_SUITE(entry);

/** Create and destroy text entry */
PCUT_TEST(create_destroy)
{
	ui_entry_t *entry = NULL;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	ui_entry_destroy(entry);
}

/** ui_entry_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_entry_destroy(NULL);
}

/** ui_entry_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_entry_t *entry;
	ui_control_t *control;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_entry_ctl(entry);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set text entry rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_entry_t *entry;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_entry_set_rect(entry, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, entry->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, entry->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, entry->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, entry->rect.p1.y);

	ui_entry_destroy(entry);
}

/** Set entry text horizontal alignment sets internal field */
PCUT_TEST(set_halign)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_set_halign(entry, gfx_halign_left);
	PCUT_ASSERT_EQUALS(gfx_halign_left, entry->halign);
	ui_entry_set_halign(entry, gfx_halign_center);
	PCUT_ASSERT_EQUALS(gfx_halign_center, entry->halign);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Set entry read only flag sets internal field */
PCUT_TEST(set_read_only)
{
	ui_entry_t *entry;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_set_read_only(entry, true);
	PCUT_ASSERT_TRUE(entry->read_only);
	ui_entry_set_read_only(entry, false);
	PCUT_ASSERT_FALSE(entry->read_only);

	ui_entry_destroy(entry);
}

/** Set text entry rectangle sets internal field */
PCUT_TEST(set_text)
{
	ui_entry_t *entry;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_entry_set_rect(entry, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, entry->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, entry->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, entry->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, entry->rect.p1.y);

	ui_entry_destroy(entry);
}

/** Paint text entry */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_entry_paint(entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_delete_sel() deletes selected text */
PCUT_TEST(delete_sel)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDEF", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCDEF", entry->text);

	ui_entry_activate(entry);

	/* Select all but first and last character */
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	ui_entry_delete_sel(entry);

	PCUT_ASSERT_STR_EQUALS("AF", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_insert_str() inserts string at cursor. */
PCUT_TEST(insert_str)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "A", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("A", entry->text);

	ui_entry_activate(entry);
	ui_entry_seek_end(entry, false);

	rc = ui_entry_insert_str(entry, "B");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("AB", entry->text);

	rc = ui_entry_insert_str(entry, "EF");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABEF", entry->text);

	entry->pos = 2;
	entry->sel_start = 2;
	rc = ui_entry_insert_str(entry, "CD");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCDEF", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_insert_str() deletes selection before inserting string */
PCUT_TEST(insert_str_with_sel)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDE", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCDE", entry->text);

	/* Select all but the first and last character */
	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	rc = ui_entry_insert_str(entry, "123");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("A123E", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_backspace() deletes character before cursor. */
PCUT_TEST(backspace)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCD", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);
	entry->pos = 3;
	entry->sel_start = 3;

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("ABD", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("AD", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("D", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("D", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_backspace() with selected text deletes selection. */
PCUT_TEST(backspace_with_sel)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDE", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCDE", entry->text);

	/* Select all but the first and last character */
	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	ui_entry_backspace(entry);

	PCUT_ASSERT_STR_EQUALS("AE", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_delete() deletes character after cursor. */
PCUT_TEST(delete)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCD", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);
	entry->pos = 1;
	entry->sel_start = 1;

	ui_entry_delete(entry);
	PCUT_ASSERT_STR_EQUALS("ACD", entry->text);

	ui_entry_delete(entry);
	PCUT_ASSERT_STR_EQUALS("AD", entry->text);

	ui_entry_delete(entry);
	PCUT_ASSERT_STR_EQUALS("A", entry->text);

	ui_entry_delete(entry);
	PCUT_ASSERT_STR_EQUALS("A", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_delete() with selected text deletes selection. */
PCUT_TEST(delete_with_sel)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDE", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCDE", entry->text);

	/* Select all but the first and last character */
	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	ui_entry_delete(entry);

	PCUT_ASSERT_STR_EQUALS("AE", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_copy() copies selected text to clipboard. */
PCUT_TEST(copy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;
	char *str;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDEF", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	// FIXME: This is not safe unless we could create a private
	// test clipboard

	ui_entry_copy(entry);
	rc = clipboard_get_str(&str);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("BCDE", str);
	PCUT_ASSERT_STR_EQUALS("ABCDEF", entry->text);
	free(str);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_cut() cuts selected text to clipboard. */
PCUT_TEST(cut)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;
	char *str;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDEF", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);
	ui_entry_seek_end(entry, true);
	ui_entry_seek_prev_char(entry, true);

	// FIXME: This is not safe unless we could create a private
	// test clipboard

	ui_entry_cut(entry);
	rc = clipboard_get_str(&str);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("BCDE", str);
	PCUT_ASSERT_STR_EQUALS("AF", entry->text);
	free(str);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_paste() pastes text from clipboard. */
PCUT_TEST(paste)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "AB", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_activate(entry);
	ui_entry_seek_start(entry, false);
	ui_entry_seek_next_char(entry, false);

	// FIXME: This is not safe unless we could create a private
	// test clipboard

	rc = clipboard_put_str("123");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_paste(entry);
	PCUT_ASSERT_STR_EQUALS("A123B", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_seek_start() moves cursor to beginning of text */
PCUT_TEST(seek_start)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCDEF", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_activate(entry);

	entry->pos = 2;
	entry->sel_start = 2;

	ui_entry_seek_start(entry, true);
	PCUT_ASSERT_INT_EQUALS(0, entry->pos);
	PCUT_ASSERT_INT_EQUALS(2, entry->sel_start);

	ui_entry_seek_start(entry, false);
	PCUT_ASSERT_INT_EQUALS(0, entry->pos);
	PCUT_ASSERT_INT_EQUALS(0, entry->sel_start);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_seek_end() moves cursor to the end of text */
PCUT_TEST(seek_end)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCD", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);
	entry->pos = 2;
	entry->sel_start = 2;

	ui_entry_seek_end(entry, true);
	PCUT_ASSERT_INT_EQUALS(4, entry->pos);
	PCUT_ASSERT_INT_EQUALS(2, entry->sel_start);
	ui_entry_seek_end(entry, false);
	PCUT_ASSERT_INT_EQUALS(4, entry->pos);
	PCUT_ASSERT_INT_EQUALS(4, entry->sel_start);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_seek_prev_char() moves cursor to the previous character */
PCUT_TEST(seek_prev_char)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCD", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);
	entry->pos = 3;
	entry->sel_start = 3;

	ui_entry_seek_prev_char(entry, true);
	PCUT_ASSERT_INT_EQUALS(2, entry->pos);
	PCUT_ASSERT_INT_EQUALS(3, entry->sel_start);

	ui_entry_seek_prev_char(entry, false);
	PCUT_ASSERT_INT_EQUALS(1, entry->pos);
	PCUT_ASSERT_INT_EQUALS(1, entry->sel_start);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_seek_prev_char() moves cursor to the next character */
PCUT_TEST(seek_next_char)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABCD", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);
	entry->pos = 1;
	entry->sel_start = 1;

	ui_entry_seek_next_char(entry, true);
	PCUT_ASSERT_INT_EQUALS(2, entry->pos);
	PCUT_ASSERT_INT_EQUALS(1, entry->sel_start);
	ui_entry_seek_next_char(entry, false);
	PCUT_ASSERT_INT_EQUALS(3, entry->pos);
	PCUT_ASSERT_INT_EQUALS(3, entry->sel_start);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_entry_activate() / ui_entry_deactivate() */
PCUT_TEST(activate_deactivate)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_window_t *window = NULL;
	ui_wnd_params_t params;
	ui_entry_t *entry;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_entry_create(window, "ABC", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(entry->active);

	ui_entry_activate(entry);
	PCUT_ASSERT_TRUE(entry->active);

	ui_entry_deactivate(entry);
	PCUT_ASSERT_FALSE(entry->active);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(entry);
