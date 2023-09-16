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

#include <as.h>
#include <async.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <ipcgfx/client.h>
#include <ipcgfx/server.h>
#include <loc.h>
#include <pcut/pcut.h>
#include <stdint.h>

PCUT_INIT;

PCUT_TEST_SUITE(ipcgfx);

static const char *test_ipcgfx_server = "test-ipcgfx";
static const char *test_ipcgfx_svc = "test/ipcgfx";

static void test_ipcgc_conn(ipc_call_t *, void *);

static errno_t test_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t test_gc_set_color(void *, gfx_color_t *);
static errno_t test_gc_fill_rect(void *, gfx_rect_t *);
static errno_t test_gc_update(void *);
static errno_t test_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t test_gc_bitmap_destroy(void *);
static errno_t test_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t test_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t test_gc_ops = {
	.set_clip_rect = test_gc_set_clip_rect,
	.set_color = test_gc_set_color,
	.fill_rect = test_gc_fill_rect,
	.update = test_gc_update,
	.bitmap_create = test_gc_bitmap_create,
	.bitmap_destroy = test_gc_bitmap_destroy,
	.bitmap_render = test_gc_bitmap_render,
	.bitmap_get_alloc = test_gc_bitmap_get_alloc
};

/** Describes to the server how to respond to our request and pass tracking
 * data back to the client.
 */
typedef struct {
	errno_t rc;

	bool set_clip_rect_called;
	bool do_clip;
	gfx_rect_t set_clip_rect_rect;

	bool set_color_called;
	uint16_t set_color_r;
	uint16_t set_color_g;
	uint16_t set_color_b;

	bool fill_rect_called;
	gfx_rect_t fill_rect_rect;

	bool update_called;

	bool bitmap_create_called;
	gfx_bitmap_params_t bitmap_create_params;
	gfx_bitmap_alloc_t bitmap_create_alloc;

	bool bitmap_destroy_called;

	bool bitmap_render_called;
	gfx_rect_t bitmap_render_srect;
	gfx_coord2_t bitmap_render_offs;

	bool bitmap_get_alloc_called;
} test_response_t;

/** Bitmap in test GC */
typedef struct {
	test_response_t *resp;
	gfx_bitmap_alloc_t alloc;
} test_bitmap_t;

/** gfx_set_clip_rect with server returning failure */
PCUT_TEST(set_clip_rect_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_rect_t rect;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = ENOMEM;
	resp.set_clip_rect_called = false;
	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.set_clip_rect_called);
	PCUT_ASSERT_EQUALS(rect.p0.x, resp.set_clip_rect_rect.p0.x);
	PCUT_ASSERT_EQUALS(rect.p0.y, resp.set_clip_rect_rect.p0.y);
	PCUT_ASSERT_EQUALS(rect.p1.x, resp.set_clip_rect_rect.p1.x);
	PCUT_ASSERT_EQUALS(rect.p1.y, resp.set_clip_rect_rect.p1.y);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_set_clip_rect with server returning success */
PCUT_TEST(set_clip_rect_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_rect_t rect;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.set_clip_rect_called = false;
	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.set_clip_rect_called);
	PCUT_ASSERT_TRUE(resp.do_clip);
	PCUT_ASSERT_EQUALS(rect.p0.x, resp.set_clip_rect_rect.p0.x);
	PCUT_ASSERT_EQUALS(rect.p0.y, resp.set_clip_rect_rect.p0.y);
	PCUT_ASSERT_EQUALS(rect.p1.x, resp.set_clip_rect_rect.p1.x);
	PCUT_ASSERT_EQUALS(rect.p1.y, resp.set_clip_rect_rect.p1.y);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_set_clip_rect with null rectangle, server returning success */
PCUT_TEST(set_clip_rect_null_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.set_clip_rect_called = false;

	rc = gfx_set_clip_rect(gc, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.set_clip_rect_called);
	PCUT_ASSERT_FALSE(resp.do_clip);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_set_color with server returning failure */
PCUT_TEST(set_color_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_color_t *color;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.rc = ENOMEM;
	resp.set_color_called = false;
	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.set_color_called);
	PCUT_ASSERT_EQUALS(1, resp.set_color_r);
	PCUT_ASSERT_EQUALS(2, resp.set_color_g);
	PCUT_ASSERT_EQUALS(3, resp.set_color_b);

	gfx_color_delete(color);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_set_color with server returning success */
PCUT_TEST(set_color_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_color_t *color;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.rc = EOK;
	resp.set_color_called = false;
	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.set_color_called);
	PCUT_ASSERT_EQUALS(1, resp.set_color_r);
	PCUT_ASSERT_EQUALS(2, resp.set_color_g);
	PCUT_ASSERT_EQUALS(3, resp.set_color_b);

	gfx_color_delete(color);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_fill_rect with server returning failure */
PCUT_TEST(fill_rect_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_rect_t rect;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = ENOMEM;
	resp.fill_rect_called = false;
	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.fill_rect_called);
	PCUT_ASSERT_EQUALS(rect.p0.x, resp.fill_rect_rect.p0.x);
	PCUT_ASSERT_EQUALS(rect.p0.y, resp.fill_rect_rect.p0.y);
	PCUT_ASSERT_EQUALS(rect.p1.x, resp.fill_rect_rect.p1.x);
	PCUT_ASSERT_EQUALS(rect.p1.y, resp.fill_rect_rect.p1.y);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_fill_rect with server returning success */
PCUT_TEST(fill_rect_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_rect_t rect;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.fill_rect_called = false;
	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;
	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.fill_rect_called);
	PCUT_ASSERT_EQUALS(rect.p0.x, resp.fill_rect_rect.p0.x);
	PCUT_ASSERT_EQUALS(rect.p0.y, resp.fill_rect_rect.p0.y);
	PCUT_ASSERT_EQUALS(rect.p1.x, resp.fill_rect_rect.p1.x);
	PCUT_ASSERT_EQUALS(rect.p1.y, resp.fill_rect_rect.p1.y);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_update with server returning failure */
PCUT_TEST(update_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = ENOMEM;
	resp.update_called = false;
	rc = gfx_update(gc);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.update_called);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_update with server returning success */
PCUT_TEST(update_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.update_called = false;
	rc = gfx_update(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.update_called);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_create with server returning failure */
PCUT_TEST(bitmap_create_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = ENOMEM;
	resp.bitmap_create_called = false;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	bitmap = NULL;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.bitmap_create_params.rect.p1.y);
	PCUT_ASSERT_EQUALS((params.rect.p1.x - params.rect.p0.x) *
	    sizeof(uint32_t), (unsigned) resp.bitmap_create_alloc.pitch);
	PCUT_ASSERT_EQUALS(0, resp.bitmap_create_alloc.off0);
	PCUT_ASSERT_NOT_NULL(resp.bitmap_create_alloc.pixels);
	PCUT_ASSERT_NULL(bitmap);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_create and gfx_bitmap_destroy with server returning success */
PCUT_TEST(bitmap_create_destroy_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.bitmap_create_called = false;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	bitmap = NULL;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.bitmap_create_params.rect.p1.y);
	PCUT_ASSERT_EQUALS((params.rect.p1.x - params.rect.p0.x) *
	    sizeof(uint32_t), (unsigned) resp.bitmap_create_alloc.pitch);
	PCUT_ASSERT_EQUALS(0, resp.bitmap_create_alloc.off0);
	PCUT_ASSERT_NOT_NULL(resp.bitmap_create_alloc.pixels);
	PCUT_ASSERT_NOT_NULL(bitmap);

	resp.bitmap_destroy_called = false;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_destroy_called);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_destroy with server returning failure */
PCUT_TEST(bitmap_destroy_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bitmap);

	resp.rc = EIO;
	resp.bitmap_destroy_called = false;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_destroy_called);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_create direct output bitmap with server returning failure */
PCUT_TEST(bitmap_create_dout_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = ENOMEM;
	resp.bitmap_create_called = false;

	gfx_bitmap_params_init(&params);
	params.flags = bmpf_direct_output;
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	bitmap = NULL;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.bitmap_create_params.rect.p1.y);
	PCUT_ASSERT_EQUALS((params.rect.p1.x - params.rect.p0.x) *
	    sizeof(uint32_t), (unsigned) resp.bitmap_create_alloc.pitch);
	PCUT_ASSERT_EQUALS(0, resp.bitmap_create_alloc.off0);
	PCUT_ASSERT_NOT_NULL(resp.bitmap_create_alloc.pixels);
	PCUT_ASSERT_NULL(bitmap);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_create direct output bitmap with server returning success */
PCUT_TEST(bitmap_create_dout_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	resp.bitmap_create_called = false;

	gfx_bitmap_params_init(&params);
	params.flags = bmpf_direct_output;
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	bitmap = NULL;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_create_called);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, resp.bitmap_create_params.rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, resp.bitmap_create_params.rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, resp.bitmap_create_params.rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, resp.bitmap_create_params.rect.p1.y);
	PCUT_ASSERT_EQUALS((params.rect.p1.x - params.rect.p0.x) *
	    sizeof(uint32_t), (unsigned) resp.bitmap_create_alloc.pitch);
	PCUT_ASSERT_EQUALS(0, resp.bitmap_create_alloc.off0);
	PCUT_ASSERT_NOT_NULL(resp.bitmap_create_alloc.pixels);
	PCUT_ASSERT_NOT_NULL(bitmap);

	resp.bitmap_destroy_called = false;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_destroy_called);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_render with server returning failure */
PCUT_TEST(bitmap_render_failure)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_rect_t srect;
	gfx_coord2_t offs;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bitmap);

	resp.rc = EIO;
	srect.p0.x = 1;
	srect.p0.y = 2;
	srect.p1.x = 3;
	srect.p1.y = 4;
	rc = gfx_bitmap_render(bitmap, &srect, &offs);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_render_called);
	PCUT_ASSERT_EQUALS(srect.p0.x, resp.bitmap_render_srect.p0.x);
	PCUT_ASSERT_EQUALS(srect.p0.y, resp.bitmap_render_srect.p0.y);
	PCUT_ASSERT_EQUALS(srect.p1.x, resp.bitmap_render_srect.p1.x);
	PCUT_ASSERT_EQUALS(srect.p1.y, resp.bitmap_render_srect.p1.y);
	PCUT_ASSERT_EQUALS(offs.x, resp.bitmap_render_offs.x);
	PCUT_ASSERT_EQUALS(offs.y, resp.bitmap_render_offs.y);

	resp.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_render with server returning success */
PCUT_TEST(bitmap_render_success)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_rect_t srect;
	gfx_coord2_t offs;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bitmap);

	resp.rc = EOK;
	srect.p0.x = 1;
	srect.p0.y = 2;
	srect.p1.x = 3;
	srect.p1.y = 4;
	rc = gfx_bitmap_render(bitmap, &srect, &offs);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(resp.bitmap_render_called);
	PCUT_ASSERT_EQUALS(srect.p0.x, resp.bitmap_render_srect.p0.x);
	PCUT_ASSERT_EQUALS(srect.p0.y, resp.bitmap_render_srect.p0.y);
	PCUT_ASSERT_EQUALS(srect.p1.x, resp.bitmap_render_srect.p1.x);
	PCUT_ASSERT_EQUALS(srect.p1.y, resp.bitmap_render_srect.p1.y);
	PCUT_ASSERT_EQUALS(offs.x, resp.bitmap_render_offs.x);
	PCUT_ASSERT_EQUALS(offs.y, resp.bitmap_render_offs.y);

	resp.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** gfx_bitmap_get_alloc - server is not currently involved */
PCUT_TEST(bitmap_get_alloc)
{
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	async_sess_t *sess;
	ipc_gc_t *ipcgc;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_ipcgc_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_ipcgfx_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_ipcgfx_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	sess = loc_service_connect(sid, INTERFACE_GC, 0);
	PCUT_ASSERT_NOT_NULL(sess);

	rc = ipc_gc_create(sess, &ipcgc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gc = ipc_gc_get_ctx(ipcgc);
	PCUT_ASSERT_NOT_NULL(gc);

	resp.rc = EOK;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bitmap);

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_EQUALS((params.rect.p1.x - params.rect.p0.x) *
	    sizeof(uint32_t), (unsigned) alloc.pitch);
	PCUT_ASSERT_EQUALS(0, alloc.off0);
	PCUT_ASSERT_NOT_NULL(alloc.pixels);

	resp.rc = EOK;
	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ipc_gc_delete(ipcgc);
	async_hangup(sess);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

static void test_ipcgc_conn(ipc_call_t *icall, void *arg)
{
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_gc_ops, arg, &gc);
	if (rc != EOK) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	/* Window GC connection */
	gc_conn(icall, gc);
}

/** Set clipping rectangle in test GC.
 *
 * @param arg Test GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t test_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->set_clip_rect_called = true;
	if (rect != NULL) {
		resp->do_clip = true;
		resp->set_clip_rect_rect = *rect;
	} else {
		resp->do_clip = false;
	}

	return resp->rc;
}

/** Set color in test GC.
 *
 * Set drawing color in test GC.
 *
 * @param arg Test GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t test_gc_set_color(void *arg, gfx_color_t *color)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->set_color_called = true;
	gfx_color_get_rgb_i16(color, &resp->set_color_r, &resp->set_color_g,
	    &resp->set_color_b);
	return resp->rc;
}

/** Fill rectangle in test GC.
 *
 * @param arg Test GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t test_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->fill_rect_called = true;
	resp->fill_rect_rect = *rect;
	return resp->rc;
}

/** Update test GC.
 *
 * @param arg Test GC
 *
 * @return EOK on success or an error code
 */
static errno_t test_gc_update(void *arg)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->update_called = true;
	return resp->rc;
}

/** Create bitmap in test GC.
 *
 * @param arg Test GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t test_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	test_response_t *resp = (test_response_t *) arg;
	test_bitmap_t *bitmap;
	gfx_coord2_t dim;

	resp->bitmap_create_called = true;
	resp->bitmap_create_params = *params;

	if ((params->flags & bmpf_direct_output) != 0) {
		gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);

		resp->bitmap_create_alloc.pitch = dim.x * sizeof(uint32_t);
		resp->bitmap_create_alloc.off0 = 0;
		resp->bitmap_create_alloc.pixels = as_area_create(AS_AREA_ANY,
		    dim.x * dim.y * sizeof(uint32_t), AS_AREA_READ |
		    AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
		if (resp->bitmap_create_alloc.pixels == AS_MAP_FAILED)
			return ENOMEM;
	} else {
		resp->bitmap_create_alloc = *alloc;
	}

	if (resp->rc != EOK)
		return resp->rc;

	bitmap = calloc(1, sizeof(test_bitmap_t));
	if (bitmap == NULL)
		return ENOMEM;

	bitmap->resp = resp;
	bitmap->alloc = resp->bitmap_create_alloc;
	*rbm = (void *) bitmap;
	return EOK;
}

/** Destroy bitmap in test GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t test_gc_bitmap_destroy(void *bm)
{
	test_bitmap_t *bitmap = (test_bitmap_t *) bm;
	test_response_t *resp = bitmap->resp;

	resp->bitmap_destroy_called = true;
	if (resp->rc != EOK)
		return resp->rc;

	if ((resp->bitmap_create_params.flags & bmpf_direct_output) != 0)
		as_area_destroy(resp->bitmap_create_alloc.pixels);

	free(bitmap);
	return EOK;
}

/** Render bitmap in test GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t test_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	test_bitmap_t *bitmap = (test_bitmap_t *) bm;
	test_response_t *resp = bitmap->resp;

	resp->bitmap_render_called = true;
	resp->bitmap_render_srect = *srect0;
	resp->bitmap_render_offs = *offs0;
	return resp->rc;
}

/** Get allocation info for bitmap in test GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t test_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	test_bitmap_t *bitmap = (test_bitmap_t *) bm;

	*alloc = bitmap->alloc;
	return EOK;
}

PCUT_EXPORT(ipcgfx);
