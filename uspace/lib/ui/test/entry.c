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
	ui_entry_t *entry;
	errno_t rc;

	rc = ui_entry_create(NULL, "Hello", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_entry_set_halign(entry, gfx_halign_left);
	PCUT_ASSERT_EQUALS(gfx_halign_left, entry->halign);
	ui_entry_set_halign(entry, gfx_halign_center);
	PCUT_ASSERT_EQUALS(gfx_halign_center, entry->halign);

	ui_entry_destroy(entry);
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

	rc = ui_entry_insert_str(entry, "B");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("AB", entry->text);

	rc = ui_entry_insert_str(entry, "CD");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABCD", entry->text);

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

	rc = ui_entry_create(window, "ABC", &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_STR_EQUALS("ABC", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("AB", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("A", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("", entry->text);

	ui_entry_backspace(entry);
	PCUT_ASSERT_STR_EQUALS("", entry->text);

	ui_entry_destroy(entry);
	ui_window_destroy(window);
	ui_destroy(ui);
}

PCUT_EXPORT(entry);
