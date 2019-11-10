/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <async.h>
#include <errno.h>
#include <display.h>
#include <disp_srv.h>
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <ipcgfx/server.h>
#include <loc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(display);

static const char *test_display_server = "test-display";
static const char *test_display_svc = "test/display";

static void test_display_conn(ipc_call_t *, void *);
static void test_kbd_event(void *, kbd_event_t *);

static errno_t test_window_create(void *, sysarg_t *);
static errno_t test_window_destroy(void *, sysarg_t);
static errno_t test_get_event(void *, sysarg_t *, display_wnd_ev_t *);

static errno_t test_gc_set_color(void *, gfx_color_t *);

static display_ops_t test_display_srv_ops = {
	.window_create = test_window_create,
	.window_destroy = test_window_destroy,
	.get_event = test_get_event
};

static display_wnd_cb_t test_display_wnd_cb = {
	.kbd_event = test_kbd_event
};

static gfx_context_ops_t test_gc_ops = {
	.set_color = test_gc_set_color
};

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;
	sysarg_t wnd_id;
	display_wnd_ev_t event;
	display_wnd_ev_t revent;
	int event_cnt;
	bool window_create_called;
	bool window_destroy_called;
	bool get_event_called;
	bool set_color_called;
	bool kbd_event_called;
	fibril_condvar_t kbd_event_cv;
	fibril_mutex_t kbd_event_lock;
	display_srv_t *srv;
} test_response_t;

/** display_open(), display_close() work for valid display service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	test_response_t resp;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** display_window_create() with server returning error response works */
PCUT_TEST(window_create_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = ENOMEM;
	resp.window_create_called = false;
	rc = display_window_create(disp, &test_display_wnd_cb, NULL, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_NULL(wnd);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** display_window_create() and display_window_destroy() with success
 *
 * with server returning success,
 */
PCUT_TEST(window_create_destroy_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	resp.window_create_called = false;
	rc = display_window_create(disp, &test_display_wnd_cb, NULL, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.window_destroy_called = false;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_TRUE(resp.window_destroy_called);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** display_window_create() with server returning error response works. */
PCUT_TEST(window_destroy_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	resp.window_create_called = false;
	rc = display_window_create(disp, &test_display_wnd_cb, NULL, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_destroy_called = false;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_TRUE(resp.window_destroy_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** display_window_get_gc with server returning failure */
PCUT_TEST(window_get_gc_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;
	gfx_context_t *gc;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	rc = display_window_create(disp, &test_display_wnd_cb, NULL, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	gc = NULL;
	resp.rc = ENOMEM;
	rc = display_window_get_gc(wnd, &gc);
	/* async_connect_me_to() does not return specific error */
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);
	PCUT_ASSERT_NULL(gc);

	resp.rc = EOK;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** display_window_get_gc with server returning success */
PCUT_TEST(window_get_gc_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_color_t *color;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	rc = display_window_create(disp, &test_display_wnd_cb, NULL, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	gc = NULL;
	rc = display_window_get_gc(wnd, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gc);

	rc = gfx_color_new_rgb_i16(0, 0, 0, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.set_color_called = false;
	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.set_color_called);

	gfx_color_delete(color);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Keyboard event can be delivered from server to client callback function */
PCUT_TEST(kbd_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_window_t *wnd;
	test_response_t resp;
	gfx_context_t *gc;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	rc = display_window_create(disp, &test_display_wnd_cb, (void *) &resp,
	    &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	gc = NULL;
	rc = display_window_get_gc(wnd, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.event_cnt = 1;
	resp.event.kbd_event.type = KEY_PRESS;
	resp.event.kbd_event.key = KC_ENTER;
	resp.event.kbd_event.mods = 0;
	resp.event.kbd_event.c = L'\0';
	resp.wnd_id = wnd->id;
	resp.kbd_event_called = false;
	fibril_mutex_initialize(&resp.kbd_event_lock);
	fibril_condvar_initialize(&resp.kbd_event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.kbd_event_lock);
	while (!resp.kbd_event_called) {
		fibril_condvar_wait(&resp.kbd_event_cv, &resp.kbd_event_lock);
	}
	fibril_mutex_unlock(&resp.kbd_event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_EQUALS(resp.event.kbd_event.type,
	    resp.revent.kbd_event.type);
	PCUT_ASSERT_EQUALS(resp.event.kbd_event.key,
	    resp.revent.kbd_event.key);
	PCUT_ASSERT_EQUALS(resp.event.kbd_event.mods,
	    resp.revent.kbd_event.mods);
	PCUT_ASSERT_EQUALS(resp.event.kbd_event.c,
	    resp.revent.kbd_event.c);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test display service connection.
 *
 * This is very similar to connection handler in the display server.
 * XXX This should be folded into display_srv, if possible
 */
static void test_display_conn(ipc_call_t *icall, void *arg)
{
	test_response_t *resp = (test_response_t *) arg;
	display_srv_t srv;
	sysarg_t wnd_id;
	sysarg_t svc_id;
	gfx_context_t *gc;
	errno_t rc;

	svc_id = ipc_get_arg2(icall);
	wnd_id = ipc_get_arg3(icall);

	if (svc_id != 0) {
		/* Set up protocol structure */
		display_srv_initialize(&srv);
		srv.ops = &test_display_srv_ops;
		srv.arg = arg;
		resp->srv = &srv;

		/* Handle connection */
		display_conn(icall, &srv);

		resp->srv = NULL;
	} else {
		(void) wnd_id;

		if (resp->rc != EOK) {
			async_answer_0(icall, resp->rc);
			return;
		}

		rc = gfx_context_new(&test_gc_ops, arg, &gc);
		if (rc != EOK) {
			async_answer_0(icall, ENOMEM);
			return;
		}

		/* Window GC connection */
		gc_conn(icall, gc);
	}
}

static void test_kbd_event(void *arg, kbd_event_t *event)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.kbd_event = *event;

	fibril_mutex_lock(&resp->kbd_event_lock);
	resp->kbd_event_called = true;
	fibril_condvar_broadcast(&resp->kbd_event_cv);
	fibril_mutex_unlock(&resp->kbd_event_lock);
}

static errno_t test_window_create(void *arg, sysarg_t *rwnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_create_called = true;
	if (resp->rc == EOK)
		*rwnd_id = resp->wnd_id;

	return resp->rc;
}

static errno_t test_window_destroy(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_destroy_called = true;
	return resp->rc;
}

static errno_t test_get_event(void *arg, sysarg_t *wnd_id, display_wnd_ev_t *event)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_event_called = true;
	if (resp->event_cnt > 0) {
		--resp->event_cnt;
		*wnd_id = resp->wnd_id;
		*event = resp->event;
		return EOK;
	}

	return ENOENT;
}

static errno_t test_gc_set_color(void *arg, gfx_color_t *color)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->set_color_called = true;
	return resp->rc;
}

PCUT_EXPORT(display);
