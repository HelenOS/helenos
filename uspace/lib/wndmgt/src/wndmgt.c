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

/** @addtogroup libwndmgt
 * @{
 */
/**
 * @file
 * @brief Window management protocol client
 */

#include <async.h>
#include <wndmgt.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/wndmgt.h>
#include <ipc/services.h>
#include <loc.h>
#include <mem.h>
#include <stdlib.h>
#include "../private/wndmgt.h"

static errno_t wndmgt_callback_create(wndmgt_t *);
static void wndmgt_cb_conn(ipc_call_t *, void *);

/** Open window management service.
 *
 * @param wmname Window management service name or @c NULL to use default
 * @param cb Window management callbacks
 * @param cb_arg Callback argument
 * @param rwndmgt Place to store pointer to window management session
 * @return EOK on success or an error code
 */
errno_t wndmgt_open(const char *wmname, wndmgt_cb_t *cb, void *cb_arg,
    wndmgt_t **rwndmgt)
{
	service_id_t wndmgt_svc;
	wndmgt_t *wndmgt;
	errno_t rc;

	wndmgt = calloc(1, sizeof(wndmgt_t));
	if (wndmgt == NULL)
		return ENOMEM;

	wndmgt->cb = cb;
	wndmgt->cb_arg = cb_arg;

	fibril_mutex_initialize(&wndmgt->lock);
	fibril_condvar_initialize(&wndmgt->cv);

	if (wmname == NULL)
		wmname = SERVICE_NAME_WNDMGT;

	rc = loc_service_get_id(wmname, &wndmgt_svc, 0);
	if (rc != EOK) {
		free(wndmgt);
		return ENOENT;
	}

	wndmgt->sess = loc_service_connect(wndmgt_svc, INTERFACE_WNDMGT,
	    0);
	if (wndmgt->sess == NULL) {
		free(wndmgt);
		return ENOENT;
	}

	rc = wndmgt_callback_create(wndmgt);
	if (rc != EOK) {
		async_hangup(wndmgt->sess);
		free(wndmgt);
		return EIO;
	}

	*rwndmgt = wndmgt;
	return EOK;
}

/** Create callback connection from window management service.
 *
 * @param wndmgt Window management
 * @return EOK on success or an error code
 */
static errno_t wndmgt_callback_create(wndmgt_t *wndmgt)
{
	async_exch_t *exch = async_exchange_begin(wndmgt->sess);

	aid_t req = async_send_0(exch, WNDMGT_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_WNDMGT_CB, 0, 0,
	    wndmgt_cb_conn, wndmgt, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Close window management service.
 *
 * @param wndmgt Window management
 */
void wndmgt_close(wndmgt_t *wndmgt)
{
	fibril_mutex_lock(&wndmgt->lock);
	async_hangup(wndmgt->sess);
	wndmgt->sess = NULL;

	/* Wait for callback handler to terminate */

	while (!wndmgt->cb_done)
		fibril_condvar_wait(&wndmgt->cv, &wndmgt->lock);
	fibril_mutex_unlock(&wndmgt->lock);

	free(wndmgt);
}

/** Get window list.
 *
 * @param wndmgt Window management
 * @param rlist Place to store pointer to new window list structure
 * @return EOK on success or an error code
 */
errno_t wndmgt_get_window_list(wndmgt_t *wndmgt, wndmgt_window_list_t **rlist)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	wndmgt_window_list_t *list;
	sysarg_t nwindows;
	sysarg_t *windows;
	errno_t rc;

	exch = async_exchange_begin(wndmgt->sess);
	req = async_send_0(exch, WNDMGT_GET_WINDOW_LIST, &answer);

	/* Receive window list length */
	rc = async_data_read_start(exch, &nwindows, sizeof (nwindows));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(req, &rc);
		return rc;
	}

	windows = calloc(nwindows, sizeof(sysarg_t));
	if (windows == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	/* Receive window list */
	rc = async_data_read_start(exch, windows, nwindows * sizeof (sysarg_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	list = calloc(1, sizeof(wndmgt_window_list_t));
	if (list == NULL)
		return ENOMEM;

	list->nwindows = nwindows;
	list->windows = windows;
	*rlist = list;
	return EOK;
}

/** Free window list.
 *
 * @param list Window management list
 */
void wndmgt_free_window_list(wndmgt_window_list_t *list)
{
	free(list->windows);
	free(list);
}

/** Get window information.
 *
 * @param wndmgt Window management
 * @param wnd_id Window ID
 * @param rinfo Place to store pointer to new window information structure
 * @return EOK on success or an error code
 */
errno_t wndmgt_get_window_info(wndmgt_t *wndmgt, sysarg_t wnd_id,
    wndmgt_window_info_t **rinfo)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	wndmgt_window_info_t *info;
	sysarg_t capsize;
	char *caption;
	errno_t rc;

	exch = async_exchange_begin(wndmgt->sess);
	req = async_send_1(exch, WNDMGT_GET_WINDOW_INFO, wnd_id, &answer);

	/* Receive caption size */
	rc = async_data_read_start(exch, &capsize, sizeof (capsize));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(req, &rc);
		return rc;
	}

	caption = calloc(capsize + 1, sizeof(char));
	if (caption == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	/* Receive caption */
	rc = async_data_read_start(exch, caption, capsize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	/* Null-terminate the string */
	caption[capsize] = '\0';

	info = calloc(1, sizeof(wndmgt_window_info_t));
	if (info == NULL)
		return ENOMEM;

	info->caption = caption;
	info->flags = ipc_get_arg1(&answer);
	info->nfocus = ipc_get_arg2(&answer);
	*rinfo = info;
	return EOK;
}

/** Free window information.
 *
 * @param info Window management information
 */
void wndmgt_free_window_info(wndmgt_window_info_t *info)
{
	free(info->caption);
	free(info);
}

/** Activate window.
 *
 * @param wndmgt Window management session
 * @param dev_id ID of input device belonging to seat whose
 *               focus is to be switched
 * @param wnd_id Window ID
 * @return EOK on success or an error code
 */
errno_t wndmgt_activate_window(wndmgt_t *wndmgt, sysarg_t dev_id,
    sysarg_t wnd_id)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(wndmgt->sess);
	rc = async_req_2_0(exch, WNDMGT_ACTIVATE_WINDOW, dev_id,
	    wnd_id);

	async_exchange_end(exch);
	return rc;
}

/** Close window.
 *
 * @param wndmgt Window management
 * @param wnd_id Window ID
 * @return EOK on success or an error code
 */
errno_t wndmgt_close_window(wndmgt_t *wndmgt, sysarg_t wnd_id)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(wndmgt->sess);
	rc = async_req_1_0(exch, WNDMGT_CLOSE_WINDOW, wnd_id);

	async_exchange_end(exch);
	return rc;
}

/** Get window management event.
 *
 * @param wndmgt Window management
 * @param event Place to store event
 * @return EOK on success or an error code
 */
static errno_t wndmgt_get_event(wndmgt_t *wndmgt, wndmgt_ev_t *event)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	exch = async_exchange_begin(wndmgt->sess);
	req = async_send_0(exch, WNDMGT_GET_EVENT, &answer);
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

/** Window management events are pending.
 *
 * @param wndmgt Window management
 * @param icall Call data
 */
static void wndmgt_ev_pending(wndmgt_t *wndmgt, ipc_call_t *icall)
{
	errno_t rc;
	wndmgt_ev_t event;

	while (true) {
		fibril_mutex_lock(&wndmgt->lock);

		if (wndmgt->sess != NULL)
			rc = wndmgt_get_event(wndmgt, &event);
		else
			rc = ENOENT;

		fibril_mutex_unlock(&wndmgt->lock);

		if (rc != EOK)
			break;

		switch (event.etype) {
		case wmev_window_added:
			if (wndmgt->cb != NULL &&
			    wndmgt->cb->window_added != NULL) {
				wndmgt->cb->window_added(wndmgt->cb_arg,
				    event.wnd_id);
			}
			break;
		case wmev_window_removed:
			if (wndmgt->cb != NULL &&
			    wndmgt->cb->window_removed != NULL) {
				wndmgt->cb->window_removed(wndmgt->cb_arg,
				    event.wnd_id);
			}
			break;
		case wmev_window_changed:
			if (wndmgt->cb != NULL &&
			    wndmgt->cb->window_changed != NULL) {
				wndmgt->cb->window_changed(wndmgt->cb_arg,
				    event.wnd_id);
			}
			break;
		}
	}

	async_answer_0(icall, EOK);
}

/** Callback connection handler.
 *
 * @param icall Connect call data
 * @param arg   Argument, wndmgt_t *
 */
static void wndmgt_cb_conn(ipc_call_t *icall, void *arg)
{
	wndmgt_t *wndmgt = (wndmgt_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case WNDMGT_EV_PENDING:
			wndmgt_ev_pending(wndmgt, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

out:
	fibril_mutex_lock(&wndmgt->lock);
	wndmgt->cb_done = true;
	fibril_mutex_unlock(&wndmgt->lock);
	fibril_condvar_broadcast(&wndmgt->cv);
}

/** @}
 */
