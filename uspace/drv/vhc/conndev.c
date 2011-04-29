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
#include "conn.h"

static fibril_local uintptr_t plugged_device_handle = 0;

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun Device handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	vhc_data_t *vhc = fun->dev->driver_data;
	sysarg_t method = IPC_GET_IMETHOD(*icall);

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);
		int rc = vhc_virtdev_plug(vhc, callback,
		    &plugged_device_handle);
		if (rc != EOK) {
			async_answer_0(icallid, rc);
			async_hangup(callback);
			return;
		}

		async_answer_0(icallid, EOK);

		usb_log_info("New virtual device `%s' (id = %" PRIxn ").\n",
		    rc == EOK ? "XXX" : "<unknown>", plugged_device_handle);

		return;
	}

	async_answer_0(icallid, EINVAL);
}

/** Callback when client disconnects.
 *
 * Used to unplug virtual USB device.
 *
 * @param fun
 */
void on_client_close(ddf_fun_t *fun)
{
	vhc_data_t *vhc = fun->dev->driver_data;

	if (plugged_device_handle != 0) {
		usb_log_info("Virtual device disconnected (id = %" PRIxn ").\n",
		    plugged_device_handle);
		vhc_virtdev_unplug(vhc, plugged_device_handle);
	}
}


/**
 * @}
 */
