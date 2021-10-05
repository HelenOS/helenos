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
#include "../panel.h"

PCUT_INIT;

PCUT_TEST_SUITE(panel);

/** Create and destroy panel. */
PCUT_TEST(create_destroy)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	panel_destroy(panel);
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

	rc = panel_create(window, &panel);
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

	rc = panel_create(NULL, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = panel_ctl(panel);
	PCUT_ASSERT_NOT_NULL(control);

	panel_destroy(panel);
}

/** Test panel_pos_event() */
PCUT_TEST(pos_event)
{
	panel_t *panel;
	ui_control_t *control;
	ui_evclaim_t claimed;
	pos_event_t event;
	errno_t rc;

	rc = panel_create(NULL, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = panel_ctl(panel);
	PCUT_ASSERT_NOT_NULL(control);

	claimed = panel_pos_event(panel, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	panel_destroy(panel);
}

/** panel_set_rect() sets internal field */
PCUT_TEST(set_rect)
{
	panel_t *panel;
	ui_control_t *control;
	gfx_rect_t rect;
	errno_t rc;

	rc = panel_create(NULL, &panel);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = panel_ctl(panel);
	PCUT_ASSERT_NOT_NULL(control);

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

/** panel_entry_append() appends new entry */
PCUT_TEST(entry_append)
{
	panel_t *panel;
	errno_t rc;

	rc = panel_create(NULL, &panel);
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

	rc = panel_create(NULL, &panel);
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

/** panel_first() returns valid entry or @c NULL as appropriate */
PCUT_TEST(first)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, &panel);
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

/** panel_next() returns the next entry or @c NULL as appropriate */
PCUT_TEST(next)
{
	panel_t *panel;
	panel_entry_t *entry;
	errno_t rc;

	rc = panel_create(NULL, &panel);
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

PCUT_EXPORT(panel);
