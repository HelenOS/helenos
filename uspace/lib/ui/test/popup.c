/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <ui/popup.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/popup.h"
#include "../private/window.h"

PCUT_INIT;

PCUT_TEST_SUITE(popup);

static void test_popup_close(ui_popup_t *, void *);
static void test_popup_kbd(ui_popup_t *, void *, kbd_event_t *);
static errno_t test_popup_paint(ui_popup_t *, void *);
static void test_popup_pos(ui_popup_t *, void *, pos_event_t *);

static ui_popup_cb_t test_popup_cb = {
	.close = test_popup_close,
	.kbd = test_popup_kbd,
	.paint = test_popup_paint,
	.pos = test_popup_pos,
};

static ui_popup_cb_t dummy_popup_cb = {
};

static errno_t test_ctl_paint(void *);
static ui_evclaim_t test_ctl_pos_event(void *, pos_event_t *);
static void test_ctl_unfocus(void *, unsigned);

static ui_control_ops_t test_ctl_ops = {
	.paint = test_ctl_paint,
	.pos_event = test_ctl_pos_event,
	.unfocus = test_ctl_unfocus
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
	bool unfocus;
	unsigned unfocus_nfocus;
} test_ctl_resp_t;

/** Create and destroy popup window */
PCUT_TEST(create_destroy)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t wparams;
	ui_window_t *window = NULL;
	ui_popup_params_t params;
	ui_popup_t *popup = NULL;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&wparams);
	wparams.caption = "Hello";

	rc = ui_window_create(ui, &wparams, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_popup_params_init(&params);

	rc = ui_popup_create(ui, window, &params, &popup);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(popup);

	ui_popup_destroy(popup);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_popup_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_popup_destroy(NULL);
}

/** ui_popup_add()/ui_popup_remove() ... */
PCUT_TEST(add_remove)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t wparams;
	ui_window_t *window = NULL;
	ui_popup_params_t params;
	ui_popup_t *popup = NULL;
	ui_control_t *control = NULL;
	test_ctl_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&wparams);
	wparams.caption = "Hello";

	rc = ui_window_create(ui, &wparams, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_popup_params_init(&params);

	rc = ui_popup_create(ui, window, &params, &popup);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(popup);

	rc = ui_control_new(&test_ctl_ops, &resp, &control);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Control not called since it hasn't been added yet */
	resp.rc = ENOMEM;
	resp.paint = false;
	rc = ui_window_def_paint(popup->window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(resp.paint);

	ui_popup_add(popup, control);

	/* Now paint request should be delivered to control */
	resp.rc = EOK;
	resp.paint = false;
	rc = ui_window_def_paint(popup->window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.paint);

	ui_popup_remove(popup, control);

	/*
	 * After having removed the control the request should no longer
	 * be delivered to it.
	 */
	resp.rc = ENOMEM;
	resp.paint = false;
	rc = ui_window_def_paint(popup->window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_FALSE(resp.paint);

	ui_popup_destroy(popup);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** ui_popup_get_res/gc() return valid objects */
PCUT_TEST(get_res_gc)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t wparams;
	ui_window_t *window = NULL;
	ui_popup_params_t params;
	ui_popup_t *popup = NULL;
	ui_resource_t *res;
	gfx_context_t *gc;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&wparams);
	wparams.caption = "Hello";

	rc = ui_window_create(ui, &wparams, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_popup_params_init(&params);

	rc = ui_popup_create(ui, window, &params, &popup);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(popup);

	res = ui_popup_get_res(popup);
	PCUT_ASSERT_NOT_NULL(res);

	gc = ui_popup_get_gc(popup);
	PCUT_ASSERT_NOT_NULL(gc);

	ui_popup_destroy(popup);
	ui_window_destroy(window);
	ui_destroy(ui);
}

/** Test position event callback set via ui_popup_set_cb() */
PCUT_TEST(send_pos)
{
	errno_t rc;
	ui_t *ui = NULL;
	ui_wnd_params_t wparams;
	ui_window_t *window = NULL;
	ui_popup_params_t params;
	ui_popup_t *popup = NULL;
	pos_event_t pos_event;
	test_cb_resp_t resp;

	rc = ui_create_disp(NULL, &ui);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wnd_params_init(&wparams);
	wparams.caption = "Hello";

	rc = ui_window_create(ui, &wparams, &window);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(window);

	ui_popup_params_init(&params);

	rc = ui_popup_create(ui, window, &params, &popup);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(popup);

	pos_event.pos_id = 1;
	pos_event.type = POS_PRESS;
	pos_event.btn_num = 2;
	pos_event.hpos = 3;
	pos_event.vpos = 4;

	/* Pos callback with no callbacks set */
	ui_window_send_pos(popup->window, &pos_event);

	/* Pos callback with pos callback not implemented */
	ui_popup_set_cb(popup, &dummy_popup_cb, NULL);
	ui_window_send_pos(popup->window, &pos_event);

	/* Pos callback with real callback set */
	resp.pos = false;
	ui_popup_set_cb(popup, &test_popup_cb, &resp);
	ui_window_send_pos(popup->window, &pos_event);
	PCUT_ASSERT_TRUE(resp.pos);
	PCUT_ASSERT_INT_EQUALS(pos_event.pos_id, resp.pos_event.pos_id);
	PCUT_ASSERT_EQUALS(pos_event.type, resp.pos_event.type);
	PCUT_ASSERT_INT_EQUALS(pos_event.btn_num, resp.pos_event.btn_num);
	PCUT_ASSERT_INT_EQUALS(pos_event.hpos, resp.pos_event.hpos);
	PCUT_ASSERT_INT_EQUALS(pos_event.vpos, resp.pos_event.vpos);

	ui_popup_destroy(popup);
	ui_window_destroy(window);
	ui_destroy(ui);
}

static void test_popup_close(ui_popup_t *popup, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->close = true;
}

static void test_popup_kbd(ui_popup_t *popup, void *arg,
    kbd_event_t *event)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->kbd = true;
	resp->kbd_event = *event;
}

static errno_t test_popup_paint(ui_popup_t *popup, void *arg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->paint = true;
	return resp->rc;
}

static void test_popup_pos(ui_popup_t *popup, void *arg,
    pos_event_t *event)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->pos = true;
	resp->pos_event = *event;
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

static void test_ctl_unfocus(void *arg, unsigned nfocus)
{
	test_ctl_resp_t *resp = (test_ctl_resp_t *) arg;

	resp->unfocus = true;
	resp->unfocus_nfocus = nfocus;
}

PCUT_EXPORT(popup);
