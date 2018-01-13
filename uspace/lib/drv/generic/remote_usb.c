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
	IPC_M_USB_RESERVE_DEFAULT_ADDRESS,
	IPC_M_USB_RELEASE_DEFAULT_ADDRESS,
	IPC_M_USB_DEVICE_ENUMERATE,
	IPC_M_USB_DEVICE_REMOVE,
	IPC_M_USB_REGISTER_ENDPOINT,
	IPC_M_USB_UNREGISTER_ENDPOINT,
	IPC_M_USB_READ,
	IPC_M_USB_WRITE,
} usb_iface_funcs_t;

/** Tell interface number given device can use.
 * @param[in] exch IPC communication exchange
 * @param[in] handle Id of the device
 * @param[out] usb_iface Assigned USB interface
 * @return Error code.
 */
errno_t usb_get_my_interface(async_exch_t *exch, int *usb_iface)
{
	if (!exch)
		return EBADMEM;
	sysarg_t iface_no;
	const errno_t ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
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
errno_t usb_get_my_device_handle(async_exch_t *exch, devman_handle_t *handle)
{
	devman_handle_t h = 0;
	const errno_t ret = async_req_1_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_MY_DEVICE_HANDLE, &h);
	if (ret == EOK && handle)
		*handle = (devman_handle_t)h;
	return ret;
}

/** Reserve default USB address.
 * @param[in] exch IPC communication exchange
 * @param[in] speed Communication speed of the newly attached device
 * @return Error code.
 */
errno_t usb_reserve_default_address(async_exch_t *exch, usb_speed_t speed)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_RESERVE_DEFAULT_ADDRESS, speed);
}

/** Release default USB address.
 *
 * @param[in] exch IPC communication exchange
 *
 * @return Error code.
 *
 */
errno_t usb_release_default_address(async_exch_t *exch)
{
	if (!exch)
		return EBADMEM;
	return async_req_1_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_RELEASE_DEFAULT_ADDRESS);
}

/** Trigger USB device enumeration
 *
 * @param[in]  exch   IPC communication exchange
 * @param[out] handle Identifier of the newly added device (if successful)
 *
 * @return Error code.
 *
 */
errno_t usb_device_enumerate(async_exch_t *exch, unsigned port)
{
	if (!exch)
		return EBADMEM;
	const errno_t ret = async_req_2_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_DEVICE_ENUMERATE, port);
	return ret;
}

/** Trigger USB device enumeration
 *
 * @param[in] exch   IPC communication exchange
 * @param[in] handle Identifier of the device
 *
 * @return Error code.
 *
 */
errno_t usb_device_remove(async_exch_t *exch, unsigned port)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_DEVICE_REMOVE, port);
}

static_assert(sizeof(sysarg_t) >= 4);

typedef union {
	uint8_t arr[sizeof(sysarg_t)];
	sysarg_t arg;
} pack8_t;

errno_t usb_register_endpoint(async_exch_t *exch, usb_endpoint_t endpoint,
    usb_transfer_type_t type, usb_direction_t direction,
    size_t mps, unsigned packets, unsigned interval)
{
	if (!exch)
		return EBADMEM;
	pack8_t pack;
	pack.arr[0] = type;
	pack.arr[1] = direction;
	pack.arr[2] = interval;
	pack.arr[3] = packets;

	return async_req_4_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_REGISTER_ENDPOINT, endpoint, pack.arg, mps);

}

errno_t usb_unregister_endpoint(async_exch_t *exch, usb_endpoint_t endpoint,
    usb_direction_t direction)
{
	if (!exch)
		return EBADMEM;
	return async_req_3_0(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_UNREGISTER_ENDPOINT, endpoint, direction);
}

errno_t usb_read(async_exch_t *exch, usb_endpoint_t endpoint, uint64_t setup,
    void *data, size_t size, size_t *rec_size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	/* Make call identifying target USB device and type of transfer. */
	aid_t opening_request = async_send_4(exch,
	    DEV_IFACE_ID(USB_DEV_IFACE), IPC_M_USB_READ, endpoint,
	    (setup & UINT32_MAX), (setup >> 32), NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	/* Retrieve the data. */
	ipc_call_t data_request_call;
	aid_t data_request =
	    async_data_read(exch, data, size, &data_request_call);

	if (data_request == 0) {
		// FIXME: How to let the other side know that we want to abort?
		async_forget(opening_request);
		return ENOMEM;
	}

	/* Wait for the answer. */
	errno_t data_request_rc;
	errno_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK) {
			return (errno_t) opening_request_rc;
		} else {
			return (errno_t) data_request_rc;
		}
	}
	if (opening_request_rc != EOK) {
		return (errno_t) opening_request_rc;
	}

	*rec_size = IPC_GET_ARG2(data_request_call);
	return EOK;
}

errno_t usb_write(async_exch_t *exch, usb_endpoint_t endpoint, uint64_t setup,
    const void *data, size_t size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	aid_t opening_request = async_send_5(exch,
	    DEV_IFACE_ID(USB_DEV_IFACE), IPC_M_USB_WRITE, endpoint, size,
	    (setup & UINT32_MAX), (setup >> 32), NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	/* Send the data if any. */
	if (size > 0) {
		const errno_t ret = async_data_write_start(exch, data, size);
		if (ret != EOK) {
			async_forget(opening_request);
			return ret;
		}
	}

	/* Wait for the answer. */
	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (errno_t) opening_request_rc;
}

static void remote_usb_get_my_interface(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_get_my_device_handle(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_reserve_default_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_release_default_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_device_enumerate(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_device_remove(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_register_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_unregister_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usb_read(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call);
static void remote_usb_write(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_usb_iface_ops [] = {
	[IPC_M_USB_GET_MY_INTERFACE] = remote_usb_get_my_interface,
	[IPC_M_USB_GET_MY_DEVICE_HANDLE] = remote_usb_get_my_device_handle,
	[IPC_M_USB_RESERVE_DEFAULT_ADDRESS] = remote_usb_reserve_default_address,
	[IPC_M_USB_RELEASE_DEFAULT_ADDRESS] = remote_usb_release_default_address,
	[IPC_M_USB_DEVICE_ENUMERATE] = remote_usb_device_enumerate,
	[IPC_M_USB_DEVICE_REMOVE] = remote_usb_device_remove,
	[IPC_M_USB_REGISTER_ENDPOINT] = remote_usb_register_endpoint,
	[IPC_M_USB_UNREGISTER_ENDPOINT] = remote_usb_unregister_endpoint,
	[IPC_M_USB_READ] = remote_usb_read,
	[IPC_M_USB_WRITE] = remote_usb_write,
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
	const errno_t ret = usb_iface->get_my_interface(fun, &iface_no);
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
	const errno_t ret = usb_iface->get_my_device_handle(fun, &handle);
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
	const errno_t ret = usb_iface->reserve_default_address(fun, speed);
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

	const errno_t ret = usb_iface->release_default_address(fun);
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

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = usb_iface->device_enumerate(fun, port);
	async_answer_0(callid, ret);
}

static void remote_usb_device_remove(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usb_iface_t *usb_iface = (usb_iface_t *) iface;

	if (usb_iface->device_remove == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = usb_iface->device_remove(fun, port);
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

	const usb_endpoint_t endpoint = DEV_IPC_GET_ARG1(*call);
	const pack8_t pack = { .arg = DEV_IPC_GET_ARG2(*call)};
	const size_t max_packet_size = DEV_IPC_GET_ARG3(*call);

	const usb_transfer_type_t transfer_type = pack.arr[0];
	const usb_direction_t direction = pack.arr[1];
	unsigned packets = pack.arr[2];
	unsigned interval = pack.arr[3];

	const errno_t ret = usb_iface->register_endpoint(fun, endpoint,
	    transfer_type, direction, max_packet_size, packets, interval);

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

	errno_t rc = usb_iface->unregister_endpoint(fun, endpoint, direction);

	async_answer_0(callid, rc);
}

typedef struct {
	ipc_callid_t caller;
	ipc_callid_t data_caller;
	void *buffer;
} async_transaction_t;

static void async_transaction_destroy(async_transaction_t *trans)
{
	if (trans == NULL) {
		return;
	}
	if (trans->buffer != NULL) {
		free(trans->buffer);
	}

	free(trans);
}

static async_transaction_t *async_transaction_create(ipc_callid_t caller)
{
	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	if (trans == NULL) {
		return NULL;
	}

	trans->caller = caller;
	trans->data_caller = 0;
	trans->buffer = NULL;

	return trans;
}

static void callback_out(errno_t outcome, void *arg)
{
	async_transaction_t *trans = arg;

	async_answer_0(trans->caller, outcome);

	async_transaction_destroy(trans);
}

static void callback_in(errno_t outcome, size_t actual_size, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	if (outcome != EOK) {
		async_answer_0(trans->caller, outcome);
		if (trans->data_caller) {
			async_answer_0(trans->data_caller, EINTR);
		}
		async_transaction_destroy(trans);
		return;
	}

	if (trans->data_caller) {
		async_data_read_finalize(trans->data_caller,
		    trans->buffer, actual_size);
	}

	async_answer_0(trans->caller, EOK);

	async_transaction_destroy(trans);
}

void remote_usb_read(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usb_iface_t *usb_iface = iface;

	if (!usb_iface->read) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const usb_endpoint_t ep = DEV_IPC_GET_ARG1(*call);
	const uint64_t setup =
	    ((uint64_t)DEV_IPC_GET_ARG2(*call)) |
	    (((uint64_t)DEV_IPC_GET_ARG3(*call)) << 32);

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	size_t size = 0;
	if (!async_data_read_receive(&trans->data_caller, &size)) {
		async_answer_0(callid, EPARTY);
		async_transaction_destroy(trans);
		return;
	}

	trans->buffer = malloc(size);
	if (trans->buffer == NULL) {
		async_answer_0(trans->data_caller, ENOMEM);
		async_answer_0(callid, ENOMEM);
		async_transaction_destroy(trans);
		return;
	}

	const errno_t rc = usb_iface->read(
	    fun, ep, setup, trans->buffer, size, callback_in, trans);

	if (rc != EOK) {
		async_answer_0(trans->data_caller, rc);
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}

void remote_usb_write(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usb_iface_t *usb_iface = iface;

	if (!usb_iface->write) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const usb_endpoint_t ep = DEV_IPC_GET_ARG1(*call);
	const size_t data_buffer_len = DEV_IPC_GET_ARG2(*call);
	const uint64_t setup =
	    ((uint64_t)DEV_IPC_GET_ARG3(*call)) |
	    (((uint64_t)DEV_IPC_GET_ARG4(*call)) << 32);

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	size_t size = 0;
	if (data_buffer_len > 0) {
		const errno_t rc = async_data_write_accept(&trans->buffer, false,
		    1, data_buffer_len, 0, &size);

		if (rc != EOK) {
			async_answer_0(callid, rc);
			async_transaction_destroy(trans);
			return;
		}
	}

	const errno_t rc = usb_iface->write(
	    fun, ep, setup, trans->buffer, size, callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}
/**
 * @}
 */
