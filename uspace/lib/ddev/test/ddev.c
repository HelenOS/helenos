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
#include <ddev.h>
#include <ddev_srv.h>
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <ipcgfx/server.h>
#include <loc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(ddev);

static const char *test_ddev_server = "test-ddev";
static const char *test_ddev_svc = "test/ddev";

static void test_ddev_conn(ipc_call_t *, void *);

static errno_t test_get_gc(void *, sysarg_t *, sysarg_t *);
static errno_t test_get_info(void *, ddev_info_t *);
static errno_t test_gc_set_color(void *, gfx_color_t *);

static ddev_ops_t test_ddev_ops = {
	.get_gc = test_get_gc,
	.get_info = test_get_info
};

static gfx_context_ops_t test_gc_ops = {
	.set_color = test_gc_set_color
};

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;
	bool set_color_called;
	ddev_srv_t *srv;
	ddev_info_t info;
} test_response_t;

/** ddev_open(), ddev_close() work for valid display device service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	ddev_t *ddev = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ddev_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ddev_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ddev_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ddev_open(test_ddev_svc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ddev);

	ddev_close(ddev);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** ddev_get_gc with server returning failure */
PCUT_TEST(dev_get_gc_failure)
{
	errno_t rc;
	service_id_t sid;
	ddev_t *ddev = NULL;
	test_response_t resp;
	gfx_context_t *gc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ddev_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ddev_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ddev_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ddev_open(test_ddev_svc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ddev);

	gc = NULL;
	resp.rc = ENOMEM;
	rc = ddev_get_gc(ddev, &gc);
	PCUT_ASSERT_ERRNO_VAL(ENOMEM, rc);
	PCUT_ASSERT_NULL(gc);

	ddev_close(ddev);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** ddev_get_gc with server returning success */
PCUT_TEST(dev_get_gc_success)
{
	errno_t rc;
	service_id_t sid;
	ddev_t *ddev = NULL;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_color_t *color;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ddev_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ddev_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ddev_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ddev_open(test_ddev_svc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ddev);

	resp.rc = EOK;
	gc = NULL;
	rc = ddev_get_gc(ddev, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gc);

	rc = gfx_color_new_rgb_i16(0, 0, 0, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.set_color_called = false;
	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.set_color_called);

	gfx_color_delete(color);

	ddev_close(ddev);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** ddev_get_info with server returning failure */
PCUT_TEST(dev_get_info_failure)
{
	errno_t rc;
	service_id_t sid;
	ddev_t *ddev = NULL;
	test_response_t resp;
	ddev_info_t info;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ddev_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ddev_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ddev_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ddev_open(test_ddev_svc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ddev);

	resp.rc = ENOMEM;
	rc = ddev_get_info(ddev, &info);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	ddev_close(ddev);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** ddev_get_info with server returning success */
PCUT_TEST(dev_get_info_success)
{
	errno_t rc;
	service_id_t sid;
	ddev_t *ddev = NULL;
	test_response_t resp;
	ddev_info_t info;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ddev_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ddev_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ddev_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ddev_open(test_ddev_svc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(ddev);

	resp.rc = EOK;

	ddev_info_init(&resp.info);
	resp.info.rect.p0.x = 1;
	resp.info.rect.p0.y = 2;
	resp.info.rect.p1.x = 3;
	resp.info.rect.p1.y = 4;

	rc = ddev_get_info(ddev, &info);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_INT_EQUALS(resp.info.rect.p0.x, info.rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(resp.info.rect.p0.y, info.rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(resp.info.rect.p1.x, info.rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(resp.info.rect.p1.y, info.rect.p1.y);

	ddev_close(ddev);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Test display device connection.
 *
 * This is very similar to connection handler in the display server.
 * XXX This should be folded into display_srv, if possible
 */
static void test_ddev_conn(ipc_call_t *icall, void *arg)
{
	test_response_t *resp = (test_response_t *) arg;
	ddev_srv_t srv;
	sysarg_t svc_id;
	gfx_context_t *gc;
	errno_t rc;

	svc_id = ipc_get_arg2(icall);

	if (svc_id != 0) {
		/* Set up protocol structure */
		ddev_srv_initialize(&srv);
		srv.ops = &test_ddev_ops;
		srv.arg = arg;
		resp->srv = &srv;

		/* Handle connection */
		ddev_conn(icall, &srv);

		resp->srv = NULL;
	} else {
		if (resp->rc != EOK) {
			async_answer_0(icall, resp->rc);
			return;
		}

		rc = gfx_context_new(&test_gc_ops, arg, &gc);
		if (rc != EOK) {
			async_answer_0(icall, ENOMEM);
			return;
		}

		/* GC connection */
		gc_conn(icall, gc);
	}
}

static errno_t test_get_gc(void *arg, sysarg_t *arg2, sysarg_t *arg3)
{
	*arg2 = 0;
	*arg3 = 42;
	return EOK;
}

static errno_t test_get_info(void *arg, ddev_info_t *info)
{
	test_response_t *resp = (test_response_t *) arg;

	if (resp->rc != EOK)
		return resp->rc;

	*info = resp->info;
	return EOK;
}

static errno_t test_gc_set_color(void *arg, gfx_color_t *color)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->set_color_called = true;
	return resp->rc;
}

PCUT_EXPORT(ddev);
