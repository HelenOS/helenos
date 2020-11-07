/*
 * Copyright (c) 2020 Jiri Svoboda
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
#include <ui/pbutton.h>
#include <ui/ui.h>
#include "../private/pbutton.h"

PCUT_INIT;

PCUT_TEST_SUITE(pbutton);

static void test_pbutton_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t test_pbutton_cb = {
	.clicked = test_pbutton_clicked
};

static ui_pbutton_cb_t dummy_pbutton_cb = {
};

typedef struct {
	bool clicked;
} test_cb_resp_t;

/** Create and destroy button */
PCUT_TEST(create_destroy)
{
	ui_pbutton_t *pbutton = NULL;
	errno_t rc;

	rc = ui_pbutton_create(NULL, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(pbutton);

	ui_pbutton_destroy(pbutton);
}

/** ui_pbutton_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_pbutton_destroy(NULL);
}

/** ui_pbutton_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_pbutton_t *pbutton;
	ui_control_t *control;
	errno_t rc;

	rc = ui_pbutton_create(NULL, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_pbutton_ctl(pbutton);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set button rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_pbutton_t *pbutton;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_pbutton_create(NULL, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_pbutton_set_rect(pbutton, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, pbutton->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, pbutton->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, pbutton->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, pbutton->rect.p1.y);

	ui_pbutton_destroy(pbutton);
}

/** Set default flag sets internal field */
PCUT_TEST(set_default)
{
	ui_pbutton_t *pbutton;
	errno_t rc;

	rc = ui_pbutton_create(NULL, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_pbutton_set_default(pbutton, true);
	PCUT_ASSERT_TRUE(pbutton->isdefault);

	ui_pbutton_set_default(pbutton, false);
	PCUT_ASSERT_FALSE(pbutton->isdefault);

	ui_pbutton_destroy(pbutton);
}

/** Paint button */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_paint(pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

/** Test ui_pbutton_clicked() */
PCUT_TEST(clicked)
{
	errno_t rc;
	ui_pbutton_t *pbutton;
	test_cb_resp_t resp;

	rc = ui_pbutton_create(NULL, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Clicked with no callbacks set */
	ui_pbutton_clicked(pbutton);

	/* Clicked with callback not implementing clicked */
	ui_pbutton_set_cb(pbutton, &dummy_pbutton_cb, NULL);
	ui_pbutton_clicked(pbutton);

	/* Clicked with real callback set */
	resp.clicked = false;
	ui_pbutton_set_cb(pbutton, &test_pbutton_cb, &resp);
	ui_pbutton_clicked(pbutton);
	PCUT_ASSERT_TRUE(resp.clicked);

	ui_pbutton_destroy(pbutton);
}

/** Press and release button */
PCUT_TEST(press_release)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.clicked = false;
	ui_pbutton_set_cb(pbutton, &test_pbutton_cb, &resp);

	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);

	ui_pbutton_press(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_release(pbutton);
	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_TRUE(resp.clicked);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

/** Press, leave and release button */
PCUT_TEST(press_leave_release)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.clicked = false;
	ui_pbutton_set_cb(pbutton, &test_pbutton_cb, &resp);

	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);

	ui_pbutton_press(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_leave(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_release(pbutton);
	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

/** Press, leave, enter and release button */
PCUT_TEST(press_leave_enter_release)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.clicked = false;
	ui_pbutton_set_cb(pbutton, &test_pbutton_cb, &resp);

	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);

	ui_pbutton_press(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_leave(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_FALSE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_enter(pbutton);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_FALSE(resp.clicked);

	ui_pbutton_release(pbutton);
	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_TRUE(pbutton->inside);
	PCUT_ASSERT_TRUE(resp.clicked);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

/** ui_pos_event() correctly translates POS_PRESS/POS_RELEASE */
PCUT_TEST(pos_event_press_release)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(pbutton->held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_pbutton_set_rect(pbutton, &rect);

	/* Press outside is not claimed and does nothing */
	event.type = POS_PRESS;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	/* Press inside is claimed and depresses button */
	event.type = POS_PRESS;
	event.hpos = 10;
	event.vpos = 20;
	claim = ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_TRUE(pbutton->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases button */
	event.type = POS_RELEASE;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_FALSE(pbutton->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

/** ui_pos_event() correctly translates POS_UPDATE to enter/leave */
PCUT_TEST(pos_event_enter_leave)
{
	errno_t rc;
	ui_t *ui;
	ui_pbutton_t *pbutton;
	pos_event_t event;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_pbutton_create(ui, "Hello", &pbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(pbutton->inside);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_pbutton_set_rect(pbutton, &rect);

	/* Moving outside does nothing */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_FALSE(pbutton->inside);

	/* Moving inside sets inside flag */
	event.type = POS_UPDATE;
	event.hpos = 10;
	event.vpos = 20;
	ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_TRUE(pbutton->inside);

	/* Moving outside clears inside flag */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_pbutton_pos_event(pbutton, &event);
	PCUT_ASSERT_FALSE(pbutton->inside);

	ui_pbutton_destroy(pbutton);
	ui_destroy(ui);
}

static void test_pbutton_clicked(ui_pbutton_t *pbutton, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->clicked = true;
}

PCUT_EXPORT(pbutton);
