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

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <loc.h>
#include <pcut/pcut.h>
#include <str.h>
#include <system.h>
#include <system_srv.h>
#include "../private/system.h"

PCUT_INIT;

PCUT_TEST_SUITE(system);

static const char *test_system_server = "test-system";
static const char *test_system_svc = "test/system";

void test_system_conn(ipc_call_t *, void *);

static errno_t test_shutdown(void *);

static void test_sys_shutdown_complete(void *);
static void test_sys_shutdown_failed(void *);

static system_ops_t test_system_srv_ops = {
	.shutdown = test_shutdown
};

system_cb_t test_system_cb = {
	.shutdown_complete = test_sys_shutdown_complete,
	.shutdown_failed = test_sys_shutdown_failed
};

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;

	bool shutdown_called;
	bool shutdown_complete_called;
	bool shutdown_failed_called;

	fibril_condvar_t event_cv;
	fibril_mutex_t event_lock;
	system_srv_t *srv;
} test_response_t;

/** system_open(), system_close() work for valid system control service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	system_t *system = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_system_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_system_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_system_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = system_open(test_system_svc, NULL, NULL, &system);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(system);

	system_close(system);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** system_shutdown() with server returning error response works */
PCUT_TEST(shutdown_failure)
{
	errno_t rc;
	service_id_t sid;
	system_t *system = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_system_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_system_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_system_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = system_open(test_system_svc, NULL, NULL, &system);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(system);

	resp.rc = ENOMEM;
	resp.shutdown_called = false;

	rc = system_shutdown(system);
	PCUT_ASSERT_TRUE(resp.shutdown_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	system_close(system);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** system_shutdown() with server returning success response works */
PCUT_TEST(shutdown_success)
{
	errno_t rc;
	service_id_t sid;
	system_t *system = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_system_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_system_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_system_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = system_open(test_system_svc, NULL, NULL, &system);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(system);

	resp.rc = EOK;
	resp.shutdown_called = false;

	rc = system_shutdown(system);
	PCUT_ASSERT_TRUE(resp.shutdown_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	system_close(system);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** 'Shutdown complete' event is delivered from server to client callback
 * function.
 */
PCUT_TEST(shutdown_complete)
{
	errno_t rc;
	service_id_t sid;
	system_t *system = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_system_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_system_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_system_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = system_open(test_system_svc, &test_system_cb, &resp, &system);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(system);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.shutdown_complete_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	system_srv_shutdown_complete(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.shutdown_complete_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	system_close(system);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** 'Shutdown failed' event is delivered from server to client callback
 * function.
 */
PCUT_TEST(shutdown_failed)
{
	errno_t rc;
	service_id_t sid;
	system_t *system = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_system_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_system_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_system_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = system_open(test_system_svc, &test_system_cb, &resp, &system);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(system);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.shutdown_failed_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	system_srv_shutdown_failed(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.shutdown_failed_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	system_close(system);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Test system control service connection. */
void test_system_conn(ipc_call_t *icall, void *arg)
{
	test_response_t *resp = (test_response_t *)arg;
	system_srv_t srv;

	/* Set up protocol structure */
	system_srv_initialize(&srv);
	srv.ops = &test_system_srv_ops;
	srv.arg = arg;
	resp->srv = &srv;

	/* Handle connection */
	system_conn(icall, &srv);

	resp->srv = NULL;
}

/** Test system shutdown.
 *
 * @param arg Argument (test_response_t *)
 */
static errno_t test_shutdown(void *arg)
{
	test_response_t *resp = (test_response_t *)arg;

	resp->shutdown_called = true;
	return resp->rc;
}

/** Test system shutdown complete.
 *
 * @param arg Argument (test_response_t *)
 */
static void test_sys_shutdown_complete(void *arg)
{
	test_response_t *resp = (test_response_t *)arg;

	resp->shutdown_complete_called = true;
	fibril_condvar_signal(&resp->event_cv);
}

/** Test system shutdown failed.
 *
 * @param arg Argument (test_response_t *)
 */
static void test_sys_shutdown_failed(void *arg)
{
	test_response_t *resp = (test_response_t *)arg;

	resp->shutdown_failed_called = true;
}

PCUT_EXPORT(system);
