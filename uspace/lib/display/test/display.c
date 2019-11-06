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
#include <loc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(display);

static const char *test_display_server = "test-display";
static const char *test_display_svc = "test/display";

static void test_display_conn(ipc_call_t *, void *);

static display_ops_t test_display_srv_ops;

/** display_open(), display_close() work for valid display service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	display_t *disp = NULL;

	async_set_fallback_port_handler(test_display_conn, disp);

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
	// TODO
}

/** display_window_create() with server returning success */
PCUT_TEST(window_create_success)
{
	// TODO
}

/** display_window_create() with server returning error response works */
PCUT_TEST(window_destroy_failure)
{
	// TODO
}

/** display_window_create() with server returning success */
PCUT_TEST(window_destroy_success)
{
	// TODO
}

/** display_window_get_gc with server returning failure */
PCUT_TEST(window_get_gc_failure)
{
	// TODO
}

/** display_window_get_gc with server returning success */
PCUT_TEST(window_get_gc_success)
{
	// TODO
}

static void test_display_conn(ipc_call_t *icall, void *arg)
{
	display_srv_t srv;

	display_srv_initialize(&srv);
	srv.ops = &test_display_srv_ops;
	srv.arg = NULL;

	display_conn(icall, &srv);
}

PCUT_EXPORT(display);
