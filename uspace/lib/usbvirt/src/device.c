/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
 * @param icall Initial incoming call
 * @param arg   Local argument
 *
 */
static void callback_connection(ipc_call_t *icall, void *arg)
{
	assert(DEV != NULL);

	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		bool processed = usbvirt_ipc_handle_call(DEV, &call);
		if (!processed) {
			if (!ipc_get_imethod(&call)) {
				async_answer_0(&call, EOK);
				return;
			} else
				async_answer_0(&call, EINVAL);
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
