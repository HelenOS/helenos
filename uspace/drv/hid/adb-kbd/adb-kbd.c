/*
 * Copyright (c) 2011 Jiri Svoboda
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
 * @brief ADB keyboard port driver.
 */

#include <async.h>
#include <ddf/log.h>
#include <errno.h>
#include <io/console.h>
#include <ipc/adb.h>
#include <ipc/kbdev.h>
#include <loc.h>
#include <vfs/vfs.h>

#include "adb-kbd.h"
#include "ctl.h"

static void adb_kbd_events(cap_call_handle_t, ipc_call_t *, void *);
static void adb_kbd_reg0_data(adb_kbd_t *, uint16_t);
static void adb_kbd_conn(cap_call_handle_t, ipc_call_t *, void *);

/** Add ADB keyboard device */
errno_t adb_kbd_add(adb_kbd_t *kbd)
{
	errno_t rc;
	bool bound = false;

	kbd->fun = ddf_fun_create(kbd->dev, fun_exposed, "a");
	if (kbd->fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function");
		rc = ENOMEM;
		goto error;
	}

	kbd->parent_sess = ddf_dev_parent_sess_get(kbd->dev);
	if (kbd->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Error connecting parent driver");
		rc = EIO;
		goto error;
	}

	async_exch_t *exch = async_exchange_begin(kbd->parent_sess);
	if (exch == NULL) {
		ddf_msg(LVL_ERROR, "Error starting exchange with parent");
		rc = ENOMEM;
		goto error;
	}

	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_ADB_CB, 0, 0,
	    adb_kbd_events, kbd, &port);

	async_exchange_end(exch);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error creating callback from device");
		goto error;
	}

	ddf_fun_set_conn_handler(kbd->fun, adb_kbd_conn);

	rc = ddf_fun_bind(kbd->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function");
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(kbd->fun, "keyboard");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding function to category");
		goto error;
	}

	return EOK;
error:
	if (bound)
		ddf_fun_unbind(kbd->fun);

	if (kbd->parent_sess != NULL) {
		async_hangup(kbd->parent_sess);
		kbd->parent_sess = NULL;
	}

	if (kbd->fun != NULL) {
		ddf_fun_destroy(kbd->fun);
		kbd->fun = NULL;
	}

	return rc;
}

/** Remove ADB keyboard device */
errno_t adb_kbd_remove(adb_kbd_t *con)
{
	return ENOTSUP;
}

/** ADB keyboard device gone */
errno_t adb_kbd_gone(adb_kbd_t *con)
{
	return ENOTSUP;
}

static void adb_kbd_events(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	adb_kbd_t *kbd = (adb_kbd_t *) arg;

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
			adb_kbd_reg0_data(kbd, IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}
		async_answer_0(chandle, retval);
	}
}

static void adb_kbd_data(adb_kbd_t *kbd, uint8_t b)
{
	kbd_event_type_t etype;
	unsigned int key;
	errno_t rc;

	rc = adb_kbd_key_translate(b, &etype, &key);
	if (rc != EOK)
		return;

	if (kbd->client_sess == NULL)
		return;

	async_exch_t *exch = async_exchange_begin(kbd->client_sess);
	async_msg_4(exch, KBDEV_EVENT, etype, key, 0, 0);
	async_exchange_end(exch);
}

static void adb_kbd_reg0_data(adb_kbd_t *kbd, uint16_t data)
{
	uint8_t b0 = (data >> 8) & 0xff;
	uint8_t b1 = data & 0xff;

	if (b0 != 0xff)
		adb_kbd_data(kbd, b0);

	if (b1 != 0xff)
		adb_kbd_data(kbd, b1);
	(void)b0;
	(void)b1;
}

/** Handle client connection */
static void adb_kbd_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	cap_call_handle_t chandle;
	ipc_call_t call;
	sysarg_t method;
	adb_kbd_t *kbd;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(icall_handle, EOK);

	kbd = (adb_kbd_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

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
			kbd->client_sess = sess;
			async_answer_0(chandle, EOK);
		} else {
			async_answer_0(chandle, EINVAL);
		}
	}
}

/**
 * @}
 */
