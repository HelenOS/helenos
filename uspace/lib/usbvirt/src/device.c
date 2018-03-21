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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Virtual USB device main routines.
 */

#include <errno.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <async.h>
#include <devman.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>

#include <usb/debug.h>

/** Current device. */
static usbvirt_device_t *DEV = NULL;

/** Main IPC call handling from virtual host controller.
 *
 * @param iid   Caller identification
 * @param icall Initial incoming call
 * @param arg   Local argument
 */
static void callback_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	assert(DEV != NULL);

	async_answer_0(icall_handle, EOK);

	while (true) {
		cap_call_handle_t chandle;
		ipc_call_t call;

		chandle = async_get_call(&call);
		bool processed = usbvirt_ipc_handle_call(DEV, chandle, &call);
		if (!processed) {
			if (!IPC_GET_IMETHOD(call)) {
				async_answer_0(chandle, EOK);
				return;
			} else
				async_answer_0(chandle, EINVAL);
		}
	}
}

/** Connect the device to the virtual host controller.
 *
 * @param dev The virtual device to be (virtually) plugged in.
 * @param vhc_path Devman path to the virtual host controller.
 * @return Error code.
 */
errno_t usbvirt_device_plug(usbvirt_device_t *dev, const char *vhc_path)
{
	if (DEV != NULL)
		return ELIMIT;

	devman_handle_t handle;
	errno_t rc = devman_fun_get_handle(vhc_path, &handle, 0);
	if (rc != EOK)
		return rc;

	async_sess_t *hcd_sess =
	    devman_device_connect(handle, 0);
	if (!hcd_sess)
		return ENOMEM;

	DEV = dev;
	dev->vhc_sess = hcd_sess;

	async_exch_t *exch = async_exchange_begin(hcd_sess);

	port_id_t port;
	rc = async_create_callback_port(exch, INTERFACE_USBVIRT_CB, 0, 0,
	    callback_connection, NULL, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		DEV = NULL;

	return rc;
}

/** Disconnect the device from virtual host controller.
 *
 * @param dev Device to be disconnected.
 */
void usbvirt_device_unplug(usbvirt_device_t *dev)
{
	async_hangup(dev->vhc_sess);
}

/**
 * @}
 */
