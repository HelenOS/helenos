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
#include <str.h>
#include "../private/display.h"

PCUT_INIT;

PCUT_TEST_SUITE(display);

static const char *test_display_server = "test-display";
static const char *test_display_svc = "test/display";

static void test_display_conn(ipc_call_t *, void *);

static void test_close_event(void *);
static void test_focus_event(void *, unsigned);
static void test_kbd_event(void *, kbd_event_t *);
static void test_pos_event(void *, pos_event_t *);
static void test_unfocus_event(void *, unsigned);

static errno_t test_window_create(void *, display_wnd_params_t *, sysarg_t *);
static errno_t test_window_destroy(void *, sysarg_t);
static errno_t test_window_move_req(void *, sysarg_t, gfx_coord2_t *, sysarg_t);
static errno_t test_window_move(void *, sysarg_t, gfx_coord2_t *);
static errno_t test_window_get_pos(void *, sysarg_t, gfx_coord2_t *);
static errno_t test_window_get_max_rect(void *, sysarg_t, gfx_rect_t *);
static errno_t test_window_resize_req(void *, sysarg_t, display_wnd_rsztype_t,
    gfx_coord2_t *, sysarg_t);
static errno_t test_window_resize(void *, sysarg_t, gfx_coord2_t *,
    gfx_rect_t *);
static errno_t test_window_minimize(void *, sysarg_t);
static errno_t test_window_maximize(void *, sysarg_t);
static errno_t test_window_unmaximize(void *, sysarg_t);
static errno_t test_window_set_cursor(void *, sysarg_t, display_stock_cursor_t);
static errno_t test_window_set_caption(void *, sysarg_t, const char *);
static errno_t test_get_event(void *, sysarg_t *, display_wnd_ev_t *);
static errno_t test_get_info(void *, display_info_t *);

static errno_t test_gc_set_color(void *, gfx_color_t *);

static display_ops_t test_display_srv_ops = {
	.window_create = test_window_create,
	.window_destroy = test_window_destroy,
	.window_move_req = test_window_move_req,
	.window_move = test_window_move,
	.window_get_pos = test_window_get_pos,
	.window_get_max_rect = test_window_get_max_rect,
	.window_resize_req = test_window_resize_req,
	.window_resize = test_window_resize,
	.window_minimize = test_window_minimize,
	.window_maximize = test_window_maximize,
	.window_unmaximize = test_window_unmaximize,
	.window_set_cursor = test_window_set_cursor,
	.window_set_caption = test_window_set_caption,
	.get_event = test_get_event,
	.get_info = test_get_info
};

static display_wnd_cb_t test_display_wnd_cb = {
	.close_event = test_close_event,
	.focus_event = test_focus_event,
	.kbd_event = test_kbd_event,
	.pos_event = test_pos_event,
	.unfocus_event = test_unfocus_event
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
	gfx_rect_t create_rect;
	gfx_coord2_t create_min_size;
	sysarg_t create_idev_id;
	bool window_destroy_called;
	sysarg_t destroy_wnd_id;

	bool window_move_req_called;
	sysarg_t move_req_wnd_id;
	gfx_coord2_t move_req_pos;
	sysarg_t move_req_pos_id;

	bool window_move_called;
	sysarg_t move_wnd_id;
	gfx_coord2_t move_dpos;

	bool window_get_pos_called;
	sysarg_t get_pos_wnd_id;
	gfx_coord2_t get_pos_rpos;

	bool window_get_max_rect_called;
	sysarg_t get_max_rect_wnd_id;
	gfx_rect_t get_max_rect_rrect;

	bool window_resize_req_called;
	sysarg_t resize_req_wnd_id;
	display_wnd_rsztype_t resize_req_rsztype;
	gfx_coord2_t resize_req_pos;
	sysarg_t resize_req_pos_id;

	bool window_resize_called;
	gfx_coord2_t resize_offs;
	gfx_rect_t resize_nbound;
	sysarg_t resize_wnd_id;

	bool window_minimize_called;
	bool window_maximize_called;
	bool window_unmaximize_called;

	bool window_set_cursor_called;
	sysarg_t set_cursor_wnd_id;
	display_stock_cursor_t set_cursor_cursor;

	bool window_set_caption_called;
	sysarg_t set_caption_wnd_id;
	char *set_caption_caption;

	bool get_event_called;

	bool get_info_called;
	gfx_rect_t get_info_rect;

	bool set_color_called;
	bool close_event_called;

	bool focus_event_called;
	bool kbd_event_called;
	bool pos_event_called;
	bool unfocus_event_called;
	fibril_condvar_t event_cv;
	fibril_mutex_t event_lock;
	display_srv_t *srv;
} test_response_t;

/** display_open(), display_close() work for valid display service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_create() with server returning error response works */
PCUT_TEST(window_create_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = ENOMEM;
	resp.window_create_called = false;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;
	params.min_size.x = 11;
	params.min_size.y = 12;
	params.idev_id = 42;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.create_rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.create_rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.create_rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.create_rect.p1.y);
	PCUT_ASSERT_EQUALS(params.min_size.x, resp.create_min_size.x);
	PCUT_ASSERT_EQUALS(params.min_size.y, resp.create_min_size.y);
	PCUT_ASSERT_EQUALS(params.idev_id, resp.create_idev_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_NULL(wnd);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
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
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	resp.window_create_called = false;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;
	params.idev_id = 42;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.create_rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.create_rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.create_rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.create_rect.p1.y);
	PCUT_ASSERT_EQUALS(params.idev_id, resp.create_idev_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.window_destroy_called = false;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_TRUE(resp.window_destroy_called);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.destroy_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_create() with server returning error response works. */
PCUT_TEST(window_destroy_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	resp.window_create_called = false;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_TRUE(resp.window_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.create_rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.create_rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.create_rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.create_rect.p1.y);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_destroy_called = false;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_TRUE(resp.window_destroy_called);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.destroy_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_destroy() can handle NULL argument */
PCUT_TEST(window_destroy_null)
{
	display_window_destroy(NULL);
}

/** display_window_move_req() with server returning error response works. */
PCUT_TEST(window_move_req_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_move_req_called = false;

	pos.x = 42;
	pos.y = 43;
	pos_id = 44;

	rc = display_window_move_req(wnd, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.window_move_req_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.move_req_wnd_id);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.move_req_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.move_req_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.move_req_pos_id);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_move_req() with server returning success response works. */
PCUT_TEST(window_move_req_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_move_req_called = false;

	pos.x = 42;
	pos.y = 43;
	pos_id = 44;

	rc = display_window_move_req(wnd, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.window_move_req_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.move_req_wnd_id);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.move_req_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.move_req_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.move_req_pos_id);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_move() with server returning error response works. */
PCUT_TEST(window_move_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t dpos;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_move_called = false;
	dpos.x = 11;
	dpos.y = 12;

	rc = display_window_move(wnd, &dpos);
	PCUT_ASSERT_TRUE(resp.window_move_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.move_wnd_id);
	PCUT_ASSERT_INT_EQUALS(dpos.x, resp.move_dpos.x);
	PCUT_ASSERT_INT_EQUALS(dpos.y, resp.move_dpos.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_move() with server returning success response works. */
PCUT_TEST(window_move_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t dpos;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_move_called = false;
	dpos.x = 11;
	dpos.y = 12;

	rc = display_window_move(wnd, &dpos);
	PCUT_ASSERT_TRUE(resp.window_move_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.move_wnd_id);
	PCUT_ASSERT_INT_EQUALS(dpos.x, resp.move_dpos.x);
	PCUT_ASSERT_INT_EQUALS(dpos.y, resp.move_dpos.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_pos() with server returning error response works. */
PCUT_TEST(window_get_pos_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t dpos;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_get_pos_called = false;

	dpos.x = 0;
	dpos.y = 0;

	rc = display_window_get_pos(wnd, &dpos);
	PCUT_ASSERT_TRUE(resp.window_get_pos_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.get_pos_wnd_id);
	PCUT_ASSERT_INT_EQUALS(0, dpos.x);
	PCUT_ASSERT_INT_EQUALS(0, dpos.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_pos() with server returning success response works. */
PCUT_TEST(window_get_pos_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t dpos;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_get_pos_called = false;
	resp.get_pos_rpos.x = 11;
	resp.get_pos_rpos.y = 12;

	dpos.x = 0;
	dpos.y = 0;

	rc = display_window_get_pos(wnd, &dpos);
	PCUT_ASSERT_TRUE(resp.window_get_pos_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.get_pos_wnd_id);
	PCUT_ASSERT_INT_EQUALS(resp.get_pos_rpos.x, dpos.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_pos_rpos.y, dpos.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_max_rect() with server returning error response works. */
PCUT_TEST(window_get_max_rect_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_rect_t rect;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_get_max_rect_called = false;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 0;
	rect.p1.y = 0;

	rc = display_window_get_max_rect(wnd, &rect);
	PCUT_ASSERT_TRUE(resp.window_get_max_rect_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.get_max_rect_wnd_id);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_max_rect() with server returning success response works. */
PCUT_TEST(window_get_max_rect_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_rect_t rect;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_get_max_rect_called = false;
	resp.get_max_rect_rrect.p0.x = 11;
	resp.get_max_rect_rrect.p0.y = 12;
	resp.get_max_rect_rrect.p1.x = 13;
	resp.get_max_rect_rrect.p1.y = 14;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 0;
	rect.p1.y = 0;

	rc = display_window_get_max_rect(wnd, &rect);
	PCUT_ASSERT_TRUE(resp.window_get_max_rect_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.get_max_rect_wnd_id);
	PCUT_ASSERT_INT_EQUALS(resp.get_max_rect_rrect.p0.x, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_max_rect_rrect.p0.y, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(resp.get_max_rect_rrect.p1.x, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_max_rect_rrect.p1.y, rect.p1.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_resize_req() with server returning error response works. */
PCUT_TEST(window_resize_req_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	display_wnd_rsztype_t rsztype;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_resize_req_called = false;

	rsztype = display_wr_top_right;
	pos.x = 42;
	pos.y = 43;
	pos_id = 44;

	rc = display_window_resize_req(wnd, rsztype, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.window_resize_req_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(rsztype, resp.resize_req_rsztype);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.resize_req_wnd_id);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.resize_req_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.resize_req_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.resize_req_pos_id);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_resize_req() with server returning success response works. */
PCUT_TEST(window_resize_req_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	display_wnd_rsztype_t rsztype;
	gfx_coord2_t pos;
	sysarg_t pos_id;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_resize_req_called = false;

	rsztype = display_wr_top_right;
	pos.x = 42;
	pos.y = 43;
	pos_id = 44;

	rc = display_window_resize_req(wnd, rsztype, &pos, pos_id);
	PCUT_ASSERT_TRUE(resp.window_resize_req_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(rsztype, resp.resize_req_rsztype);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.resize_req_wnd_id);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.resize_req_pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.resize_req_pos.y);
	PCUT_ASSERT_INT_EQUALS(pos_id, resp.resize_req_pos_id);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_resize() with server returning error response works. */
PCUT_TEST(window_resize_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t offs;
	gfx_rect_t nrect;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_resize_called = false;
	offs.x = 11;
	offs.y = 12;
	nrect.p0.x = 13;
	nrect.p0.y = 14;
	nrect.p1.x = 15;
	nrect.p1.y = 16;

	rc = display_window_resize(wnd, &offs, &nrect);
	PCUT_ASSERT_TRUE(resp.window_resize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.resize_wnd_id);
	PCUT_ASSERT_INT_EQUALS(offs.x, resp.resize_offs.x);
	PCUT_ASSERT_INT_EQUALS(offs.y, resp.resize_offs.y);
	PCUT_ASSERT_INT_EQUALS(nrect.p0.x, resp.resize_nbound.p0.x);
	PCUT_ASSERT_INT_EQUALS(nrect.p0.y, resp.resize_nbound.p0.y);
	PCUT_ASSERT_INT_EQUALS(nrect.p1.x, resp.resize_nbound.p1.x);
	PCUT_ASSERT_INT_EQUALS(nrect.p1.y, resp.resize_nbound.p1.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_resize() with server returning success response works. */
PCUT_TEST(window_resize_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	gfx_coord2_t offs;
	gfx_rect_t nrect;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_resize_called = false;
	offs.x = 11;
	offs.y = 12;
	nrect.p0.x = 13;
	nrect.p0.y = 14;
	nrect.p1.x = 15;
	nrect.p1.y = 16;

	rc = display_window_resize(wnd, &offs, &nrect);
	PCUT_ASSERT_TRUE(resp.window_resize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(offs.x, resp.resize_offs.x);
	PCUT_ASSERT_INT_EQUALS(offs.y, resp.resize_offs.y);
	PCUT_ASSERT_INT_EQUALS(nrect.p0.x, resp.resize_nbound.p0.x);
	PCUT_ASSERT_INT_EQUALS(nrect.p0.y, resp.resize_nbound.p0.y);
	PCUT_ASSERT_INT_EQUALS(nrect.p1.x, resp.resize_nbound.p1.x);
	PCUT_ASSERT_INT_EQUALS(nrect.p1.y, resp.resize_nbound.p1.y);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_minimize() with server returning error response works. */
PCUT_TEST(window_minimize_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_minimize_called = false;

	rc = display_window_minimize(wnd);
	PCUT_ASSERT_TRUE(resp.window_minimize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_minimize() with server returning success response works. */
PCUT_TEST(window_minimize_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_minimize_called = false;

	rc = display_window_minimize(wnd);
	PCUT_ASSERT_TRUE(resp.window_minimize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_maximize() with server returning error response works. */
PCUT_TEST(window_maximize_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_maximize_called = false;

	rc = display_window_maximize(wnd);
	PCUT_ASSERT_TRUE(resp.window_maximize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_maximize() with server returning success response works. */
PCUT_TEST(window_maximize_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_maximize_called = false;

	rc = display_window_maximize(wnd);
	PCUT_ASSERT_TRUE(resp.window_maximize_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_set_cursor() with server returning error response works. */
PCUT_TEST(window_set_cursor_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EIO;
	resp.window_set_cursor_called = false;

	rc = display_window_set_cursor(wnd, dcurs_size_ud);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.set_cursor_wnd_id);
	PCUT_ASSERT_TRUE(resp.window_set_cursor_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(dcurs_size_ud, resp.set_cursor_cursor);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_set_cursor() with server returning success response works. */
PCUT_TEST(window_set_cursor_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.rc = EOK;
	resp.window_set_cursor_called = false;

	rc = display_window_set_cursor(wnd, dcurs_size_ud);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.set_cursor_wnd_id);
	PCUT_ASSERT_TRUE(resp.window_set_cursor_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, EOK);
	PCUT_ASSERT_INT_EQUALS(dcurs_size_ud, resp.set_cursor_cursor);

	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_set_caption() with server returning error response works. */
PCUT_TEST(window_set_caption_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	const char *caption;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	caption = "Hello";

	resp.rc = EIO;
	resp.window_set_caption_called = false;

	rc = display_window_set_caption(wnd, caption);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.set_caption_wnd_id);
	PCUT_ASSERT_TRUE(resp.window_set_caption_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(caption, resp.set_caption_caption));

	//free(resp.set_caption_caption);
	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_set_caption() with server returning success response works. */
PCUT_TEST(window_set_caption_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	const char *caption;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	caption = "Hello";

	resp.rc = EOK;
	resp.window_set_caption_called = false;

	rc = display_window_set_caption(wnd, caption);
	PCUT_ASSERT_INT_EQUALS(wnd->id, resp.set_caption_wnd_id);
	PCUT_ASSERT_TRUE(resp.window_set_caption_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(caption, resp.set_caption_caption));

	//free(resp.set_caption_caption);
	display_window_destroy(wnd);
	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_gc with server returning failure */
PCUT_TEST(window_get_gc_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	gfx_context_t *gc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	gc = NULL;
	resp.rc = ENOMEM;
	rc = display_window_get_gc(wnd, &gc);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_NULL(gc);

	resp.rc = EOK;
	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_window_get_gc with server returning success */
PCUT_TEST(window_get_gc_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_color_t *color;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
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
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Close event can be delivered from server to client callback function */
PCUT_TEST(close_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.event_cnt = 1;
	resp.event.etype = wev_close;
	resp.wnd_id = wnd->id;
	resp.close_event_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.close_event_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Focus event can be delivered from server to client callback function */
PCUT_TEST(focus_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.event_cnt = 1;
	resp.event.etype = wev_focus;
	resp.event.ev.focus.nfocus = 42;
	resp.wnd_id = wnd->id;
	resp.focus_event_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.focus_event_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.focus.nfocus,
	    resp.revent.ev.focus.nfocus);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Keyboard event can be delivered from server to client callback function */
PCUT_TEST(kbd_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.event_cnt = 1;
	resp.event.etype = wev_kbd;
	resp.event.ev.kbd.type = KEY_PRESS;
	resp.event.ev.kbd.key = KC_ENTER;
	resp.event.ev.kbd.mods = 0;
	resp.event.ev.kbd.c = L'\0';
	resp.wnd_id = wnd->id;
	resp.kbd_event_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.kbd_event_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.kbd.type,
	    resp.revent.ev.kbd.type);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.kbd.key,
	    resp.revent.ev.kbd.key);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.kbd.mods,
	    resp.revent.ev.kbd.mods);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.kbd.c,
	    resp.revent.ev.kbd.c);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Position event can be delivered from server to client callback function */
PCUT_TEST(pos_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.event_cnt = 1;
	resp.event.etype = wev_pos;
	resp.event.ev.pos.type = POS_PRESS;
	resp.event.ev.pos.btn_num = 1;
	resp.event.ev.pos.hpos = 2;
	resp.event.ev.pos.vpos = 3;
	resp.wnd_id = wnd->id;
	resp.pos_event_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.pos_event_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.pos.type,
	    resp.revent.ev.pos.type);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.pos.btn_num,
	    resp.revent.ev.pos.btn_num);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.pos.hpos,
	    resp.revent.ev.pos.hpos);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.pos.vpos,
	    resp.revent.ev.pos.vpos);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Unfocus event can be delivered from server to client callback function */
PCUT_TEST(unfocus_event_deliver)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_wnd_params_t params;
	display_window_t *wnd;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	wnd = NULL;
	resp.rc = EOK;
	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p0.x = 100;
	params.rect.p0.y = 100;

	rc = display_window_create(disp, &params, &test_display_wnd_cb,
	    (void *) &resp, &wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wnd);

	resp.event_cnt = 1;
	resp.event.etype = wev_unfocus;
	resp.event.ev.unfocus.nfocus = 42;
	resp.wnd_id = wnd->id;
	resp.unfocus_event_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	display_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.unfocus_event_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);
	PCUT_ASSERT_INT_EQUALS(resp.event.ev.focus.nfocus,
	    resp.revent.ev.focus.nfocus);

	rc = display_window_destroy(wnd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	display_close(disp);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_get_info() with server returning failure response works. */
PCUT_TEST(get_info_failure)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_info_t info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = ENOMEM;
	resp.get_info_called = false;

	rc = display_get_info(disp, &info);
	PCUT_ASSERT_TRUE(resp.get_info_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** display_get_info() with server returning success response works. */
PCUT_TEST(get_info_success)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;
	display_info_t info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_display_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_display_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_display_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_open(test_display_svc, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(disp);

	resp.rc = EOK;
	resp.get_info_called = false;
	resp.get_info_rect.p0.x = 10;
	resp.get_info_rect.p0.y = 11;
	resp.get_info_rect.p1.x = 20;
	resp.get_info_rect.p1.y = 21;

	rc = display_get_info(disp, &info);
	PCUT_ASSERT_TRUE(resp.get_info_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_INT_EQUALS(resp.get_info_rect.p0.x, info.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_info_rect.p0.y, info.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(resp.get_info_rect.p1.x, info.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(resp.get_info_rect.p1.y, info.rect.p1.y);

	display_close(disp);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
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

static void test_close_event(void *arg)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wev_close;

	fibril_mutex_lock(&resp->event_lock);
	resp->close_event_called = true;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_focus_event(void *arg, unsigned nfocus)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wev_focus;
	resp->revent.ev.focus.nfocus = nfocus;

	fibril_mutex_lock(&resp->event_lock);
	resp->focus_event_called = true;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_kbd_event(void *arg, kbd_event_t *event)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wev_kbd;
	resp->revent.ev.kbd = *event;

	fibril_mutex_lock(&resp->event_lock);
	resp->kbd_event_called = true;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_pos_event(void *arg, pos_event_t *event)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wev_pos;
	resp->revent.ev.pos = *event;

	fibril_mutex_lock(&resp->event_lock);
	resp->pos_event_called = true;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_unfocus_event(void *arg, unsigned nfocus)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wev_unfocus;
	resp->revent.ev.unfocus.nfocus = nfocus;

	fibril_mutex_lock(&resp->event_lock);
	resp->unfocus_event_called = true;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static errno_t test_window_create(void *arg, display_wnd_params_t *params,
    sysarg_t *rwnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_create_called = true;
	resp->create_rect = params->rect;
	resp->create_min_size = params->min_size;
	resp->create_idev_id = params->idev_id;
	if (resp->rc == EOK)
		*rwnd_id = resp->wnd_id;

	return resp->rc;
}

static errno_t test_window_destroy(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_destroy_called = true;
	resp->destroy_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_window_move_req(void *arg, sysarg_t wnd_id,
    gfx_coord2_t *pos, sysarg_t pos_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_move_req_called = true;
	resp->move_req_wnd_id = wnd_id;
	resp->move_req_pos = *pos;
	resp->move_req_pos_id = pos_id;
	return resp->rc;
}

static errno_t test_window_move(void *arg, sysarg_t wnd_id, gfx_coord2_t *dpos)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_move_called = true;
	resp->move_wnd_id = wnd_id;
	resp->move_dpos = *dpos;
	return resp->rc;
}

static errno_t test_window_get_pos(void *arg, sysarg_t wnd_id, gfx_coord2_t *dpos)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_get_pos_called = true;
	resp->get_pos_wnd_id = wnd_id;

	if (resp->rc == EOK)
		*dpos = resp->get_pos_rpos;

	return resp->rc;
}

static errno_t test_window_get_max_rect(void *arg, sysarg_t wnd_id,
    gfx_rect_t *rect)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_get_max_rect_called = true;
	resp->get_max_rect_wnd_id = wnd_id;

	if (resp->rc == EOK)
		*rect = resp->get_max_rect_rrect;

	return resp->rc;
}

static errno_t test_window_resize_req(void *arg, sysarg_t wnd_id,
    display_wnd_rsztype_t rsztype, gfx_coord2_t *pos, sysarg_t pos_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_resize_req_called = true;
	resp->resize_req_rsztype = rsztype;
	resp->resize_req_wnd_id = wnd_id;
	resp->resize_req_pos = *pos;
	resp->resize_req_pos_id = pos_id;
	return resp->rc;
}

static errno_t test_window_resize(void *arg, sysarg_t wnd_id,
    gfx_coord2_t *offs, gfx_rect_t *nrect)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_resize_called = true;
	resp->resize_wnd_id = wnd_id;
	resp->resize_offs = *offs;
	resp->resize_nbound = *nrect;
	return resp->rc;
}

static errno_t test_window_minimize(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_minimize_called = true;
	resp->resize_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_window_maximize(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_maximize_called = true;
	resp->resize_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_window_unmaximize(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_unmaximize_called = true;
	resp->resize_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_window_set_cursor(void *arg, sysarg_t wnd_id,
    display_stock_cursor_t cursor)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_set_cursor_called = true;
	resp->set_cursor_wnd_id = wnd_id;
	resp->set_cursor_cursor = cursor;

	return resp->rc;
}

static errno_t test_window_set_caption(void *arg, sysarg_t wnd_id,
    const char *caption)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->window_set_caption_called = true;
	resp->set_caption_wnd_id = wnd_id;
	resp->set_caption_caption = str_dup(caption);

	return resp->rc;
}

static errno_t test_get_event(void *arg, sysarg_t *wnd_id,
    display_wnd_ev_t *event)
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

static errno_t test_get_info(void *arg, display_info_t *info)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_info_called = true;
	info->rect = resp->get_info_rect;

	return resp->rc;
}

static errno_t test_gc_set_color(void *arg, gfx_color_t *color)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->set_color_called = true;
	return resp->rc;
}

PCUT_EXPORT(display);
