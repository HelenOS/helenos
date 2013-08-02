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
#include <macros.h>
#include <errno.h>
#include <devman.h>

#include "usb_iface.h"
#include "ddf/driver.h"


usb_dev_session_t *usb_dev_connect(devman_handle_t handle)
{
	return devman_device_connect(EXCHANGE_PARALLEL, handle, IPC_FLAG_BLOCKING);
}

usb_dev_session_t *usb_dev_connect_to_self(ddf_dev_t *dev)
{
	// TODO All usb requests are atomic so this is safe,
	// it will need to change once USING EXCHANGE PARALLEL is safe with
	// devman_parent_device_connect
	return devman_parent_device_connect(EXCHANGE_ATOMIC,
	    ddf_dev_get_handle(dev), IPC_FLAG_BLOCKING);
}

void usb_dev_disconnect(usb_dev_session_t *sess)
{
	if (sess)
		async_hangup(sess);
}

typedef enum {
	IPC_M_USB_GET_MY_ADDRESS,
	IPC_M_USB_GET_MY_INTERFACE,
	IPC_M_USB_GET_HOST_CONTROLLER_HANDLE,
	IPC_M_USB_GET_DEVICE_HANDLE,
	IPC_M_USB_RESERVE_DEFAULT_ADDRESS,
	IPC_M_USB_RELEASE_DEFAULT_ADDRESS,
	IPC_M_USB_DEVICE_ENUMERATE,
	IPC_M_USB_DEVICE_REMOVE,
	IPC_M_USB_REGISTER_ENDPOINT,
	IPC_M_USB_UNREGISTER_ENDPOINT,
} usb_iface_funcs_t;

/** Tell USB address assigned to device.
 * @param exch Vaid IPC exchange
 * @param address Pointer to address storage place.
 * @return Error code.
 *
 * Exch param is an open communication to device implementing usb_iface.
 */
int usb_get_my_address(async_exch_t *exch, usb_address_t *address)
{
	sysarg_t addr;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_MY_ADDRESS, &addr);

	if (ret == EOK && address != NULL)
		*address = (usb_address_t) addr;
	return ret;
}

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

/** Tell devman handle of device host controller.
 * @param[in] exch IPC communication exchange
 * @param[out] hc_handle devman handle of the HC used by the target device.
 * @return Error code.
 */
int usb_get_hc_handle(async_exch_t *exch, devman_handle_t *hc_handle)
{
	if (!exch)
		return EBADMEM;
	devman_handle_t h;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_HOST_CONTROLLER_HANDLE, &h);
	if (ret == EOK && hc_handle)
		*hc_handle = (devman_handle_t)h;
	return ret;
}

/** Tell devman handle of the usb device function.
 * @param[in] exch IPC communication exchange
 * @param[out] handle devman handle of the HC used by the target device.
 * @return Error code.
 */
int usb_get_device_handle(async_exch_t *exch, devman_handle_t *handle)
{
	devman_handle_t h = 0;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_DEVICE_HANDLE, &h);
	if (ret == EOK && handle)
		*handle = (devman_handle_t)h;
	return ret;
}

/** Reserve default USB address.
 * @param[in] exch IPC communication exchange
 * @param[in] speed Communication speed of the newly attached device
 * @return Error code.
 */
int usb_reserve_default_address(async_exch_t *exch, usb_speed_t speed)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_RESERVE_DEFAULT_ADDRESS, speed);
}

/** Release default USB address.
 * @param[in] exch IPC communication exchange
 * @return Error code.
 */
int usb_release_default_address(async_exch_t *exch)
{
	if (!exch)
		return EBADMEM;
	return async_req_1_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_RELEASE_DEFAULT_ADDRESS);
}

/** Trigger USB device enumeration
 * @param[in] exch IPC communication exchange
 * @param[out] handle Identifier of the newly added device (if successful)
 * @return Error code.
 */
int usb_device_enumerate(async_exch_t *exch, usb_device_handle_t *handle)
{
	if (!exch || !handle)
		return EBADMEM;
	sysarg_t h;
	const int ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_DEVICE_ENUMERATE, &h);
	if (ret == EOK)
		*handle = (usb_device_handle_t)h;
	return ret;
}

/** Trigger USB device enumeration
 * @param[in] exch IPC communication exchange
 * @param[in] handle Identifier of the device
 * @return Error code.
 */
int usb_device_remove(async_exch_t *exch, usb_device_handle_t handle)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_DEVICE_REMOVE, handle);
}

int usb_register_endpoint(async_exch_t *exch, usb_endpoint_t endpoint,
    usb_transfer_type_t type, usb_direction_t direction,
    size_t mps, unsigned interval)
{
	if (!exch)
		return EBADMEM;
#define _PACK2(high, low) (((high & 0xffff) << 16) | (low & 0xffff))

	return async_req_4_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_REGISTER_ENDPOINT, endpoint,
	    _PACK2(type, direction), _PACK2(mps, interval));

#undef _PACK2
}

int usb_unregister_endpoint(async_exch_t *exch, usb_endpoint_t endpoint,
    usb_direction_t direction)
{
	if (!exch)
		return EBADMEM;
	return async_req_3_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_UNREGISTER_ENDPOINT, endpoint, direction);
}

static void remote_usb_get_my_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_get_my_interface(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_get_hc_handle(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_get_device_handle(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

static void remote_usb_reserve_default_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_release_default_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_device_enumerate(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_device_remove(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_register_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_unregister_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB interface operations. */
static remote_iface_func_ptr_t remote_usb_iface_ops [] = {
	[IPC_M_USB_GET_MY_ADDRESS] = remote_usb_get_my_address,
	[IPC_M_USB_GET_MY_INTERFACE] = remote_usb_get_my_interface,
	[IPC_M_USB_GET_HOST_CONTROLLER_HANDLE] = remote_usb_get_hc_handle,
	[IPC_M_USB_GET_DEVICE_HANDLE] = remote_usb_get_device_handle,
	[IPC_M_USB_RESERVE_DEFAULT_ADDRESS] = remote_usb_reserve_default_address,
	[IPC_M_USB_RELEASE_DEFAULT_ADDRESS] = remote_usb_release_default_address,
	[IPC_M_USB_DEVICE_ENUMERATE] = remote_usb_device_enumerate,
	[IPC_M_USB_DEVICE_REMOVE] = remote_usb_device_remove,
	[IPC_M_USB_REGISTER_ENDPOINT] = remote_usb_register_endpoint,
	[IPC_M_USB_UNREGISTER_ENDPOINT] = remote_usb_unregister_endpoint,
};

/** Remote USB interface structure.
 */
remote_iface_t remote_usb_iface = {
	.method_count = ARRAY_SIZE(remote_usb_iface_ops),
	.methods = remote_usb_iface_ops,
};


void remote_usb_get_my_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_my_address == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address;
	const int ret = usb_iface->get_my_address(fun, &address);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, address);
	}
}

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

void remote_usb_get_hc_handle(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_hc_handle == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	devman_handle_t handle;
	const int ret = usb_iface->get_hc_handle(fun, &handle);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	}

	async_answer_1(callid, EOK, (sysarg_t) handle);
}

void remote_usb_get_device_handle(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->get_device_handle == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	devman_handle_t handle;
	const int ret = usb_iface->get_device_handle(fun, &handle);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	}

	async_answer_1(callid, EOK, (sysarg_t) handle);
}

void remote_usb_reserve_default_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->reserve_default_address == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_speed_t speed = DEV_IPC_GET_ARG1(*call);
	const int ret = usb_iface->reserve_default_address(fun, speed);
	async_answer_0(callid, ret);
}

void remote_usb_release_default_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->release_default_address == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const int ret = usb_iface->release_default_address(fun);
	async_answer_0(callid, ret);
}

static void remote_usb_device_enumerate(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->device_enumerate == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_device_handle_t handle = 0;
	const int ret = usb_iface->device_enumerate(fun, &handle);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	}

	async_answer_1(callid, EOK, (sysarg_t) handle);
}

static void remote_usb_device_remove(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->device_remove == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_device_handle_t handle = DEV_IPC_GET_ARG1(*call);
	const int ret = usb_iface->device_remove(fun, handle);
	async_answer_0(callid, ret);
}

static void remote_usb_register_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (!usb_iface->register_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

#define _INIT_FROM_HIGH_DATA2(type, var, arg_no) \
	type var = (type) (DEV_IPC_GET_ARG##arg_no(*call) >> 16)
#define _INIT_FROM_LOW_DATA2(type, var, arg_no) \
	type var = (type) (DEV_IPC_GET_ARG##arg_no(*call) & 0xffff)

	const usb_endpoint_t endpoint = DEV_IPC_GET_ARG1(*call);

	_INIT_FROM_HIGH_DATA2(usb_transfer_type_t, transfer_type, 2);
	_INIT_FROM_LOW_DATA2(usb_direction_t, direction, 2);

	_INIT_FROM_HIGH_DATA2(size_t, max_packet_size, 3);
	_INIT_FROM_LOW_DATA2(unsigned int, interval, 3);

#undef _INIT_FROM_HIGH_DATA2
#undef _INIT_FROM_LOW_DATA2

	const int ret = usb_iface->register_endpoint(fun, endpoint,
	    transfer_type, direction, max_packet_size, interval);

	async_answer_0(callid, ret);
}

static void remote_usb_unregister_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (!usb_iface->unregister_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_endpoint_t endpoint = (usb_endpoint_t) DEV_IPC_GET_ARG1(*call);
	usb_direction_t direction = (usb_direction_t) DEV_IPC_GET_ARG2(*call);

	int rc = usb_iface->unregister_endpoint(fun, endpoint, direction);

	async_answer_0(callid, rc);
}
/**
 * @}
 */
