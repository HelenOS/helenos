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
#include <dispcfg.h>
#include <dispcfg_srv.h>
#include <fibril_synch.h>
#include <loc.h>
#include <pcut/pcut.h>
#include <str.h>
#include <testdc.h>
#include "../private/dispcfg.h"

PCUT_INIT;

PCUT_TEST_SUITE(dispcfg);

static const char *test_dispcfg_server = "test-dispcfg";
static const char *test_dispcfg_svc = "test/dispcfg";

/** dispcfg_open(), dispcfg_close() work for valid seat management service */
PCUT_TEST(open_close)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_seat_list() with server returning error response works */
PCUT_TEST(get_seat_list_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	dispcfg_seat_list_t *list;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = ENOMEM;
	resp.get_seat_list_called = false;

	rc = dispcfg_get_seat_list(dispcfg, &list);
	PCUT_ASSERT_TRUE(resp.get_seat_list_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_seat_list() with server returning success response works */
PCUT_TEST(get_seat_list_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	dispcfg_seat_list_t *list;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = EOK;
	resp.get_seat_list_called = false;
	resp.get_seat_list_rlist = calloc(1, sizeof(dispcfg_seat_list_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_list_rlist);
	resp.get_seat_list_rlist->nseats = 2;
	resp.get_seat_list_rlist->seats = calloc(2, sizeof(sysarg_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_list_rlist->seats);
	resp.get_seat_list_rlist->seats[0] = 42;
	resp.get_seat_list_rlist->seats[1] = 43;

	rc = dispcfg_get_seat_list(dispcfg, &list);
	PCUT_ASSERT_TRUE(resp.get_seat_list_called);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_INT_EQUALS(2, list->nseats);
	PCUT_ASSERT_INT_EQUALS(42, list->seats[0]);
	PCUT_ASSERT_INT_EQUALS(43, list->seats[1]);

	dispcfg_free_seat_list(list);
	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_seat_infp() with server returning error response works */
PCUT_TEST(get_seat_info_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	dispcfg_seat_info_t *info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = ENOMEM;
	resp.get_seat_info_called = false;
	seat_id = 1;

	rc = dispcfg_get_seat_info(dispcfg, seat_id, &info);
	PCUT_ASSERT_TRUE(resp.get_seat_info_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.get_seat_info_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_seat_info() with server returning success response works */
PCUT_TEST(get_seat_info_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	dispcfg_seat_info_t *info;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = EOK;
	resp.get_seat_info_called = false;
	resp.get_seat_info_rinfo = calloc(1, sizeof(dispcfg_seat_info_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_info_rinfo);
	resp.get_seat_info_rinfo->name = str_dup("Hello");
	PCUT_ASSERT_NOT_NULL(resp.get_seat_info_rinfo->name);
	seat_id = 1;

	rc = dispcfg_get_seat_info(dispcfg, seat_id, &info);
	PCUT_ASSERT_TRUE(resp.get_seat_info_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.get_seat_info_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_STR_EQUALS("Hello", info->name);

	dispcfg_free_seat_info(info);
	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_seat_create() with server returning error response works */
PCUT_TEST(seat_create_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	seat_id = 13;
	seat_id = 42;
	resp.rc = ENOMEM;
	resp.seat_create_called = false;

	rc = dispcfg_seat_create(dispcfg, "Alice", &seat_id);
	PCUT_ASSERT_TRUE(resp.seat_create_called);
	PCUT_ASSERT_STR_EQUALS("Alice", resp.seat_create_name);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	free(resp.seat_create_name);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_seat_create() with server returning success response works */
PCUT_TEST(seat_create_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = EOK;
	resp.seat_create_called = false;
	resp.seat_create_seat_id = 42;

	rc = dispcfg_seat_create(dispcfg, "Alice", &seat_id);
	PCUT_ASSERT_TRUE(resp.seat_create_called);
	PCUT_ASSERT_STR_EQUALS("Alice", resp.seat_create_name);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.seat_create_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);
	free(resp.seat_create_name);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_seat_delete() with server returning error response works */
PCUT_TEST(seat_delete_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	seat_id = 42;
	resp.rc = ENOMEM;
	resp.seat_delete_called = false;

	rc = dispcfg_seat_delete(dispcfg, seat_id);
	PCUT_ASSERT_TRUE(resp.seat_delete_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.seat_delete_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_seat_delete() with server returning success response works */
PCUT_TEST(seat_delete_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	seat_id = 42;
	resp.rc = EOK;
	resp.seat_delete_called = false;

	rc = dispcfg_seat_delete(dispcfg, seat_id);
	PCUT_ASSERT_TRUE(resp.seat_delete_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.seat_delete_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_dev_assign() with server returning error response works */
PCUT_TEST(dev_assign_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t svc_id;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	svc_id = 12;
	seat_id = 13;
	resp.rc = ENOMEM;
	resp.dev_assign_called = false;

	rc = dispcfg_dev_assign(dispcfg, svc_id, seat_id);
	PCUT_ASSERT_TRUE(resp.dev_assign_called);
	PCUT_ASSERT_INT_EQUALS(svc_id, resp.dev_assign_svc_id);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.dev_assign_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_dev_assign() with server returning success response works */
PCUT_TEST(dev_assign_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t svc_id;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	svc_id = 12;
	seat_id = 13;
	resp.rc = EOK;
	resp.dev_assign_called = false;

	rc = dispcfg_dev_assign(dispcfg, svc_id, seat_id);
	PCUT_ASSERT_TRUE(resp.dev_assign_called);
	PCUT_ASSERT_INT_EQUALS(svc_id, resp.dev_assign_svc_id);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.dev_assign_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_dev_unassign() with server returning error response works */
PCUT_TEST(dev_unassign_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t svc_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	svc_id = 12;
	resp.rc = ENOMEM;
	resp.dev_unassign_called = false;

	rc = dispcfg_dev_unassign(dispcfg, svc_id);
	PCUT_ASSERT_TRUE(resp.dev_unassign_called);
	PCUT_ASSERT_INT_EQUALS(svc_id, resp.dev_unassign_svc_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_dev_unassign() with server returning success response works */
PCUT_TEST(dev_unassign_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	sysarg_t svc_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	svc_id = 12;
	resp.rc = EOK;
	resp.dev_unassign_called = false;

	rc = dispcfg_dev_unassign(dispcfg, svc_id);
	PCUT_ASSERT_TRUE(resp.dev_unassign_called);
	PCUT_ASSERT_INT_EQUALS(svc_id, resp.dev_unassign_svc_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_asgn_dev_list() with server returning error response works */
PCUT_TEST(get_asgn_dev_list_failure)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	dispcfg_dev_list_t *list;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = ENOMEM;
	resp.get_asgn_dev_list_called = false;
	seat_id = 42;

	rc = dispcfg_get_asgn_dev_list(dispcfg, seat_id, &list);
	PCUT_ASSERT_TRUE(resp.get_asgn_dev_list_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.get_asgn_dev_list_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dispcfg_get_asgn_dev_list() with server returning success response works */
PCUT_TEST(get_asgn_dev_list_success)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	dispcfg_dev_list_t *list;
	sysarg_t seat_id;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	resp.rc = EOK;
	resp.get_asgn_dev_list_called = false;
	resp.get_asgn_dev_list_rlist = calloc(1, sizeof(dispcfg_dev_list_t));
	PCUT_ASSERT_NOT_NULL(resp.get_asgn_dev_list_rlist);
	resp.get_asgn_dev_list_rlist->ndevs = 2;
	resp.get_asgn_dev_list_rlist->devs = calloc(2, sizeof(sysarg_t));
	PCUT_ASSERT_NOT_NULL(resp.get_asgn_dev_list_rlist->devs);
	resp.get_asgn_dev_list_rlist->devs[0] = 11;
	resp.get_asgn_dev_list_rlist->devs[1] = 12;
	seat_id = 42;

	rc = dispcfg_get_asgn_dev_list(dispcfg, seat_id, &list);
	PCUT_ASSERT_TRUE(resp.get_asgn_dev_list_called);
	PCUT_ASSERT_INT_EQUALS(seat_id, resp.get_asgn_dev_list_seat_id);
	PCUT_ASSERT_ERRNO_VAL(resp.rc, rc);

	PCUT_ASSERT_INT_EQUALS(2, list->ndevs);
	PCUT_ASSERT_INT_EQUALS(11, list->devs[0]);
	PCUT_ASSERT_INT_EQUALS(12, list->devs[1]);

	dispcfg_free_dev_list(list);
	dispcfg_close(dispcfg);
	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Window added event can be delivered from server to client callback function */
PCUT_TEST(seat_added_deliver)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, &test_dispcfg_cb, &resp, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.event_cnt = 1;
	resp.event.etype = dcev_seat_added;
	resp.event.seat_id = 42;
	resp.seat_added_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	dispcfg_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.seat_added_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	dispcfg_close(dispcfg);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** Window removed event can be delivered from server to client callback function */
PCUT_TEST(seat_removed_deliver)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, &test_dispcfg_cb, &resp, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);
	PCUT_ASSERT_NOT_NULL(resp.srv);

	resp.event_cnt = 1;
	resp.event.etype = dcev_seat_removed;
	resp.event.seat_id = 42;
	resp.seat_removed_called = false;
	fibril_mutex_initialize(&resp.event_lock);
	fibril_condvar_initialize(&resp.event_cv);
	dispcfg_srv_ev_pending(resp.srv);

	/* Wait for the event handler to be called. */
	fibril_mutex_lock(&resp.event_lock);
	while (!resp.seat_removed_called) {
		fibril_condvar_wait(&resp.event_cv, &resp.event_lock);
	}
	fibril_mutex_unlock(&resp.event_lock);

	/* Verify that the event was delivered correctly */
	PCUT_ASSERT_INT_EQUALS(resp.event.etype,
	    resp.revent.etype);

	dispcfg_close(dispcfg);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

PCUT_EXPORT(dispcfg);
