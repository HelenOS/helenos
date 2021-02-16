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
#include <gfx/render.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(window);

static void test_window_close(ui_window_t *, void *);
static void test_window_focus(ui_window_t *, void *);
static void test_window_kbd(ui_window_t *, void *, kbd_event_t *);
static errno_t test_window_paint(ui_window_t *, void *);
static void test_window_pos(ui_window_t *, void *, pos_event_t *);
static void test_window_unfocus(ui_window_t *, void *);

static ui_window_cb_t test_window_cb = {
	.close = test_window_close,
	.focus = test_window_focus,
	.kbd = test_window_kbd,
	.paint = test_window_paint,
	.pos = test_window_pos,
	.unfocus = test_window_unfocus
};

static ui_window_cb_t dummy_window_cb = {
};

static errno_t test_ctl_paint(void *);
static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);

static ui_control_ops_t test_ctl_ops = {
	.paint = test_ctl_paint,
	.pos_event = test_ctl_pos_event
};

typedef struct {
	errno_t rc;
	bool close;
	bool focus;
	bool kbd;
	kbd_event_t kbd_event;
	bool paint;
	bool pos;
	pos_event_t pos_event;
	bool unfocus;
} test_cb_resp_t;

typedef struct {
	errno_t rc;
	ui_evclaim_t claim;
	bool paint;
	bool pos;
	pos_event_t pos_event;
} test_ctl_resp_t;

/** Create and destroy window */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_window_destroy(NULL);
}

/** ui_window_add()/ui_window_remove() ... */
PCUT_TEST(add_remove)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_control_t *control = NULL;
	test_ctl_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Control not called since it hasn't been added yet */
	resp.rc = ENOMEM;
	resp.paint = false;
	rc = ui_window_def_paint(window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(resp.paint);

	ui_window_add(window, control);

	/* Now paint request should be delivered to control */
	resp.rc = EOK;
	resp.paint = false;
	rc = ui_window_def_paint(window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_window_remove(window, control);

	/*
	 * After having removed the control the request should no longer
	 * be delivered to it.
	 */
	resp.rc = ENOMEM;
	resp.paint = false;
	rc = ui_window_def_paint(window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(resp.paint);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_resize */
PCUT_TEST(resize)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	gfx_rect_t nrect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 1;
	params.rect.p1.y = 1;

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	nrect.p0.x = -1;
	nrect.p0.y = -1;
	nrect.p1.x = 2;
	nrect.p1.y = 2;
	rc = ui_window_resize(window, &nrect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_get_res/gc/rect() return valid objects */
PCUT_TEST(get_res_gc_rect)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_resource_t *res;
	gfx_context_t *gc;
	gfx_rect_t rect;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	res = ui_window_get_res(window);
	PCUT_ASSERT_NOT_NULL(res);

	gc = ui_window_get_gc(window);
	PCUT_ASSERT_NOT_NULL(gc);

	ui_window_get_app_rect(window, &rect);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_get_app_gc() return valid GC */
PCUT_TEST(get_app_gc)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	gfx_context_t *gc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_window_get_app_gc(window, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gc);

	rc = gfx_fill_rect(gc, &params.rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_window_paint() */
PCUT_TEST(paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_window_paint(window);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test ui_window_def_paint() */
PCUT_TEST(def_paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_control_t *control = NULL;
	test_ctl_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_add(window, control);

	resp.rc = EOK;
	resp.paint = false;
	rc = ui_window_def_paint(window);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	resp.rc = ENOMEM;
	resp.paint = false;
	rc = ui_window_def_paint(window);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	PCUT_ASSERT_TRUE(resp.paint);

	/* Need to remove first because we didn't implement the destructor */
	ui_window_remove(window, control);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_def_pos() delivers position event to control in window */
PCUT_TEST(def_pos)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_control_t *control = NULL;
	test_ctl_resp_t resp;
	pos_event_t event;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_window_add(window, control);

	event.pos_id = 1;
	event.type = POS_PRESS;
	event.btn_num = 2;
	event.hpos = 3;
	event.vpos = 4;

	resp.pos = false;
	resp.claim = ui_claimed;

	ui_window_def_pos(window, &event);

	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(event.pos_id, resp.pos_event.pos_id);
	PCUT_ASSERT_INT_EQUALS(event.type, resp.pos_event.type);
	PCUT_ASSERT_INT_EQUALS(event.btn_num, resp.pos_event.btn_num);
	PCUT_ASSERT_INT_EQUALS(event.hpos, resp.pos_event.hpos);
	PCUT_ASSERT_INT_EQUALS(event.vpos, resp.pos_event.vpos);

	/* Need to remove first because we didn't implement the destructor */
	ui_window_remove(window, control);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_close() calls close callback set via ui_window_set_cb() */
PCUT_TEST(send_close)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	/* Close callback with no callbacks set */
	ui_window_send_close(window);

	/* Close callback with close callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_close(window);

	/* Close callback with real callback set */
	resp.close = false;
	ui_window_set_cb(window, &test_window_cb, &resp);
	ui_window_send_close(window);
	PCUT_ASSERT_TRUE(resp.close);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_focus() calls focus callback set via ui_window_set_cb() */
PCUT_TEST(send_focus)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	/* Focus callback with no callbacks set */
	ui_window_send_focus(window);

	/* Focus callback with focus callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_focus(window);

	/* Focus callback with real callback set */
	resp.focus = false;
	ui_window_set_cb(window, &test_window_cb, &resp);
	ui_window_send_focus(window);
	PCUT_ASSERT_TRUE(resp.focus);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_kbd() calls kbd callback set via ui_window_set_cb() */
PCUT_TEST(send_kbd)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	kbd_event_t kbd_event;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	kbd_event.type = POS_PRESS;
	kbd_event.key = KC_X;
	kbd_event.mods = 0;
	kbd_event.c = 'x';

	/* Kbd callback with no callbacks set */
	ui_window_send_kbd(window, &kbd_event);

	/* Kbd callback with kbd callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_kbd(window, &kbd_event);

	/* Kbd callback with real callback set */
	resp.kbd = false;
	ui_window_set_cb(window, &test_window_cb, &resp);
	ui_window_send_kbd(window, &kbd_event);
	PCUT_ASSERT_TRUE(resp.kbd);
	PCUT_ASSERT_EQUALS(kbd_event.type, resp.kbd_event.type);
	PCUT_ASSERT_INT_EQUALS(kbd_event.key, resp.kbd_event.key);
	PCUT_ASSERT_INT_EQUALS(kbd_event.mods, resp.kbd_event.mods);
	PCUT_ASSERT_INT_EQUALS(kbd_event.c, resp.kbd_event.c);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_paint() calls paint callback set via ui_window_set_cb() */
PCUT_TEST(send_paint)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	/* Paint callback with no callbacks set */
	ui_window_send_paint(window);

	/* Paint callback with paint callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_paint(window);

	/* Paint callback with real callback set */
	resp.paint = false;
	resp.rc = EOK;
	ui_window_set_cb(window, &test_window_cb, &resp);
	rc = ui_window_send_paint(window);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_pos() calls pos callback set via ui_window_set_cb() */
PCUT_TEST(send_pos)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	pos_event_t pos_event;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	pos_event.pos_id = 1;
	pos_event.type = POS_PRESS;
	pos_event.btn_num = 2;
	pos_event.hpos = 3;
	pos_event.vpos = 4;

	/* Pos callback with no callbacks set */
	ui_window_send_pos(window, &pos_event);

	/* Pos callback with pos callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_pos(window, &pos_event);

	/* Pos callback with real callback set */
	resp.pos = false;
	ui_window_set_cb(window, &test_window_cb, &resp);
	ui_window_send_pos(window, &pos_event);
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(pos_event.pos_id, resp.pos_event.pos_id);
	PCUT_ASSERT_EQUALS(pos_event.type, resp.pos_event.type);
	PCUT_ASSERT_INT_EQUALS(pos_event.btn_num, resp.pos_event.btn_num);
	PCUT_ASSERT_INT_EQUALS(pos_event.hpos, resp.pos_event.hpos);
	PCUT_ASSERT_INT_EQUALS(pos_event.vpos, resp.pos_event.vpos);

	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_window_send_unfocus() calls unfocus callback set via ui_window_set_cb() */
PCUT_TEST(send_unfocus)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&params);
	params.caption = "Hello";

	rc = ui_window_create(ui, &params, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	/* Unfocus callback with no callbacks set */
	ui_window_send_unfocus(window);

	/* Unfocus callback with unfocus callback not implemented */
	ui_window_set_cb(window, &dummy_window_cb, NULL);
	ui_window_send_unfocus(window);

	/* Unfocus callback with real callback set */
	resp.close = false;
	ui_window_set_cb(window, &test_window_cb, &resp);
	ui_window_send_unfocus(window);
	PCUT_ASSERT_TRUE(resp.unfocus);

	ui_window_destroy(window);
	ui_destroy(ui);
}

static void test_window_close(ui_window_t *window, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

static void test_window_focus(ui_window_t *window, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->focus = true;
}

static void test_window_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->kbd = true;
	resp->kbd_event = *event;
}

static errno_t test_window_paint(ui_window_t *window, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->paint = true;
	return resp->rc;
}

static void test_window_pos(ui_window_t *window, void *arg,
    pos_event_t *event)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->pos = true;
	resp->pos_event = *event;
}

static void test_window_unfocus(ui_window_t *window, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->unfocus = true;
}

static errno_t test_ctl_paint(void *arg)
{
	test_ctl_resp_t *resp = (test_ctl_resp_t *) arg;

	resp->paint = true;
	return resp->rc;
}

static ui_evclaim_t test_ctl_pos_event(void *arg, pos_event_t *event)
{
	test_ctl_resp_t *resp = (test_ctl_resp_t *) arg;

	resp->pos = true;
	resp->pos_event = *event;

	return resp->claim;
}

PCUT_EXPORT(window);
