/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * Connection handling of calls from virtual device (implementation).
 */

#include <assert.h>
#include <errno.h>
#include <ddf/driver.h>
#include <usbvirt/ipc.h>
#include <usb/debug.h>
#include <async.h>

#include "vhcd.h"

static fibril_local uintptr_t plugged_device_handle = 0;
#define PLUGGED_DEVICE_NAME_MAXLEN 256
static fibril_local char plugged_device_name[PLUGGED_DEVICE_NAME_MAXLEN + 1] = "<unknown>";

/** Receive device name.
 *
 * @warning Errors are silently ignored.
 *
 * @param sess Session to the virtual device.
 *
 */
static void receive_device_name(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);

	aid_t opening_request = async_send_0(exch, IPC_M_USBVIRT_GET_NAME, NULL);
	if (opening_request == 0) {
		async_exchange_end(exch);
		return;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, plugged_device_name,
	    PLUGGED_DEVICE_NAME_MAXLEN, &data_request_call);

	async_exchange_end(exch);

	if (data_request == 0) {
		async_forget(opening_request);
		return;
	}

	errno_t data_request_rc;
	errno_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if ((data_request_rc != EOK) || (opening_request_rc != EOK))
		return;

	size_t len = ipc_get_arg2(&data_request_call);
	plugged_device_name[len] = 0;
}

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun   Device handling the call.
 * @param icall Call data.
 *
 */
void default_connection_handler(ddf_fun_t *fun, ipc_call_t *icall)
{
	vhc_data_t *vhc = ddf_fun_data_get(fun);

	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);

	if (callback) {
		errno_t rc = vhc_virtdev_plug(vhc, callback, &plugged_device_handle);
		if (rc != EOK) {
			async_answer_0(icall, rc);
			async_hangup(callback);
			return;
		}

		async_answer_0(icall, EOK);

		receive_device_name(callback);

		usb_log_info("New virtual device `%s' (id: %" PRIxn ").",
		    plugged_device_name, plugged_device_handle);
	} else
		async_answer_0(icall, EINVAL);
}

/** Callback when client disconnects.
 *
 * Used to unplug virtual USB device.
 *
 * @param fun
 */
void on_client_close(ddf_fun_t *fun)
{
	vhc_data_t *vhc = ddf_fun_data_get(fun);

	if (plugged_device_handle != 0) {
		usb_log_info("Virtual device `%s' disconnected (id: %" PRIxn ").",
		    plugged_device_name, plugged_device_handle);
		vhc_virtdev_unplug(vhc, plugged_device_handle);
	}
}

/**
 * @}
 */
