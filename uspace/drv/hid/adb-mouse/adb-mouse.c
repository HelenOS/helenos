/*
 * Copyright (c) 2011 Martin Decky
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

/** @file
 * @brief ADB mouse driver
 */

#include <async.h>
#include <ddf/log.h>
#include <errno.h>
#include <ipc/adb.h>
#include <ipc/mouseev.h>
#include <loc.h>
#include <stdio.h>

#include "adb-mouse.h"

static void adb_mouse_conn(cap_call_handle_t, ipc_call_t *, void *);

static void adb_mouse_event_button(adb_mouse_t *mouse, int bnum, int bpress)
{
	async_exch_t *exch = async_exchange_begin(mouse->client_sess);
	async_msg_2(exch, MOUSEEV_BUTTON_EVENT, bnum, bpress);
	async_exchange_end(exch);
}

static void adb_mouse_event_move(adb_mouse_t *mouse, int dx, int dy, int dz)
{
	async_exch_t *exch = async_exchange_begin(mouse->client_sess);
	async_msg_3(exch, MOUSEEV_MOVE_EVENT, dx, dy, dz);
	async_exchange_end(exch);
}

/** Process mouse data */
static void adb_mouse_data(adb_mouse_t *mouse, sysarg_t data)
{
	bool b1, b2;
	uint16_t udx, udy;
	int dx, dy;

	/* Extract fields. */
	b1 = ((data >> 15) & 1) == 0;
	udy = (data >> 8) & 0x7f;
	b2 = ((data >> 7) & 1) == 0;
	udx = data & 0x7f;

	/* Decode 7-bit two's complement signed values. */
	dx = (udx & 0x40) ? (udx - 0x80) : udx;
	dy = (udy & 0x40) ? (udy - 0x80) : udy;

	if (b1 != mouse->b1_pressed) {
		adb_mouse_event_button(mouse, 1, b1);
		mouse->b1_pressed = b1;
	}

	if (b2 != mouse->b2_pressed) {
		adb_mouse_event_button(mouse, 2, b2);
		mouse->b2_pressed = b2;
	}

	if (dx != 0 || dy != 0)
		adb_mouse_event_move(mouse, dx, dy, 0);
}

static void adb_mouse_events(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	adb_mouse_t *mouse = (adb_mouse_t *) arg;

	/* Ignore parameters, the connection is already opened */
	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);

		errno_t retval = EOK;

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case ADB_REG_NOTIF:
			adb_mouse_data(mouse, IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}

		async_answer_0(chandle, retval);
	}
}

/** Add ADB mouse device */
errno_t adb_mouse_add(adb_mouse_t *mouse)
{
	errno_t rc;
	bool bound = false;

	mouse->fun = ddf_fun_create(mouse->dev, fun_exposed, "a");
	if (mouse->fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function");
		rc = ENOMEM;
		goto error;
	}

	mouse->parent_sess = ddf_dev_parent_sess_get(mouse->dev);
	if (mouse->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Error connecting parent driver");
		rc = EIO;
		goto error;
	}

	async_exch_t *exch = async_exchange_begin(mouse->parent_sess);
	if (exch == NULL) {
		ddf_msg(LVL_ERROR, "Error starting exchange with parent");
		rc = ENOMEM;
		goto error;
	}

	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_ADB_CB, 0, 0,
	    adb_mouse_events, mouse, &port);

	async_exchange_end(exch);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error creating callback from device");
		goto error;
	}

	ddf_fun_set_conn_handler(mouse->fun, adb_mouse_conn);

	rc = ddf_fun_bind(mouse->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function");
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(mouse->fun, "mouse");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding function to category");
		goto error;
	}

	return EOK;
error:
	if (bound)
		ddf_fun_unbind(mouse->fun);

	if (mouse->parent_sess != NULL) {
		async_hangup(mouse->parent_sess);
		mouse->parent_sess = NULL;
	}

	if (mouse->fun != NULL) {
		ddf_fun_destroy(mouse->fun);
		mouse->fun = NULL;
	}

	return rc;
}

/** Remove ADB mouse device */
errno_t adb_mouse_remove(adb_mouse_t *con)
{
	return ENOTSUP;
}

/** ADB mouse device gone */
errno_t adb_mouse_gone(adb_mouse_t *con)
{
	return ENOTSUP;
}

/** Handle client connection */
static void adb_mouse_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	cap_call_handle_t chandle;
	ipc_call_t call;
	sysarg_t method;
	adb_mouse_t *mouse;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(icall_handle, EOK);

	mouse = (adb_mouse_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

	while (true) {
		chandle = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up. */
			async_answer_0(chandle, EOK);
			return;
		}

		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			mouse->client_sess = sess;
			async_answer_0(chandle, EOK);
		} else {
			async_answer_0(chandle, EINVAL);
		}
	}
}


/**
 * @}
 */
