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

/** @addtogroup libdispcfg
 * @{
 */
/**
 * @file
 * @brief Display configuration test service
 */

#include <errno.h>
#include <dispcfg.h>
#include <dispcfg_srv.h>
#include <fibril_synch.h>
#include <str.h>
#include <testdc.h>

static errno_t test_get_seat_list(void *, dispcfg_seat_list_t **);
static errno_t test_get_seat_info(void *, sysarg_t, dispcfg_seat_info_t **);
static errno_t test_seat_create(void *, const char *, sysarg_t *);
static errno_t test_seat_delete(void *, sysarg_t);
static errno_t test_dev_assign(void *, sysarg_t, sysarg_t);
static errno_t test_dev_unassign(void *, sysarg_t);
static errno_t test_get_asgn_dev_list(void *, sysarg_t, dispcfg_dev_list_t **);
static errno_t test_get_event(void *, dispcfg_ev_t *);

static void test_seat_added(void *, sysarg_t);
static void test_seat_removed(void *, sysarg_t);

static dispcfg_ops_t test_dispcfg_srv_ops = {
	.get_seat_list = test_get_seat_list,
	.get_seat_info = test_get_seat_info,
	.seat_create = test_seat_create,
	.seat_delete = test_seat_delete,
	.dev_assign = test_dev_assign,
	.dev_unassign = test_dev_unassign,
	.get_asgn_dev_list = test_get_asgn_dev_list,
	.get_event = test_get_event
};

dispcfg_cb_t test_dispcfg_cb = {
	.seat_added = test_seat_added,
	.seat_removed = test_seat_removed
};

/** Test seat management service connection. */
void test_dispcfg_conn(ipc_call_t *icall, void *arg)
{
	test_response_t *resp = (test_response_t *) arg;
	dispcfg_srv_t srv;

	/* Set up protocol structure */
	dispcfg_srv_initialize(&srv);
	srv.ops = &test_dispcfg_srv_ops;
	srv.arg = arg;
	resp->srv = &srv;

	/* Handle connection */
	dispcfg_conn(icall, &srv);

	resp->srv = NULL;
}

static void test_seat_added(void *arg, sysarg_t seat_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = dcev_seat_added;

	fibril_mutex_lock(&resp->event_lock);
	resp->seat_added_called = true;
	resp->seat_added_seat_id = seat_id;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static void test_seat_removed(void *arg, sysarg_t seat_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->revent.etype = dcev_seat_removed;

	fibril_mutex_lock(&resp->event_lock);
	resp->seat_removed_called = true;
	resp->seat_removed_seat_id = seat_id;
	fibril_condvar_broadcast(&resp->event_cv);
	fibril_mutex_unlock(&resp->event_lock);
}

static errno_t test_get_seat_list(void *arg, dispcfg_seat_list_t **rlist)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_seat_list_called = true;

	if (resp->rc != EOK)
		return resp->rc;

	*rlist = resp->get_seat_list_rlist;
	return EOK;
}

static errno_t test_get_seat_info(void *arg, sysarg_t seat_id,
    dispcfg_seat_info_t **rinfo)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_seat_info_called = true;
	resp->get_seat_info_seat_id = seat_id;

	if (resp->rc != EOK)
		return resp->rc;

	*rinfo = resp->get_seat_info_rinfo;
	return EOK;
}

static errno_t test_seat_create(void *arg, const char *name, sysarg_t *rseat_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->seat_create_called = true;
	resp->seat_create_name = str_dup(name);
	*rseat_id = resp->seat_create_seat_id;
	return resp->rc;
}

static errno_t test_seat_delete(void *arg, sysarg_t seat_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->seat_delete_called = true;
	resp->seat_delete_seat_id = seat_id;
	return resp->rc;
}

static errno_t test_dev_assign(void *arg, sysarg_t svc_id, sysarg_t seat_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->dev_assign_called = true;
	resp->dev_assign_svc_id = svc_id;
	resp->dev_assign_seat_id = seat_id;
	return resp->rc;
}

static errno_t test_dev_unassign(void *arg, sysarg_t svc_id)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->dev_unassign_called = true;
	resp->dev_unassign_svc_id = svc_id;
	return resp->rc;
}

static errno_t test_get_asgn_dev_list(void *arg, sysarg_t seat_id,
    dispcfg_dev_list_t **rlist)
{
	test_response_t *resp = (test_response_t *) arg;

	resp->get_asgn_dev_list_called = true;
	resp->get_asgn_dev_list_seat_id = seat_id;

	if (resp->rc != EOK)
		return resp->rc;

	*rlist = resp->get_asgn_dev_list_rlist;
	return EOK;
}

static errno_t test_get_event(void *arg, dispcfg_ev_t *event)
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

/** @}
 */
