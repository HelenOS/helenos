/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <assert.h>
#include <macros.h>
#include <errno.h>
#include <devman.h>

#include "usb_iface.h"
#include "ddf/driver.h"


usb_dev_session_t *usb_dev_connect(devman_handle_t handle)
{
	return devman_device_connect(handle, IPC_FLAG_BLOCKING);
}

usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *dev)
{
	return devman_parent_device_connect(ddf_dev_get_handle(dev), IPC_FLAG_BLOCKING);
}

void usb_dev_disconnect(usb_dev_session_t *sess)
{
	if (sess)
		async_hangup(sess);
}

typedef enum {
	IPC_M_USB_GET_MY_INTERFACE,
	IPC_M_USB_GET_MY_DEVICE_HANDLE,
} usb_iface_funcs_t;

/** Tell interface number given device can use.
 * @param[in] exch IPC communication exchange
 * @param[in] handle Id of the device
 * @param[out] usb_iface Assigned USB interface
 * @return Error code.
 */
int usb_get_my_interface(async_exch_t *exch, int *usb_iface)
{
	if (!exch)
		return EBADMEM;
	sysarg_t iface_no;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_MY_INTERFACE, &iface_no);
	if (ret == EOK && usb_iface)
		*usb_iface = (int)iface_no;
	return ret;
}

/** Tell devman handle of the usb device function.
 *
 * @param[in]  exch   IPC communication exchange
 * @param[out] handle devman handle of the HC used by the target device.
 *
 * @return Error code.
 *
 */
int usb_get_my_device_handle(async_exch_t *exch, devman_handle_t *handle)
{
	devman_handle_t h = 0;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_MY_DEVICE_HANDLE, &h);
	if (ret == EOK && handle)
		*handle = (devman_handle_t)h;
	return ret;
}

static void remote_usb_get_my_interface(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_get_my_device_handle(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_usb_iface_ops [] = {
	[IPC_M_USB_GET_MY_INTERFACE] = remote_usb_get_my_interface,
	[IPC_M_USB_GET_MY_DEVICE_HANDLE] = remote_usb_get_my_device_handle,
};

/** Remote USB interface structure.
 */
const remote_iface_t remote_usb_iface = {
	.method_count = ARRAY_SIZE(remote_usb_iface_ops),
	.methods = remote_usb_iface_ops,
};

void remote_usb_get_my_interface(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_my_interface == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	int iface_no;
	const int ret = usb_iface->get_my_interface(fun, &iface_no);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, iface_no);
	}
}

void remote_usb_get_my_device_handle(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_my_device_handle == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	devman_handle_t handle;
	const int ret = usb_iface->get_my_device_handle(fun, &handle);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	}

	async_answer_1(callid, EOK, (sysarg_t) handle);
}

/**
 * @}
 */
