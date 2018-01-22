/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Ondrej Hlavaty
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

#include "usbhc_iface.h"
#include "ddf/driver.h"


typedef enum {
	IPC_M_USB_DEFAULT_ADDRESS_RESERVATION,
	IPC_M_USB_DEVICE_ENUMERATE,
	IPC_M_USB_DEVICE_REMOVE,
	IPC_M_USB_REGISTER_ENDPOINT,
	IPC_M_USB_UNREGISTER_ENDPOINT,
	IPC_M_USB_READ,
	IPC_M_USB_WRITE,
} usbhc_iface_funcs_t;

/** Reserve default USB address.
 * @param[in] exch IPC communication exchange
 * @return Error code.
 */
int usbhc_reserve_default_address(async_exch_t *exch)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_DEFAULT_ADDRESS_RESERVATION, true);
}

/** Release default USB address.
 *
 * @param[in] exch IPC communication exchange
 *
 * @return Error code.
 */
int usbhc_release_default_address(async_exch_t *exch)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_DEFAULT_ADDRESS_RESERVATION, false);
}

/**
 * Trigger USB device enumeration
 *
 * @param[in] exch IPC communication exchange
 * @param[in] port Port number at which the device is attached
 * @param[in] speed Communication speed of the newly attached device
 *
 * @return Error code.
 */
int usbhc_device_enumerate(async_exch_t *exch, unsigned port, usb_speed_t speed)
{
	if (!exch)
		return EBADMEM;
	const int ret = async_req_3_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USB_DEVICE_ENUMERATE, port, speed);
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
int usbhc_device_remove(async_exch_t *exch, unsigned port)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USB_DEVICE_REMOVE, port);
}

int usbhc_register_endpoint(async_exch_t *exch, usb_pipe_desc_t *pipe_desc,
    const usb_endpoint_descriptors_t *desc)
{
	if (!exch)
		return EBADMEM;

	if (!desc)
		return EINVAL;

	aid_t opening_request = async_send_1(exch,
	    DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_REGISTER_ENDPOINT, NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	int ret = async_data_write_start(exch, desc, sizeof(*desc));
	if (ret != EOK) {
		async_forget(opening_request);
		return ret;
	}

	/* Wait for the answer. */
	sysarg_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	if (opening_request_rc)
		return (int) opening_request_rc;

	usb_pipe_desc_t dest;
	ret = async_data_read_start(exch, &dest, sizeof(dest));
	if (ret != EOK) {
		return ret;
	}

	if (pipe_desc)
		*pipe_desc = dest;

	return EOK;
}

int usbhc_unregister_endpoint(async_exch_t *exch, const usb_pipe_desc_t *pipe_desc)
{
	if (!exch)
		return EBADMEM;

	aid_t opening_request = async_send_1(exch,
		DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_UNREGISTER_ENDPOINT, NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	const int ret = async_data_write_start(exch, pipe_desc, sizeof(*pipe_desc));
	if (ret != EOK) {
		async_forget(opening_request);
		return ret;
	}

	/* Wait for the answer. */
	sysarg_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (int) opening_request_rc;
}

int usbhc_read(async_exch_t *exch, usb_endpoint_t endpoint, uint64_t setup,
    void *data, size_t size, size_t *rec_size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	/* Make call identifying target USB device and type of transfer. */
	aid_t opening_request = async_send_4(exch,
	    DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_READ, endpoint,
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
	sysarg_t data_request_rc;
	sysarg_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK) {
			return (int) opening_request_rc;
		} else {
			return (int) data_request_rc;
		}
	}
	if (opening_request_rc != EOK) {
		return (int) opening_request_rc;
	}

	*rec_size = IPC_GET_ARG2(data_request_call);
	return EOK;
}

int usbhc_write(async_exch_t *exch, usb_endpoint_t endpoint, uint64_t setup,
    const void *data, size_t size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	aid_t opening_request = async_send_5(exch,
	    DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_WRITE, endpoint, size,
	    (setup & UINT32_MAX), (setup >> 32), NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	/* Send the data if any. */
	if (size > 0) {
		const int ret = async_data_write_start(exch, data, size);
		if (ret != EOK) {
			async_forget(opening_request);
			return ret;
		}
	}

	/* Wait for the answer. */
	sysarg_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (int) opening_request_rc;
}

static void remote_usbhc_default_address_reservation(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_device_enumerate(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_device_remove(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_register_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_unregister_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_read(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call);
static void remote_usbhc_write(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_usbhc_iface_ops [] = {
	[IPC_M_USB_DEFAULT_ADDRESS_RESERVATION] = remote_usbhc_default_address_reservation,
	[IPC_M_USB_DEVICE_ENUMERATE] = remote_usbhc_device_enumerate,
	[IPC_M_USB_DEVICE_REMOVE] = remote_usbhc_device_remove,
	[IPC_M_USB_REGISTER_ENDPOINT] = remote_usbhc_register_endpoint,
	[IPC_M_USB_UNREGISTER_ENDPOINT] = remote_usbhc_unregister_endpoint,
	[IPC_M_USB_READ] = remote_usbhc_read,
	[IPC_M_USB_WRITE] = remote_usbhc_write,
};

/** Remote USB interface structure.
 */
const remote_iface_t remote_usbhc_iface = {
	.method_count = ARRAY_SIZE(remote_usbhc_iface_ops),
	.methods = remote_usbhc_iface_ops,
};

typedef struct {
	ipc_callid_t caller;
	ipc_callid_t data_caller;
	void *buffer;
} async_transaction_t;

void remote_usbhc_default_address_reservation(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->default_address_reservation == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const bool reserve = IPC_GET_ARG2(*call);
	const int ret = usbhc_iface->default_address_reservation(fun, reserve);
	async_answer_0(callid, ret);
}


static void remote_usbhc_device_enumerate(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->device_enumerate == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	usb_speed_t speed = DEV_IPC_GET_ARG2(*call);
	const int ret = usbhc_iface->device_enumerate(fun, port, speed);
	async_answer_0(callid, ret);
}

static void remote_usbhc_device_remove(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->device_remove == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	const int ret = usbhc_iface->device_remove(fun, port);
	async_answer_0(callid, ret);
}

static void remote_usbhc_register_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->register_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_endpoint_descriptors_t ep_desc;
	ipc_callid_t data_callid;
	size_t len;

	if (!async_data_write_receive(&data_callid, &len)
	    || len != sizeof(ep_desc)) {
		async_answer_0(callid, EINVAL);
		return;
	}
	async_data_write_finalize(data_callid, &ep_desc, sizeof(ep_desc));

	usb_pipe_desc_t pipe_desc;

	const int rc = usbhc_iface->register_endpoint(fun, &pipe_desc, &ep_desc);
	async_answer_0(callid, rc);

	if (!async_data_read_receive(&data_callid, &len)
	    || len != sizeof(pipe_desc)) {
		return;
	}
	async_data_read_finalize(data_callid, &pipe_desc, sizeof(pipe_desc));
}

static void remote_usbhc_unregister_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->unregister_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_pipe_desc_t pipe_desc;
	ipc_callid_t data_callid;
	size_t len;

	if (!async_data_write_receive(&data_callid, &len)
	    || len != sizeof(pipe_desc)) {
		async_answer_0(callid, EINVAL);
		return;
	}
	async_data_write_finalize(data_callid, &pipe_desc, sizeof(pipe_desc));

	const int rc = usbhc_iface->unregister_endpoint(fun, &pipe_desc);
	async_answer_0(callid, rc);
}

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

static int callback_out(void *arg, int error, size_t transferred_size)
{
	async_transaction_t *trans = arg;

	const int err = async_answer_0(trans->caller, error);

	async_transaction_destroy(trans);

	return err;
}

static int callback_in(void *arg, int error, size_t transferred_size)
{
	async_transaction_t *trans = arg;

	if (trans->data_caller) {
		if (error == EOK) {
			error = async_data_read_finalize(trans->data_caller,
			    trans->buffer, transferred_size);
		} else {
			async_answer_0(trans->data_caller, EINTR);
		}
	}

	const int err = async_answer_0(trans->caller, error);
	async_transaction_destroy(trans);
	return err;
}

void remote_usbhc_read(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->read) {
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

	const usb_target_t target = {{
		/* .address is initialized by read itself */
		.endpoint = ep,
	}};

	const int rc = usbhc_iface->read(
	    fun, target, setup, trans->buffer, size, callback_in, trans);

	if (rc != EOK) {
		async_answer_0(trans->data_caller, rc);
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}

void remote_usbhc_write(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->write) {
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
		const int rc = async_data_write_accept(&trans->buffer, false,
		    1, data_buffer_len, 0, &size);

		if (rc != EOK) {
			async_answer_0(callid, rc);
			async_transaction_destroy(trans);
			return;
		}
	}

	const usb_target_t target = {{
		/* .address is initialized by write itself */
		.endpoint = ep,
		/* streams are not given by the API call yet */
		.stream = 0,
	}};

	const int rc = usbhc_iface->write(
	    fun, target, setup, trans->buffer, size, callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}
/**
 * @}
 */
