/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Connection handling of calls from virtual device (implementation).
 */

#include <assert.h>
#include <errno.h>
#include <usbvirt/hub.h>

#include "conn.h"
#include "hc.h"
#include "hub.h"

#define DEVICE_NAME_MAXLENGTH 32

static int get_device_name(int phone, char *buffer, size_t len)
{
	ipc_call_t answer_data;
	sysarg_t answer_rc;
	aid_t req;
	int rc;
	
	req = async_send_0(phone,
	    IPC_M_USBVIRT_GET_NAME,
	    &answer_data);
	
	rc = async_data_read_start(phone, buffer, len);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return EINVAL;
	}
	
	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;
	
	if (IPC_GET_ARG1(answer_data) < len) {
		len = IPC_GET_ARG1(answer_data);
	} else {
		len--;
	}
	buffer[len] = 0;
	
	return rc;
}

/** Default handler for IPC methods not handled by DDF.
 *
 * @param dev Device handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(device_t *dev,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	sysarg_t method = IPC_GET_IMETHOD(*icall);

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);
		virtdev_connection_t *dev
		    = virtdev_add_device(callback, (sysarg_t)fibril_get_id());
		if (!dev) {
			ipc_answer_0(icallid, EEXISTS);
			ipc_hangup(callback);
			return;
		}
		ipc_answer_0(icallid, EOK);

		char devname[DEVICE_NAME_MAXLENGTH + 1];
		int rc = get_device_name(callback, devname, DEVICE_NAME_MAXLENGTH);

		dprintf(0, "virtual device connected (name: %s, id: %x)",
		    rc == EOK ? devname : "<unknown>", dev->id);

		return;
	}

	ipc_answer_0(icallid, EINVAL);
}

/** Callback for DDF when client disconnects.
 *
 * @param d Device the client was connected to.
 */
void on_client_close(device_t *d)
{
	/*
	 * Maybe a virtual device is being unplugged.
	 */
	virtdev_connection_t *dev = virtdev_find((sysarg_t)fibril_get_id());
	if (dev == NULL) {
		return;
	}

	dprintf(0, "virtual device disconnected (id: %x)", dev->id);
	virtdev_destroy_device(dev);
}


/**
 * @}
 */
