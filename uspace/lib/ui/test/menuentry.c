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
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include "../private/dummygc.h"
#include "../private/menuentry.h"

PCUT_INIT;

PCUT_TEST_SUITE(menuentry);

typedef struct {
	bool activated;
} test_resp_t;

static void test_entry_cb(ui_menu_entry_t *, void *);

/** Create and destroy menu bar */
PCUT_TEST(create_destroy)
{
	ui_menu_bar_t *mbar = NULL;
	errno_t rc;

	rc = ui_menu_bar_create(NULL, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	ui_menu_bar_destroy(mbar);
}

/** ui_menu_bar_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_bar_destroy(NULL);
}

/** ui_menu_entry_set_cb() .. */
PCUT_TEST(set_cb)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	test_resp_t resp;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);

	resp.activated = false;
	ui_menu_entry_cb(mentry);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_entry_first() / ui_menu_entry_next() iterate over entries */
PCUT_TEST(first_next)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *entry1 = NULL;
	ui_menu_entry_t *entry2 = NULL;
	ui_menu_entry_t *e;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", &entry1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry1);

	rc = ui_menu_entry_create(menu, "Bar", &entry2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry2);

	e = ui_menu_entry_first(menu);
	PCUT_ASSERT_EQUALS(entry1, e);

	e = ui_menu_entry_next(e);
	PCUT_ASSERT_EQUALS(entry2, e);

	e = ui_menu_entry_next(e);
	PCUT_ASSERT_NULL(e);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_entry_width() / ui_menu_entry_height() */
PCUT_TEST(width_height)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord_t width;
	gfx_coord_t height;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	width = ui_menu_entry_width(mentry);
	PCUT_ASSERT_INT_EQUALS(11 + 8, width);

	height = ui_menu_entry_height(mentry);
	PCUT_ASSERT_INT_EQUALS(13 + 8, height);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Paint menu entry */
PCUT_TEST(paint)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "Foo", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_entry_paint(mentry, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Press and release activates menu entry */
PCUT_TEST(press_release)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	test_resp_t resp;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Press, leave and release does not activate entry */
PCUT_TEST(press_leave_release)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	test_resp_t resp;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_leave(mentry, &pos);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Press, leave, enter and release activates menu entry */
PCUT_TEST(press_leave_enter_release)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	test_resp_t resp;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	ui_menu_entry_set_cb(mentry, test_entry_cb, &resp);
	resp.activated = false;

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_leave(mentry, &pos);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_enter(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);
	PCUT_ASSERT_FALSE(resp.activated);

	ui_menu_entry_release(mentry);
	PCUT_ASSERT_FALSE(mentry->held);
	PCUT_ASSERT_TRUE(resp.activated);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Press event inside menu entry */
PCUT_TEST(pos_press_inside)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	pos_event_t event;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	pos.x = 0;
	pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 4;
	event.vpos = 4;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Press event outside menu entry */
PCUT_TEST(pos_press_outside)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	pos_event_t event;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	pos.x = 0;
	pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 20;
	event.vpos = 20;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_FALSE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Position event moving out of menu entry */
PCUT_TEST(pos_move_out)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	pos_event_t event;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	pos.x = 0;
	pos.y = 0;
	ui_menu_entry_press(mentry, &pos);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	event.type = POS_UPDATE;
	event.hpos = 20;
	event.vpos = 20;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_FALSE(mentry->inside);
	PCUT_ASSERT_TRUE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Position event moving inside menu entry */
PCUT_TEST(pos_move_in)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_entry_t *mentry = NULL;
	gfx_coord2_t pos;
	pos_event_t event;
	errno_t rc;

	rc = dummygc_create(&dgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = dummygc_get_ctx(dgc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_menu_bar_create(resource, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mbar);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	rc = ui_menu_entry_create(menu, "X", &mentry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(mentry);

	event.type = POS_UPDATE;
	event.hpos = 4;
	event.vpos = 4;

	pos.x = 0;
	pos.y = 0;

	ui_menu_entry_pos_event(mentry, &pos, &event);
	PCUT_ASSERT_TRUE(mentry->inside);
	PCUT_ASSERT_FALSE(mentry->held);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

static void test_entry_cb(ui_menu_entry_t *mentry, void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->activated = true;
}

PCUT_EXPORT(menuentry);
