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
 * @brief Display configuration protocol server stub
 */

#include <dispcfg.h>
#include <dispcfg_srv.h>
#include <errno.h>
#include <ipc/dispcfg.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include "../private/dispcfg.h"

static void dispcfg_callback_create_srv(dispcfg_srv_t *srv, ipc_call_t *call)
{
	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	srv->client_sess = sess;
	async_answer_0(call, EOK);
}

static void dispcfg_get_seat_list_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	ipc_call_t call;
	dispcfg_seat_list_t *list = NULL;
	size_t size;
	errno_t rc;

	if (srv->ops->get_seat_list == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_seat_list(srv->arg, &list);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Send list size */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(list->nseats)) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &list->nseats, size);
	if (rc != EOK) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Send seat list */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != list->nseats * sizeof(sysarg_t)) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, list->seats, size);
	if (rc != EOK) {
		dispcfg_free_seat_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
	dispcfg_free_seat_list(list);
}

static void dispcfg_get_seat_info_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t seat_id;
	ipc_call_t call;
	dispcfg_seat_info_t *info = NULL;
	size_t namesize;
	size_t size;
	errno_t rc;

	seat_id = ipc_get_arg1(icall);

	if (srv->ops->get_seat_info == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_seat_info(srv->arg, seat_id, &info);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Send name size */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(size_t)) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	namesize = str_size(info->name);

	rc = async_data_read_finalize(&call, &namesize, size);
	if (rc != EOK) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Send name */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != namesize) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, info->name, size);
	if (rc != EOK) {
		dispcfg_free_seat_info(info);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
	dispcfg_free_seat_info(info);
}

static void dispcfg_seat_create_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t seat_id;
	ipc_call_t call;
	char *name;
	size_t size;
	errno_t rc;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	name = calloc(size + 1, 1);
	if (name == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = async_data_write_finalize(&call, name, size);
	if (rc != EOK) {
		free(name);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (srv->ops->seat_create == NULL) {
		free(name);
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->seat_create(srv->arg, name, &seat_id);
	async_answer_1(icall, rc, seat_id);
	free(name);
}

static void dispcfg_seat_delete_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t seat_id;
	errno_t rc;

	seat_id = ipc_get_arg1(icall);

	if (srv->ops->seat_delete == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->seat_delete(srv->arg, seat_id);
	async_answer_0(icall, rc);
}

static void dispcfg_dev_assign_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t svc_id;
	sysarg_t seat_id;
	errno_t rc;

	svc_id = ipc_get_arg1(icall);
	seat_id = ipc_get_arg2(icall);

	if (srv->ops->dev_assign == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->dev_assign(srv->arg, svc_id, seat_id);
	async_answer_0(icall, rc);
}

static void dispcfg_dev_unassign_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t svc_id;
	errno_t rc;

	svc_id = ipc_get_arg1(icall);

	if (srv->ops->dev_unassign == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->dev_unassign(srv->arg, svc_id);
	async_answer_0(icall, rc);
}

static void dispcfg_get_asgn_dev_list_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t seat_id;
	ipc_call_t call;
	dispcfg_dev_list_t *list = NULL;
	size_t size;
	errno_t rc;

	seat_id = ipc_get_arg1(icall);

	if (srv->ops->get_asgn_dev_list == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_asgn_dev_list(srv->arg, seat_id, &list);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Send list size */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(list->ndevs)) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &list->ndevs, size);
	if (rc != EOK) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Send device list */

	if (!async_data_read_receive(&call, &size)) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != list->ndevs * sizeof(sysarg_t)) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, list->devs, size);
	if (rc != EOK) {
		dispcfg_free_dev_list(list);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
	dispcfg_free_dev_list(list);
}

static void dispcfg_get_event_srv(dispcfg_srv_t *srv, ipc_call_t *icall)
{
	dispcfg_ev_t event;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	if (srv->ops->get_event == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_event(srv->arg, &event);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	/* Transfer event data */
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(event)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = async_data_read_finalize(&call, &event, sizeof(event));
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

void dispcfg_conn(ipc_call_t *icall, dispcfg_srv_t *srv)
{
	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;

		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case DISPCFG_CALLBACK_CREATE:
			dispcfg_callback_create_srv(srv, &call);
			break;
		case DISPCFG_GET_SEAT_LIST:
			dispcfg_get_seat_list_srv(srv, &call);
			break;
		case DISPCFG_GET_SEAT_INFO:
			dispcfg_get_seat_info_srv(srv, &call);
			break;
		case DISPCFG_SEAT_CREATE:
			dispcfg_seat_create_srv(srv, &call);
			break;
		case DISPCFG_SEAT_DELETE:
			dispcfg_seat_delete_srv(srv, &call);
			break;
		case DISPCFG_DEV_ASSIGN:
			dispcfg_dev_assign_srv(srv, &call);
			break;
		case DISPCFG_DEV_UNASSIGN:
			dispcfg_dev_unassign_srv(srv, &call);
			break;
		case DISPCFG_GET_ASGN_DEV_LIST:
			dispcfg_get_asgn_dev_list_srv(srv, &call);
			break;
		case DISPCFG_GET_EVENT:
			dispcfg_get_event_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}

	/* Hang up callback session */
	if (srv->client_sess != NULL) {
		async_hangup(srv->client_sess);
		srv->client_sess = NULL;
	}
}

/** Initialize display configuration server structure
 *
 * @param srv Display configuration server structure to initialize
 */
void dispcfg_srv_initialize(dispcfg_srv_t *srv)
{
	memset(srv, 0, sizeof(*srv));
}

/** Send 'pending' event to client.
 *
 * @param srv Display configuration server structure
 */
void dispcfg_srv_ev_pending(dispcfg_srv_t *srv)
{
	async_exch_t *exch;

	exch = async_exchange_begin(srv->client_sess);
	async_msg_0(exch, DISPCFG_EV_PENDING);
	async_exchange_end(exch);
}

/** @}
 */
