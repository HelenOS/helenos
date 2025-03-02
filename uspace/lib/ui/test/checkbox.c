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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/checkbox.h>
#include <ui/resource.h>
#include "../private/checkbox.h"
#include "../private/testgc.h"

PCUT_INIT;

PCUT_TEST_SUITE(checkbox);

static void test_checkbox_switched(ui_checkbox_t *, void *, bool);

static ui_checkbox_cb_t test_checkbox_cb = {
	.switched = test_checkbox_switched
};

static ui_checkbox_cb_t dummy_checkbox_cb = {
};

typedef struct {
	bool switched;
} test_cb_resp_t;

/** Create and destroy check box */
PCUT_TEST(create_destroy)
{
	ui_checkbox_t *checkbox = NULL;
	errno_t rc;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(checkbox);

	ui_checkbox_destroy(checkbox);
}

/** ui_checkbox_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_checkbox_destroy(NULL);
}

/** ui_checkbox_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_checkbox_t *checkbox;
	ui_control_t *control;
	errno_t rc;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_checkbox_ctl(checkbox);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set check box rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_checkbox_t *checkbox;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_checkbox_set_rect(checkbox, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, checkbox->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, checkbox->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, checkbox->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, checkbox->rect.p1.y);

	ui_checkbox_destroy(checkbox);
}

/** Get check box checked returns internal field */
PCUT_TEST(get_checked)
{
	ui_checkbox_t *checkbox;
	errno_t rc;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	checkbox->checked = false;
	PCUT_ASSERT_FALSE(ui_checkbox_get_checked(checkbox));
	checkbox->checked = true;
	PCUT_ASSERT_TRUE(ui_checkbox_get_checked(checkbox));

	ui_checkbox_destroy(checkbox);
}

/** Set check box checked sets internal field */
PCUT_TEST(set_checked)
{
	ui_checkbox_t *checkbox;
	errno_t rc;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_checkbox_set_checked(checkbox, true);
	PCUT_ASSERT_TRUE(checkbox->checked);
	ui_checkbox_set_checked(checkbox, false);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_destroy(checkbox);
}

/** Paint check box in graphics mode */
PCUT_TEST(paint_gfx)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_checkbox_paint_gfx(checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint check box in text mode */
PCUT_TEST(paint_text)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_checkbox_paint_text(checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_checkbox_switched() */
PCUT_TEST(switched)
{
	errno_t rc;
	ui_checkbox_t *checkbox;
	test_cb_resp_t resp;

	rc = ui_checkbox_create(NULL, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Switched with no callbacks set */
	ui_checkbox_switched(checkbox);

	/* Switched with callback not implementing switched */
	ui_checkbox_set_cb(checkbox, &dummy_checkbox_cb, NULL);
	ui_checkbox_switched(checkbox);

	/* Switched with real callback set */
	resp.switched = false;
	ui_checkbox_set_cb(checkbox, &test_checkbox_cb, &resp);
	ui_checkbox_switched(checkbox);
	PCUT_ASSERT_TRUE(resp.switched);

	ui_checkbox_destroy(checkbox);
}

/** Press and release check box */
PCUT_TEST(press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(checkbox->checked);

	resp.switched = false;
	ui_checkbox_set_cb(checkbox, &test_checkbox_cb, &resp);

	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);

	ui_checkbox_press(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_release(checkbox);
	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_TRUE(resp.switched);
	PCUT_ASSERT_TRUE(checkbox->checked);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, leave and release check box */
PCUT_TEST(press_leave_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.switched = false;
	ui_checkbox_set_cb(checkbox, &test_checkbox_cb, &resp);

	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);

	ui_checkbox_press(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_leave(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_release(checkbox);
	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, leave, enter and release check box */
PCUT_TEST(press_leave_enter_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(checkbox->checked);

	resp.switched = false;
	ui_checkbox_set_cb(checkbox, &test_checkbox_cb, &resp);

	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);

	ui_checkbox_press(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_leave(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_FALSE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_enter(checkbox);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_FALSE(resp.switched);
	PCUT_ASSERT_FALSE(checkbox->checked);

	ui_checkbox_release(checkbox);
	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_TRUE(checkbox->inside);
	PCUT_ASSERT_TRUE(resp.switched);
	PCUT_ASSERT_TRUE(checkbox->checked);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_pos_event() correctly translates POS_PRESS/POS_RELEASE */
PCUT_TEST(pos_event_press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(checkbox->held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_checkbox_set_rect(checkbox, &rect);

	/* Press outside is not claimed and does nothing */
	event.type = POS_PRESS;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	/* Press inside is claimed and depresses check box */
	event.type = POS_PRESS;
	event.hpos = 10;
	event.vpos = 20;
	claim = ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_TRUE(checkbox->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases check box */
	event.type = POS_RELEASE;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_FALSE(checkbox->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_pos_event() correctly translates POS_UPDATE to enter/leave */
PCUT_TEST(pos_event_enter_leave)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_checkbox_t *checkbox;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_checkbox_create(resource, "Hello", &checkbox);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(checkbox->inside);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_checkbox_set_rect(checkbox, &rect);

	/* Moving outside does nothing */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_FALSE(checkbox->inside);

	/* Moving inside sets inside flag */
	event.type = POS_UPDATE;
	event.hpos = 10;
	event.vpos = 20;
	ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_TRUE(checkbox->inside);

	/* Moving outside clears inside flag */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_checkbox_pos_event(checkbox, &event);
	PCUT_ASSERT_FALSE(checkbox->inside);

	ui_checkbox_destroy(checkbox);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static void test_checkbox_switched(ui_checkbox_t *checkbox, void *arg,
    bool checked)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->switched = true;
}

PCUT_EXPORT(checkbox);
