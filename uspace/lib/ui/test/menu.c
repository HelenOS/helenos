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
#include <str.h>
#include <ui/control.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include "../private/dummygc.h"
#include "../private/menu.h"
#include "../private/menubar.h"

PCUT_INIT;

PCUT_TEST_SUITE(menu);

typedef struct {
	bool expose;
} test_resp_t;

static void test_expose(void *);

/** Create and destroy menu */
PCUT_TEST(create_destroy)
{
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	errno_t rc;

	rc = ui_menu_bar_create(NULL, &mbar);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_menu_create(mbar, "Test", &menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu);

	/*
	 * Normally we don't need to destroy a menu explicitly, it will
	 * be destroyed along with menu bar, but here we'll test destroying
	 * it explicitly.
	 */
	ui_menu_destroy(menu);
	ui_menu_bar_destroy(mbar);
}

/** ui_menu_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_menu_destroy(NULL);
}

/** ui_menu_first() / ui_menu_next() iterate over menus */
PCUT_TEST(first_next)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu1 = NULL;
	ui_menu_t *menu2 = NULL;
	ui_menu_t *m;
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

	rc = ui_menu_create(mbar, "Test 1", &menu1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu1);

	rc = ui_menu_create(mbar, "Test 1", &menu2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(menu2);

	m = ui_menu_first(mbar);
	PCUT_ASSERT_EQUALS(menu1, m);

	m = ui_menu_next(m);
	PCUT_ASSERT_EQUALS(menu2, m);

	m = ui_menu_next(m);
	PCUT_ASSERT_NULL(m);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_caption() returns the menu's caption */
PCUT_TEST(caption)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	const char *caption;
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

	caption = ui_menu_caption(menu);
	PCUT_ASSERT_NOT_NULL(caption);

	PCUT_ASSERT_INT_EQUALS(0, str_cmp(caption, "Test"));

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_get_rect() returns outer menu rectangle */
PCUT_TEST(get_rect)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	const char *caption;
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

	caption = ui_menu_caption(menu);
	PCUT_ASSERT_NOT_NULL(caption);

	pos.x = 0;
	pos.y = 0;
	ui_menu_get_rect(menu, &pos, &rect);

	PCUT_ASSERT_INT_EQUALS(0, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(8, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, rect.p1.y);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Paint menu */
PCUT_TEST(paint)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
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

	pos.x = 0;
	pos.y = 0;
	rc = ui_menu_paint(menu, &pos);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_unpaint() calls expose callback */
PCUT_TEST(unpaint)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
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

	ui_resource_set_expose_cb(resource, test_expose, &resp);

	resp.expose = false;
	rc = ui_menu_unpaint(menu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.expose);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_pos_event() inside menu is claimed */
PCUT_TEST(pos_event_inside)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_evclaim_t claimed;
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

	pos.x = 0;
	pos.y = 0;
	event.type = POS_PRESS;
	event.hpos = 0;
	event.vpos = 0;
	claimed = ui_menu_pos_event(menu, &pos, &event);
	PCUT_ASSERT_EQUALS(ui_claimed, claimed);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** ui_menu_pos_event() outside menu closes it */
PCUT_TEST(pos_event_outside)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_evclaim_t claimed;
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

	pos.x = 0;
	pos.y = 0;
	ui_menu_bar_select(mbar, &pos, menu);
	PCUT_ASSERT_EQUALS(menu, mbar->selected);

	pos.x = 10;
	pos.y = 0;
	event.type = POS_PRESS;
	event.hpos = 0;
	event.vpos = 0;
	claimed = ui_menu_pos_event(menu, &pos, &event);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claimed);

	/* Press event outside menu should close it */
	PCUT_ASSERT_NULL(mbar->selected);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

/** Computing menu geometry */
PCUT_TEST(get_geom)
{
	dummy_gc_t *dgc;
	gfx_context_t *gc;
	ui_resource_t *resource = NULL;
	ui_menu_bar_t *mbar = NULL;
	ui_menu_t *menu = NULL;
	ui_menu_geom_t geom;
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

	pos.x = 0;
	pos.y = 0;
	ui_menu_get_geom(menu, &pos, &geom);

	PCUT_ASSERT_INT_EQUALS(0, geom.outer_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, geom.outer_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(8, geom.outer_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, geom.outer_rect.p1.y);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, geom.entries_rect.p1.y);

	ui_menu_bar_destroy(mbar);
	ui_resource_destroy(resource);
	dummygc_destroy(dgc);
}

static void test_expose(void *arg)
{
	test_resp_t *resp = (test_resp_t *) arg;

	resp->expose = true;
}

PCUT_EXPORT(menu);
