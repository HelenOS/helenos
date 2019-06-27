/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek
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
#include <as.h>

#include "usbhc_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_USB_DEFAULT_ADDRESS_RESERVATION,
	IPC_M_USB_DEVICE_ENUMERATE,
	IPC_M_USB_DEVICE_REMOVE,
	IPC_M_USB_REGISTER_ENDPOINT,
	IPC_M_USB_UNREGISTER_ENDPOINT,
	IPC_M_USB_TRANSFER,
} usbhc_iface_funcs_t;

/** Reserve default USB address.
 * @param[in] exch IPC communication exchange
 * @return Error code.
 */
errno_t usbhc_reserve_default_address(async_exch_t *exch)
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
errno_t usbhc_release_default_address(async_exch_t *exch)
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
errno_t usbhc_device_enumerate(async_exch_t *exch, unsigned port, usb_speed_t speed)
{
	if (!exch)
		return EBADMEM;
	const errno_t ret = async_req_3_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
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
errno_t usbhc_device_remove(async_exch_t *exch, unsigned port)
{
	if (!exch)
		return EBADMEM;
	return async_req_2_0(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USB_DEVICE_REMOVE, port);
}

errno_t usbhc_register_endpoint(async_exch_t *exch, usb_pipe_desc_t *pipe_desc,
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

	errno_t ret = async_data_write_start(exch, desc, sizeof(*desc));
	if (ret != EOK) {
		async_forget(opening_request);
		return ret;
	}

	/* Wait for the answer. */
	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	if (opening_request_rc)
		return (errno_t) opening_request_rc;

	usb_pipe_desc_t dest;
	ret = async_data_read_start(exch, &dest, sizeof(dest));
	if (ret != EOK) {
		return ret;
	}

	if (pipe_desc)
		*pipe_desc = dest;

	return EOK;
}

errno_t usbhc_unregister_endpoint(async_exch_t *exch, const usb_pipe_desc_t *pipe_desc)
{
	if (!exch)
		return EBADMEM;

	aid_t opening_request = async_send_1(exch,
	    DEV_IFACE_ID(USBHC_DEV_IFACE), IPC_M_USB_UNREGISTER_ENDPOINT, NULL);

	if (opening_request == 0) {
		return ENOMEM;
	}

	const errno_t ret = async_data_write_start(exch, pipe_desc, sizeof(*pipe_desc));
	if (ret != EOK) {
		async_forget(opening_request);
		return ret;
	}

	/* Wait for the answer. */
	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (errno_t) opening_request_rc;
}

/**
 * Issue a USB transfer with a data contained in memory area. That area is
 * temporarily shared with the HC.
 */
errno_t usbhc_transfer(async_exch_t *exch,
    const usbhc_iface_transfer_request_t *req, size_t *transferred)
{
	if (transferred)
		*transferred = 0;

	if (!exch)
		return EBADMEM;

	ipc_call_t call;

	aid_t opening_request = async_send_1(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USB_TRANSFER, &call);

	if (opening_request == 0)
		return ENOMEM;

	const errno_t ret = async_data_write_start(exch, req, sizeof(*req));
	if (ret != EOK) {
		async_forget(opening_request);
		return ret;
	}

	/* Share the data, if any. */
	if (req->size > 0) {
		unsigned flags = (req->dir == USB_DIRECTION_IN) ?
		    AS_AREA_WRITE : AS_AREA_READ;

		const errno_t ret = async_share_out_start(exch, req->buffer.virt, flags);
		if (ret != EOK) {
			async_forget(opening_request);
			return ret;
		}
	}

	/* Wait for the answer. */
	errno_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	if (transferred)
		*transferred = ipc_get_arg1(&call);

	return (errno_t) opening_request_rc;
}

static void remote_usbhc_default_address_reservation(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbhc_device_enumerate(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbhc_device_remove(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbhc_register_endpoint(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbhc_unregister_endpoint(ddf_fun_t *, void *, ipc_call_t *);
static void remote_usbhc_transfer(ddf_fun_t *fun, void *iface, ipc_call_t *call);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_usbhc_iface_ops [] = {
	[IPC_M_USB_DEFAULT_ADDRESS_RESERVATION] = remote_usbhc_default_address_reservation,
	[IPC_M_USB_DEVICE_ENUMERATE] = remote_usbhc_device_enumerate,
	[IPC_M_USB_DEVICE_REMOVE] = remote_usbhc_device_remove,
	[IPC_M_USB_REGISTER_ENDPOINT] = remote_usbhc_register_endpoint,
	[IPC_M_USB_UNREGISTER_ENDPOINT] = remote_usbhc_unregister_endpoint,
	[IPC_M_USB_TRANSFER] = remote_usbhc_transfer,
};

/** Remote USB interface structure.
 */
const remote_iface_t remote_usbhc_iface = {
	.method_count = ARRAY_SIZE(remote_usbhc_iface_ops),
	.methods = remote_usbhc_iface_ops,
};

typedef struct {
	ipc_call_t call;
	usbhc_iface_transfer_request_t request;
} async_transaction_t;

void remote_usbhc_default_address_reservation(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->default_address_reservation == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const bool reserve = ipc_get_arg2(call);
	const errno_t ret = usbhc_iface->default_address_reservation(fun, reserve);
	async_answer_0(call, ret);
}

static void remote_usbhc_device_enumerate(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->device_enumerate == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	usb_speed_t speed = DEV_IPC_GET_ARG2(*call);
	const errno_t ret = usbhc_iface->device_enumerate(fun, port, speed);
	async_answer_0(call, ret);
}

static void remote_usbhc_device_remove(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const usbhc_iface_t *usbhc_iface = (usbhc_iface_t *) iface;

	if (usbhc_iface->device_remove == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const unsigned port = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = usbhc_iface->device_remove(fun, port);
	async_answer_0(call, ret);
}

static void remote_usbhc_register_endpoint(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->register_endpoint) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	usb_endpoint_descriptors_t ep_desc;
	ipc_call_t data;
	size_t len;

	if (!async_data_write_receive(&data, &len) ||
	    len != sizeof(ep_desc)) {
		async_answer_0(call, EINVAL);
		return;
	}

	async_data_write_finalize(&data, &ep_desc, sizeof(ep_desc));

	usb_pipe_desc_t pipe_desc;

	const errno_t rc = usbhc_iface->register_endpoint(fun, &pipe_desc, &ep_desc);
	async_answer_0(call, rc);

	if (!async_data_read_receive(&data, &len) ||
	    len != sizeof(pipe_desc)) {
		return;
	}
	async_data_read_finalize(&data, &pipe_desc, sizeof(pipe_desc));
}

static void remote_usbhc_unregister_endpoint(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->unregister_endpoint) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	usb_pipe_desc_t pipe_desc;
	ipc_call_t data;
	size_t len;

	if (!async_data_write_receive(&data, &len) ||
	    len != sizeof(pipe_desc)) {
		async_answer_0(call, EINVAL);
		return;
	}
	async_data_write_finalize(&data, &pipe_desc, sizeof(pipe_desc));

	const errno_t rc = usbhc_iface->unregister_endpoint(fun, &pipe_desc);
	async_answer_0(call, rc);
}

static void async_transaction_destroy(async_transaction_t *trans)
{
	if (trans == NULL) {
		return;
	}
	if (trans->request.buffer.virt != NULL) {
		as_area_destroy(trans->request.buffer.virt);
	}

	free(trans);
}

static async_transaction_t *async_transaction_create(ipc_call_t *call)
{
	async_transaction_t *trans = calloc(1, sizeof(async_transaction_t));

	if (trans != NULL)
		trans->call = *call;

	return trans;
}

static errno_t transfer_finished(void *arg, errno_t error, size_t transferred_size)
{
	async_transaction_t *trans = arg;
	const errno_t err = async_answer_1(&trans->call, error, transferred_size);
	async_transaction_destroy(trans);
	return err;
}

static errno_t receive_memory_buffer(async_transaction_t *trans)
{
	assert(trans);
	assert(trans->request.size > 0);

	const size_t required_size = trans->request.offset + trans->request.size;
	const unsigned required_flags =
	    (trans->request.dir == USB_DIRECTION_IN) ?
	    AS_AREA_WRITE : AS_AREA_READ;

	errno_t err;
	ipc_call_t data;
	size_t size;
	unsigned flags;

	if (!async_share_out_receive(&data, &size, &flags))
		return EPARTY;

	if (size < required_size || (flags & required_flags) != required_flags) {
		async_answer_0(&data, EINVAL);
		return EINVAL;
	}

	if ((err = async_share_out_finalize(&data, &trans->request.buffer.virt)))
		return err;

	/*
	 * As we're going to get physical addresses of the mapping, we must make
	 * sure the memory is actually mapped. We must do it right now, because
	 * the area might be read-only or write-only, and we may be unsure
	 * later.
	 */
	if (flags & AS_AREA_READ) {
		char foo = 0;
		volatile const char *buf = trans->request.buffer.virt + trans->request.offset;
		for (size_t i = 0; i < size; i += PAGE_SIZE)
			foo += buf[i];
	} else {
		volatile char *buf = trans->request.buffer.virt + trans->request.offset;
		for (size_t i = 0; i < size; i += PAGE_SIZE)
			buf[i] = 0xff;
	}

	return EOK;
}

void remote_usbhc_transfer(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *usbhc_iface = iface;

	if (!usbhc_iface->transfer) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	async_transaction_t *trans =
	    async_transaction_create(call);
	if (trans == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	errno_t err = EPARTY;

	ipc_call_t data;
	size_t len;
	if (!async_data_write_receive(&data, &len) ||
	    len != sizeof(trans->request)) {
		async_answer_0(&data, EINVAL);
		goto err;
	}

	if ((err = async_data_write_finalize(&data,
	    &trans->request, sizeof(trans->request))))
		goto err;

	if (trans->request.size > 0) {
		if ((err = receive_memory_buffer(trans)))
			goto err;
	} else {
		/* The value was valid on the other side, for us, its garbage. */
		trans->request.buffer.virt = NULL;
	}

	if ((err = usbhc_iface->transfer(fun, &trans->request,
	    &transfer_finished, trans)))
		goto err;

	/* The call will be answered asynchronously by the callback. */
	return;

err:
	async_answer_0(call, err);
	async_transaction_destroy(trans);
}

/**
 * @}
 */
