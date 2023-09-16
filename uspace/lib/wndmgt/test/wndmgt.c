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
#include <wndmgt.h>
#include <wndmgt_srv.h>
#include <fibril_synch.h>
#include <loc.h>
#include <pcut/pcut.h>
#include <str.h>
#include "../private/wndmgt.h"

PCUT_INIT;

PCUT_TEST_SUITE(wndmgt);

static const char *test_wndmgt_server = "test-wndmgt";
static const char *test_wndmgt_svc = "test/wndmgt";

static void test_wndmgt_conn(ipc_call_t *, void *);

static errno_t test_get_window_list(void *, wndmgt_window_list_t **);
static errno_t test_get_window_info(void *, sysarg_t, wndmgt_window_info_t **);
static errno_t test_activate_window(void *, sysarg_t, sysarg_t);
static errno_t test_close_window(void *, sysarg_t);
static errno_t test_get_event(void *, wndmgt_ev_t *);

static void test_window_added(void *, sysarg_t);
static void test_window_removed(void *, sysarg_t);
static void test_window_changed(void *, sysarg_t);

static wndmgt_ops_t test_wndmgt_srv_ops = {
	.get_window_list = test_get_window_list,
	.get_window_info = test_get_window_info,
	.activate_window = test_activate_window,
	.close_window = test_close_window,
	.get_event = test_get_event
};

static wndmgt_cb_t test_wndmgt_cb = {
	.window_added = test_window_added,
	.window_removed = test_window_removed,
	.window_changed = test_window_changed
};

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;
	sysarg_t wnd_id;
	wndmgt_ev_t event;
	wndmgt_ev_t revent;
	int event_cnt;

	bool get_window_list_called;
	wndmgt_window_list_t *get_window_list_rlist;

	bool get_window_info_called;
	sysarg_t get_window_info_wnd_id;
	wndmgt_window_info_t *get_window_info_rinfo;

	bool activate_window_called;
	sysarg_t activate_window_seat_id;
	sysarg_t activate_window_wnd_id;

	bool close_window_called;
	sysarg_t close_window_wnd_id;

	bool get_event_called;

	bool window_added_called;
	sysarg_t window_added_wnd_id;

	bool window_removed_called;
	sysarg_t window_removed_wnd_id;

	bool window_changed_called;
	sysarg_t window_changed_wnd_id;

	fibril_condvar_t event_cv;
	fibril_mutex_t event_lock;
	wndmgt_srv_t *srv;
} test_response_t;

/** wndmgt_open(), wndmgt_close() work for valid window management service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_get_window_list() with server returning error response works */
PCUT_TEST(get_window_list_failure)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	wndmgt_window_list_t *list;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	resp.rc = ENOMEM;
	resp.get_window_list_called = false;

	rc = wndmgt_get_window_list(wndmgt, &list);
	PCUT_ASSERT_TRUE(resp.get_window_list_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_get_window_list() with server returning success response works */
PCUT_TEST(get_window_list_success)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	wndmgt_window_list_t *list;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	resp.rc = EOK;
	resp.get_window_list_called = false;
	resp.get_window_list_rlist = calloc(1, sizeof(wndmgt_window_list_t));
	PCUT_ASSERT_NOT_NULL(resp.get_window_list_rlist);
	resp.get_window_list_rlist->nwindows = 2;
	resp.get_window_list_rlist->windows = calloc(2, sizeof(sysarg_t));
	PCUT_ASSERT_NOT_NULL(resp.get_window_list_rlist->windows);
	resp.get_window_list_rlist->windows[0] = 42;
	resp.get_window_list_rlist->windows[1] = 43;

	rc = wndmgt_get_window_list(wndmgt, &list);
	PCUT_ASSERT_TRUE(resp.get_window_list_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_INT_EQUALS(2, list->nwindows);
	PCUT_ASSERT_INT_EQUALS(42, list->windows[0]);
	PCUT_ASSERT_INT_EQUALS(43, list->windows[1]);

	wndmgt_free_window_list(list);
	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_get_window_infp() with server returning error response works */
PCUT_TEST(get_window_info_failure)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t wnd_id;
	wndmgt_window_info_t *info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	resp.rc = ENOMEM;
	resp.get_window_info_called = false;
	wnd_id = 1;

	rc = wndmgt_get_window_info(wndmgt, wnd_id, &info);
	PCUT_ASSERT_TRUE(resp.get_window_info_called);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.get_window_info_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_get_window_info() with server returning success response works */
PCUT_TEST(get_window_info_success)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t wnd_id;
	wndmgt_window_info_t *info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	resp.rc = EOK;
	resp.get_window_info_called = false;
	resp.get_window_info_rinfo = calloc(1, sizeof(wndmgt_window_info_t));
	PCUT_ASSERT_NOT_NULL(resp.get_window_info_rinfo);
	resp.get_window_info_rinfo->caption = str_dup("Hello");
	resp.get_window_info_rinfo->flags = 42;
	resp.get_window_info_rinfo->nfocus = 123;
	PCUT_ASSERT_NOT_NULL(resp.get_window_info_rinfo->caption);
	wnd_id = 1;

	rc = wndmgt_get_window_info(wndmgt, wnd_id, &info);
	PCUT_ASSERT_TRUE(resp.get_window_info_called);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.get_window_info_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_STR_EQUALS("Hello", info->caption);
	PCUT_ASSERT_INT_EQUALS(42, info->flags);
	PCUT_ASSERT_INT_EQUALS(123, info->nfocus);

	wndmgt_free_window_info(info);
	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_activate_window() with server returning error response works */
PCUT_TEST(activate_window_failure)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t seat_id;
	sysarg_t wnd_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	seat_id = 13;
	wnd_id = 42;
	resp.rc = ENOMEM;
	resp.activate_window_called = false;

	rc = wndmgt_activate_window(wndmgt, seat_id, wnd_id);
	PCUT_ASSERT_TRUE(resp.activate_window_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.activate_window_seat_id);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.activate_window_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_activate_window() with server returning success response works */
PCUT_TEST(activate_window_success)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t seat_id;
	sysarg_t wnd_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	seat_id = 13;
	wnd_id = 42;
	resp.rc = EOK;
	resp.activate_window_called = false;

	rc = wndmgt_activate_window(wndmgt, seat_id, wnd_id);
	PCUT_ASSERT_TRUE(resp.activate_window_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.activate_window_seat_id);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.activate_window_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_close_window() with server returning error response works */
PCUT_TEST(close_window_failure)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t wnd_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	wnd_id = 42;
	resp.rc = ENOMEM;
	resp.close_window_called = false;

	rc = wndmgt_close_window(wndmgt, wnd_id);
	PCUT_ASSERT_TRUE(resp.close_window_called);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.close_window_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** wndmgt_close_window() with server returning success response works */
PCUT_TEST(close_window_success)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	sysarg_t wnd_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, NULL, NULL, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);

	wnd_id = 42;
	resp.rc = EOK;
	resp.close_window_called = false;

	rc = wndmgt_close_window(wndmgt, wnd_id);
	PCUT_ASSERT_TRUE(resp.close_window_called);
	PCUT_ASSERT_INT_EQUALS(wnd_id, resp.close_window_wnd_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	wndmgt_close(wndmgt);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Window added event can be delivered from server to client callback function */
PCUT_TEST(window_added_deliver)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, &test_wndmgt_cb, &resp, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.event_cnt = 1;
	resp.event.etype = wmev_window_added;
	resp.event.wnd_id = 42;
	resp.window_added_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	wndmgt_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.window_added_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	wndmgt_close(wndmgt);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Window removed event can be delivered from server to client callback function */
PCUT_TEST(window_removed_deliver)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, &test_wndmgt_cb, &resp, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.event_cnt = 1;
	resp.event.etype = wmev_window_removed;
	resp.event.wnd_id = 42;
	resp.window_removed_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	wndmgt_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.window_removed_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	wndmgt_close(wndmgt);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Window changed event can be delivered from server to client callback function */
PCUT_TEST(window_changed_deliver)
{
	errno_t rc;
	service_id_t sid;
	wndmgt_t *wndmgt = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_wndmgt_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_wndmgt_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_wndmgt_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = wndmgt_open(test_wndmgt_svc, &test_wndmgt_cb, &resp, &wndmgt);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wndmgt);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.event_cnt = 1;
	resp.event.etype = wmev_window_changed;
	resp.event.wnd_id = 42;
	resp.window_changed_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	wndmgt_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.window_changed_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	wndmgt_close(wndmgt);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Test window management service connection. */
static void test_wndmgt_conn(ipc_call_t *icall, void *arg)
{
	test_response_t *resp = (test_response_t *) arg;
	wndmgt_srv_t srv;

	/* Set up protocol structure */
	wndmgt_srv_initialize(&srv);
	srv.ops = &test_wndmgt_srv_ops;
	srv.arg = arg;
	resp->srv = &srv;

	/* Handle connection */
	wndmgt_conn(icall, &srv);

	resp->srv = NULL;
}

static void test_window_added(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wmev_window_added;

	fibril_mutex_lock(&resp->event_lock);
	resp->window_added_called = true;
	resp->window_added_wnd_id = wnd_id;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_window_removed(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wmev_window_removed;

	fibril_mutex_lock(&resp->event_lock);
	resp->window_removed_called = true;
	resp->window_removed_wnd_id = wnd_id;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_window_changed(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = wmev_window_changed;

	fibril_mutex_lock(&resp->event_lock);
	resp->window_changed_called = true;
	resp->window_changed_wnd_id = wnd_id;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static errno_t test_get_window_list(void *arg, wndmgt_window_list_t **rlist)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_window_list_called = true;

	if (resp->rc != EOK)
		return resp->rc;

	*rlist = resp->get_window_list_rlist;
	return EOK;
}

static errno_t test_get_window_info(void *arg, sysarg_t wnd_id,
    wndmgt_window_info_t **rinfo)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_window_info_called = true;
	resp->get_window_info_wnd_id = wnd_id;

	if (resp->rc != EOK)
		return resp->rc;

	*rinfo = resp->get_window_info_rinfo;
	return EOK;
}

static errno_t test_activate_window(void *arg, sysarg_t seat_id,
    sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->activate_window_called = true;
	resp->activate_window_seat_id = seat_id;
	resp->activate_window_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_close_window(void *arg, sysarg_t wnd_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->close_window_called = true;
	resp->close_window_wnd_id = wnd_id;
	return resp->rc;
}

static errno_t test_get_event(void *arg, wndmgt_ev_t *event)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_event_called = true;
	if (resp->event_cnt > 0) {
		--resp->event_cnt;
		*event = resp->event;
		return EOK;
	}

	return ENOENT;
}

PCUT_EXPORT(wndmgt);
