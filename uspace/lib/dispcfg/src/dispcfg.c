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
 * @brief Display configuration protocol client
 */

#include <async.h>
#include <dispcfg.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/dispcfg.h>
#include <ipc/services.h>
#include <loc.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include "../private/dispcfg.h"

static errno_t dispcfg_callback_create(dispcfg_t *);
static void dispcfg_cb_conn(ipc_call_t *, void *);

/** Open display configuration service.
 *
 * @param wmname Display configuration service name or @c NULL to use default
 * @param cb Display configuration callbacks
 * @param cb_arg Callback argument
 * @param rdispcfg Place to store pointer to display configuration session
 * @return EOK on success or an error code
 */
errno_t dispcfg_open(const char *wmname, dispcfg_cb_t *cb, void *cb_arg,
    dispcfg_t **rdispcfg)
{
	service_id_t dispcfg_svc;
	dispcfg_t *dispcfg;
	errno_t rc;

	dispcfg = calloc(1, sizeof(dispcfg_t));
	if (dispcfg == NULL)
		return ENOMEM;

	dispcfg->cb = cb;
	dispcfg->cb_arg = cb_arg;

	fibril_mutex_initialize(&dispcfg->lock);
	fibril_condvar_initialize(&dispcfg->cv);

	if (wmname == NULL)
		wmname = SERVICE_NAME_DISPCFG;

	rc = loc_service_get_id(wmname, &dispcfg_svc, 0);
	if (rc != EOK) {
		free(dispcfg);
		return ENOENT;
	}

	dispcfg->sess = loc_service_connect(dispcfg_svc, INTERFACE_DISPCFG,
	    0);
	if (dispcfg->sess == NULL) {
		free(dispcfg);
		return ENOENT;
	}

	rc = dispcfg_callback_create(dispcfg);
	if (rc != EOK) {
		async_hangup(dispcfg->sess);
		free(dispcfg);
		return EIO;
	}

	*rdispcfg = dispcfg;
	return EOK;
}

/** Create callback connection from display configuration service.
 *
 * @param dispcfg Display configuration
 * @return EOK on success or an error code
 */
static errno_t dispcfg_callback_create(dispcfg_t *dispcfg)
{
	async_exch_t *exch = async_exchange_begin(dispcfg->sess);

	aid_t req = async_send_0(exch, DISPCFG_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_DISPCFG_CB, 0, 0,
	    dispcfg_cb_conn, dispcfg, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Close display configuration service.
 *
 * @param dispcfg Display configuration
 */
void dispcfg_close(dispcfg_t *dispcfg)
{
	fibril_mutex_lock(&dispcfg->lock);
	async_hangup(dispcfg->sess);
	dispcfg->sess = NULL;

	/* Wait for callback handler to terminate */

	while (!dispcfg->cb_done)
		fibril_condvar_wait(&dispcfg->cv, &dispcfg->lock);
	fibril_mutex_unlock(&dispcfg->lock);

	free(dispcfg);
}

/** Get seat list.
 *
 * @param dispcfg Display configuration
 * @param rlist Place to store pointer to new seat list structure
 * @return EOK on success or an error code
 */
errno_t dispcfg_get_seat_list(dispcfg_t *dispcfg, dispcfg_seat_list_t **rlist)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	dispcfg_seat_list_t *list;
	sysarg_t nseats;
	sysarg_t *seats;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	req = async_send_0(exch, DISPCFG_GET_SEAT_LIST, &answer);

	/* Receive seat list length */
	rc = async_data_read_start(exch, &nseats, sizeof (nseats));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(req, &rc);
		return rc;
	}

	seats = calloc(nseats, sizeof(sysarg_t));
	if (seats == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	/* Receive seat list */
	rc = async_data_read_start(exch, seats, nseats * sizeof (sysarg_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	list = calloc(1, sizeof(dispcfg_seat_list_t));
	if (list == NULL)
		return ENOMEM;

	list->nseats = nseats;
	list->seats = seats;
	*rlist = list;
	return EOK;
}

/** Free seat list.
 *
 * @param list Seat list
 */
void dispcfg_free_seat_list(dispcfg_seat_list_t *list)
{
	free(list->seats);
	free(list);
}

/** Get seat information.
 *
 * @param dispcfg Display configuration
 * @param seat_id Seat ID
 * @param rinfo Place to store pointer to new seat information structure
 * @return EOK on success or an error code
 */
errno_t dispcfg_get_seat_info(dispcfg_t *dispcfg, sysarg_t seat_id,
    dispcfg_seat_info_t **rinfo)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	dispcfg_seat_info_t *info;
	sysarg_t namesize;
	char *name;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	req = async_send_1(exch, DISPCFG_GET_SEAT_INFO, seat_id, &answer);

	/* Receive name size */
	rc = async_data_read_start(exch, &namesize, sizeof (namesize));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(req, &rc);
		return rc;
	}

	name = calloc(namesize + 1, sizeof(char));
	if (name == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	/* Receive name */
	rc = async_data_read_start(exch, name, namesize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	/* Null-terminate the string */
	name[namesize] = '\0';

	info = calloc(1, sizeof(dispcfg_seat_info_t));
	if (info == NULL)
		return ENOMEM;

	info->name = name;
	*rinfo = info;
	return EOK;
}

/** Free seat information.
 *
 * @param info Seat information
 */
void dispcfg_free_seat_info(dispcfg_seat_info_t *info)
{
	free(info->name);
	free(info);
}

/** Create seat.
 *
 * @param dispcfg Display configuration session
 * @param name Seat name
 * @param rseat_id Place to store ID of the new seat
 * @return EOK on success or an error code
 */
errno_t dispcfg_seat_create(dispcfg_t *dispcfg, const char *name,
    sysarg_t *rseat_id)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	size_t name_size;
	errno_t rc;

	name_size = str_size(name);

	exch = async_exchange_begin(dispcfg->sess);
	req = async_send_0(exch, DISPCFG_SEAT_CREATE, &answer);

	/* Write name */
	rc = async_data_write_start(exch, name, name_size);
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	*rseat_id = ipc_get_arg1(&answer);
	return EOK;
}

/** Delete seat.
 *
 * @param dispcfg Display configuration
 * @param seat_id Seat ID
 * @return EOK on success or an error code
 */
errno_t dispcfg_seat_delete(dispcfg_t *dispcfg, sysarg_t seat_id)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	rc = async_req_1_0(exch, DISPCFG_SEAT_DELETE, seat_id);

	async_exchange_end(exch);
	return rc;
}

/** Assign device to seat.
 *
 * @param dispcfg Display configuration
 * @param svc_id Device service ID
 * @param seat_id Seat ID
 * @return EOK on success or an error code
 */
errno_t dispcfg_dev_assign(dispcfg_t *dispcfg, sysarg_t svc_id,
    sysarg_t seat_id)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	rc = async_req_2_0(exch, DISPCFG_DEV_ASSIGN, svc_id, seat_id);

	async_exchange_end(exch);
	return rc;
}

/** Unassign device from any specific seat.
 *
 * The device will fall back to the default seat.
 *
 * @param dispcfg Display configuration
 * @param svc_id Device service ID
 * @return EOK on success or an error code
 */
errno_t dispcfg_dev_unassign(dispcfg_t *dispcfg, sysarg_t svc_id)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	rc = async_req_1_0(exch, DISPCFG_DEV_UNASSIGN, svc_id);

	async_exchange_end(exch);
	return rc;
}

/** Get list of devices assigned to a seat.
 *
 * @param dispcfg Display configuration
 * @param seat_id Seat ID
 * @param rlist Place to store pointer to new device list structure
 * @return EOK on success or an error code
 */
errno_t dispcfg_get_asgn_dev_list(dispcfg_t *dispcfg, sysarg_t seat_id,
    dispcfg_dev_list_t **rlist)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	dispcfg_dev_list_t *list;
	sysarg_t ndevs;
	sysarg_t *devs;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	req = async_send_1(exch, DISPCFG_GET_ASGN_DEV_LIST, seat_id, &answer);

	/* Receive device list length */
	rc = async_data_read_start(exch, &ndevs, sizeof (ndevs));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(req, &rc);
		return rc;
	}

	devs = calloc(ndevs, sizeof(sysarg_t));
	if (devs == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	/* Receive device list */
	rc = async_data_read_start(exch, devs, ndevs * sizeof (sysarg_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	list = calloc(1, sizeof(dispcfg_dev_list_t));
	if (list == NULL)
		return ENOMEM;

	list->ndevs = ndevs;
	list->devs = devs;
	*rlist = list;
	return EOK;
}

/** Free device list.
 *
 * @param list Device list
 */
void dispcfg_free_dev_list(dispcfg_dev_list_t *list)
{
	free(list->devs);
	free(list);
}

/** Get display configuration event.
 *
 * @param dispcfg Display configuration
 * @param event Place to store event
 * @return EOK on success or an error code
 */
static errno_t dispcfg_get_event(dispcfg_t *dispcfg, dispcfg_ev_t *event)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	exch = async_exchange_begin(dispcfg->sess);
	req = async_send_0(exch, DISPCFG_GET_EVENT, &answer);
	rc = async_data_read_start(exch, event, sizeof(*event));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Display configuration events are pending.
 *
 * @param dispcfg Display configuration
 * @param icall Call data
 */
static void dispcfg_ev_pending(dispcfg_t *dispcfg, ipc_call_t *icall)
{
	errno_t rc;
	dispcfg_ev_t event;

	while (true) {
		fibril_mutex_lock(&dispcfg->lock);

		if (dispcfg->sess != NULL)
			rc = dispcfg_get_event(dispcfg, &event);
		else
			rc = ENOENT;

		fibril_mutex_unlock(&dispcfg->lock);

		if (rc != EOK)
			break;

		switch (event.etype) {
		case dcev_seat_added:
			if (dispcfg->cb != NULL &&
			    dispcfg->cb->seat_added != NULL) {
				dispcfg->cb->seat_added(dispcfg->cb_arg,
				    event.seat_id);
			}
			break;
		case dcev_seat_removed:
			if (dispcfg->cb != NULL &&
			    dispcfg->cb->seat_removed != NULL) {
				dispcfg->cb->seat_removed(dispcfg->cb_arg,
				    event.seat_id);
			}
			break;
		}
	}

	async_answer_0(icall, EOK);
}

/** Callback connection handler.
 *
 * @param icall Connect call data
 * @param arg   Argument, dispcfg_t *
 */
static void dispcfg_cb_conn(ipc_call_t *icall, void *arg)
{
	dispcfg_t *dispcfg = (dispcfg_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case DISPCFG_EV_PENDING:
			dispcfg_ev_pending(dispcfg, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

out:
	fibril_mutex_lock(&dispcfg->lock);
	dispcfg->cb_done = true;
	fibril_mutex_unlock(&dispcfg->lock);
	fibril_condvar_broadcast(&dispcfg->cv);
}

/** @}
 */
