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
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_destroy(flist);
}

/** ui_file_list_set_cb() sets callback */
PCUT_TEST(set_cb)
{
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);
	PCUT_ASSERT_EQUALS(&test_cb, flist->cb);
	PCUT_ASSERT_EQUALS(&resp, flist->cb_arg);

	ui_file_list_destroy(flist);
}

/** ui_file_list_entry_height() gives the correct height */
PCUT_TEST(entry_height)
{
	// XXX
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
	ui_file_list_t *flist;
	ui_control_t *control;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_file_list_ctl(flist);
	PCUT_ASSERT_NOT_NULL(control);

	ui_file_list_destroy(flist);
}

/** Test ui_file_list_kbd_event() */
PCUT_TEST(kbd_event)
{
	ui_file_list_t *flist;
	ui_evclaim_t claimed;
	kbd_event_t event;
	errno_t rc;

	/* Active file list should claim events */

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_file_list_kbd_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_file_list_destroy(flist);

	/* Inactive file list should not claim events */

	rc = ui_file_list_create(NULL, false, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	event.type = KEY_PRESS;
	event.key = KC_ESCAPE;
	event.mods = 0;
	event.c = '\0';

	claimed = ui_file_list_kbd_event(flist, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	ui_file_list_destroy(flist);
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
	rect.p1.x = 30;
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
	ui_file_list_t *flist;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
	rect.p1.x = 30;
	rect.p1.y = 220;

	ui_file_list_set_rect(flist, &rect);

	/* NOTE If page size changes, we have problems elsewhere in the tests */
	PCUT_ASSERT_INT_EQUALS(11, ui_file_list_page_size(flist));

	ui_file_list_destroy(flist);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_file_list_inside_rect() ... */
PCUT_TEST(inside_rect)
{
	// XXX
}

/** ui_file_list_is_active() returns file list activity state */
PCUT_TEST(is_active)
{
	ui_file_list_t *flist;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(ui_file_list_is_active(flist));
	ui_file_list_destroy(flist);

	rc = ui_file_list_create(NULL, false, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(ui_file_list_is_active(flist));
	ui_file_list_destroy(flist);
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
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_entry_delete() deletes entry */
PCUT_TEST(entry_delete)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_clear_entries() removes all entries from file list */
PCUT_TEST(clear_entries)
{
	ui_file_list_t *flist;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_read_dir() reads the contents of a directory */
PCUT_TEST(read_dir)
{
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

	rc = ui_file_list_create(NULL, true, &flist);
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
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_entry_ptr_cmp compares two indirectly referenced entries */
PCUT_TEST(entry_ptr_cmp)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *a, *b;
	ui_file_list_entry_attr_t attr;
	int rel;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_first() returns valid entry or @c NULL as appropriate */
PCUT_TEST(first)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_last() returns valid entry or @c NULL as appropriate */
PCUT_TEST(last)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_next() returns the next entry or @c NULL as appropriate */
PCUT_TEST(next)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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
}

/** ui_file_list_prev() returns the previous entry or @c NULL as appropriate */
PCUT_TEST(prev)
{
	ui_file_list_t *flist;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	errno_t rc;

	rc = ui_file_list_create(NULL, true, &flist);
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

/** ui_file_list_cursor_move() ... */
PCUT_TEST(cursor_move)
{
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

/** ui_file_list_open() opens a directory entry */
PCUT_TEST(open)
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

	rc = ui_file_list_open(flist, entry);
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

PCUT_TEST(open_dir)
{
}

PCUT_TEST(open_file)
{
}

/** ui_file_list_activate_req() sends activation request */
PCUT_TEST(activate_req)
{
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_file_list_create(NULL, true, &flist);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_file_list_set_cb(flist, &test_cb, &resp);

	resp.activate_req = false;
	resp.activate_req_file_list = NULL;

	ui_file_list_activate_req(flist);
	PCUT_ASSERT_TRUE(resp.activate_req);
	PCUT_ASSERT_EQUALS(flist, resp.activate_req_file_list);

	ui_file_list_destroy(flist);
}

/** ui_file_list_selected() runs selected callback */
PCUT_TEST(selected)
{
	ui_file_list_t *flist;
	errno_t rc;
	test_resp_t resp;

	rc = ui_file_list_create(NULL, true, &flist);
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
