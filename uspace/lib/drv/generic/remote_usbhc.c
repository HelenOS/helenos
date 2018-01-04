/*
 * Copyright (c) 2010-2011 Vojtech Horky
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
#include <errno.h>
#include <assert.h>
#include <macros.h>

#include "usbhc_iface.h"
#include "ddf/driver.h"

#define USB_MAX_PAYLOAD_SIZE 1020

/** IPC methods for communication with HC through DDF interface.
 *
 * Notes for async methods:
 *
 * Methods for sending data to device (OUT transactions)
 * - e.g. IPC_M_USBHC_INTERRUPT_OUT -
 * always use the same semantics:
 * - first, IPC call with given method is made
 *   - argument #1 is target address
 *   - argument #2 is target endpoint
 *   - argument #3 is max packet size of the endpoint
 * - this call is immediately followed by IPC data write (from caller)
 * - the initial call (and the whole transaction) is answer after the
 *   transaction is scheduled by the HC and acknowledged by the device
 *   or immediately after error is detected
 * - the answer carries only the error code
 *
 * Methods for retrieving data from device (IN transactions)
 * - e.g. IPC_M_USBHC_INTERRUPT_IN -
 * also use the same semantics:
 * - first, IPC call with given method is made
 *   - argument #1 is target address
 *   - argument #2 is target endpoint
 * - this call is immediately followed by IPC data read (async version)
 * - the call is not answered until the device returns some data (or until
 *   error occurs)
 *
 * Some special methods (NO-DATA transactions) do not send any data. These
 * might behave as both OUT or IN transactions because communication parts
 * where actual buffers are exchanged are omitted.
 **
 * For all these methods, wrap functions exists. Important rule: functions
 * for IN transactions have (as parameters) buffers where retrieved data
 * will be stored. These buffers must be already allocated and shall not be
 * touch until the transaction is completed
 * (e.g. not before calling usb_wait_for() with appropriate handle).
 * OUT transactions buffers can be freed immediately after call is dispatched
 * (i.e. after return from wrapping function).
 *
 */
typedef enum {
	/** Get data from device.
	 * See explanation at usb_iface_funcs_t (IN transaction).
	 */
	IPC_M_USBHC_READ,

	/** Send data to device.
	 * See explanation at usb_iface_funcs_t (OUT transaction).
	 */
	IPC_M_USBHC_WRITE,
} usbhc_iface_funcs_t;

errno_t usbhc_read(async_exch_t *exch, usb_address_t address,
    usb_endpoint_t endpoint, uint64_t setup, void *data, size_t size,
    size_t *rec_size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	const usb_target_t target =
	    {{ .address = address, .endpoint = endpoint }};

	/* Make call identifying target USB device and type of transfer. */
	aid_t opening_request = async_send_4(exch,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_READ, target.packed,
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

errno_t usbhc_write(async_exch_t *exch, usb_address_t address,
    usb_endpoint_t endpoint, uint64_t setup, const void *data, size_t size)
{
	if (!exch)
		return EBADMEM;

	if (size == 0 && setup == 0)
		return EOK;

	const usb_target_t target =
	    {{ .address = address, .endpoint = endpoint }};

	aid_t opening_request = async_send_5(exch, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_WRITE, target.packed, size,
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

static void remote_usbhc_read(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_write(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB host controller interface operations. */
static const remote_iface_func_ptr_t remote_usbhc_iface_ops[] = {
	[IPC_M_USBHC_READ] = remote_usbhc_read,
	[IPC_M_USBHC_WRITE] = remote_usbhc_write,
};

/** Remote USB host controller interface structure.
 */
const remote_iface_t remote_usbhc_iface = {
	.method_count = ARRAY_SIZE(remote_usbhc_iface_ops),
	.methods = remote_usbhc_iface_ops
};

typedef struct {
	ipc_callid_t caller;
	ipc_callid_t data_caller;
	void *buffer;
} async_transaction_t;

static void async_transaction_destroy(async_transaction_t *trans)
{
	if (trans == NULL)
		return;
	
	if (trans->buffer != NULL)
		free(trans->buffer);
	
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

void remote_usbhc_read(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(fun);
	assert(iface);
	assert(call);

	const usbhc_iface_t *hc_iface = iface;

	if (!hc_iface->read) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const usb_target_t target = { .packed = DEV_IPC_GET_ARG1(*call) };
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
		return;
	}

	trans->buffer = malloc(size);
	if (trans->buffer == NULL) {
		async_answer_0(trans->data_caller, ENOMEM);
		async_answer_0(callid, ENOMEM);
		async_transaction_destroy(trans);
		return;
	}

	const errno_t rc = hc_iface->read(
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

	const usbhc_iface_t *hc_iface = iface;

	if (!hc_iface->write) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const usb_target_t target = { .packed = DEV_IPC_GET_ARG1(*call) };
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
		    1, USB_MAX_PAYLOAD_SIZE,
		    0, &size);

		if (rc != EOK) {
			async_answer_0(callid, rc);
			async_transaction_destroy(trans);
			return;
		}
	}

	const errno_t rc = hc_iface->write(
	    fun, target, setup, trans->buffer, size, callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}
/**
 * @}
 */
