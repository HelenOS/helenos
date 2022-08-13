/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Michal Staruch, Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	return devman_parent_device_connect(ddf_dev_get_handle(dev),
	    IPC_FLAG_BLOCKING);
}

void usb_dev_disconnect(usb_dev_session_t *sess)
{
	if (sess)
		async_hangup(sess);
}

typedef enum {
	IPC_M_USB_GET_MY_DESCRIPTION,
} usb_iface_funcs_t;

/** Tell interface number given device can use.
 * @param[in] exch IPC communication exchange
 * @param[in] handle Id of the device
 * @param[out] usb_iface Assigned USB interface
 * @return Error code.
 */
errno_t usb_get_my_description(async_exch_t *exch, usb_device_desc_t *desc)
{
	if (!exch)
		return EBADMEM;

	sysarg_t address, depth, speed, handle, iface;

	const errno_t ret = async_req_1_5(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_MY_DESCRIPTION, &address, &depth, &speed, &handle,
	    &iface);
	if (ret == EOK && desc) {
		*desc = (usb_device_desc_t) {
			.address = address,
			.depth = depth,
			.speed = speed,
			.handle = handle,
			.iface = iface,
		};
	}

	return ret;
}

static void remote_usb_get_my_description(ddf_fun_t *, void *, ipc_call_t *);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_usb_iface_ops [] = {
	[IPC_M_USB_GET_MY_DESCRIPTION] = remote_usb_get_my_description,
};

/** Remote USB interface structure.
 */
const remote_iface_t remote_usb_iface = {
	.method_count = ARRAY_SIZE(remote_usb_iface_ops),
	.methods = remote_usb_iface_ops,
};

void remote_usb_get_my_description(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_my_description == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	usb_device_desc_t desc;
	const errno_t ret = usb_iface->get_my_description(fun, &desc);
	if (ret != EOK) {
		async_answer_0(call, ret);
	} else {
		async_answer_5(call, EOK,
		    (sysarg_t) desc.address,
		    (sysarg_t) desc.depth,
		    (sysarg_t) desc.speed,
		    desc.handle,
		    desc.iface);
	}
}

/**
 * @}
 */
