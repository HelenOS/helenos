/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

#include "usbhc_iface.h"
#include "ddf/driver.h"

#define USB_MAX_PAYLOAD_SIZE 1020

static void remote_usbhc_interrupt_out(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_interrupt_in(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_bulk_out(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_bulk_in(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_write(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_read(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_request_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_bind_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_find_by_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_release_address(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_register_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_unregister_endpoint(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_data_read(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_data_write(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
//static void remote_usbhc(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB host controller interface operations. */
static remote_iface_func_ptr_t remote_usbhc_iface_ops[] = {
	[IPC_M_USBHC_REQUEST_ADDRESS] = remote_usbhc_request_address,
	[IPC_M_USBHC_BIND_ADDRESS] = remote_usbhc_bind_address,
	[IPC_M_USBHC_GET_HANDLE_BY_ADDRESS] = remote_usbhc_find_by_address,
	[IPC_M_USBHC_RELEASE_ADDRESS] = remote_usbhc_release_address,

	[IPC_M_USBHC_INTERRUPT_OUT] = remote_usbhc_interrupt_out,
	[IPC_M_USBHC_INTERRUPT_IN] = remote_usbhc_interrupt_in,

	[IPC_M_USBHC_BULK_OUT] = remote_usbhc_bulk_out,
	[IPC_M_USBHC_BULK_IN] = remote_usbhc_bulk_in,

	[IPC_M_USBHC_CONTROL_WRITE] = remote_usbhc_control_write,
	[IPC_M_USBHC_CONTROL_READ] = remote_usbhc_control_read,

	[IPC_M_USBHC_REGISTER_ENDPOINT] = remote_usbhc_register_endpoint,
	[IPC_M_USBHC_UNREGISTER_ENDPOINT] = remote_usbhc_unregister_endpoint,

	[IPC_M_USBHC_DATA_READ] = remote_usbhc_data_read,
	[IPC_M_USBHC_DATA_WRITE] = remote_usbhc_data_write,
};

/** Remote USB host controller interface structure.
 */
remote_iface_t remote_usbhc_iface = {
	.method_count = sizeof(remote_usbhc_iface_ops) /
	    sizeof(remote_usbhc_iface_ops[0]),
	.methods = remote_usbhc_iface_ops
};

typedef struct {
	ipc_callid_t caller;
	ipc_callid_t data_caller;
	void *buffer;
	void *setup_packet;
	size_t size;
} async_transaction_t;

static void async_transaction_destroy(async_transaction_t *trans)
{
	if (trans == NULL) {
		return;
	}

	if (trans->setup_packet != NULL) {
		free(trans->setup_packet);
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
	trans->setup_packet = NULL;
	trans->size = 0;

	return trans;
}

void remote_usbhc_request_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->request_address) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_speed_t speed = DEV_IPC_GET_ARG1(*call);

	usb_address_t address;
	int rc = usb_iface->request_address(fun, speed, &address);
	if (rc != EOK) {
		async_answer_0(callid, rc);
	} else {
		async_answer_1(callid, EOK, (sysarg_t) address);
	}
}

void remote_usbhc_bind_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->bind_address) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);
	devman_handle_t handle = (devman_handle_t) DEV_IPC_GET_ARG2(*call);

	int rc = usb_iface->bind_address(fun, address, handle);

	async_answer_0(callid, rc);
}

void remote_usbhc_find_by_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->find_by_address) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);
	devman_handle_t handle;
	int rc = usb_iface->find_by_address(fun, address, &handle);

	if (rc == EOK) {
		async_answer_1(callid, EOK, handle);
	} else {
		async_answer_0(callid, rc);
	}
}

void remote_usbhc_release_address(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->release_address) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);

	int rc = usb_iface->release_address(fun, address);

	async_answer_0(callid, rc);
}


static void callback_out(ddf_fun_t *fun,
    int outcome, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	async_answer_0(trans->caller, outcome);

	async_transaction_destroy(trans);
}

static void callback_in(ddf_fun_t *fun,
    int outcome, size_t actual_size, void *arg)
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

	trans->size = actual_size;

	if (trans->data_caller) {
		async_data_read_finalize(trans->data_caller,
		    trans->buffer, actual_size);
	}

	async_answer_0(trans->caller, EOK);

	async_transaction_destroy(trans);
}

/** Process an outgoing transfer (both OUT and SETUP).
 *
 * @param device Target device.
 * @param callid Initiating caller.
 * @param call Initiating call.
 * @param transfer_func Transfer function (might be NULL).
 */
static void remote_usbhc_out_transfer(ddf_fun_t *fun,
    ipc_callid_t callid, ipc_call_t *call,
    usbhc_iface_transfer_out_t transfer_func)
{
	if (!transfer_func) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	size_t len = 0;
	void *buffer = NULL;

	int rc = async_data_write_accept(&buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE,
	    0, &len);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		if (buffer != NULL) {
			free(buffer);
		}
		async_answer_0(callid, ENOMEM);
		return;
	}

	trans->buffer = buffer;
	trans->size = len;

	rc = transfer_func(fun, target,
	    buffer, len,
	    callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}

/** Process an incoming transfer.
 *
 * @param device Target device.
 * @param callid Initiating caller.
 * @param call Initiating call.
 * @param transfer_func Transfer function (might be NULL).
 */
static void remote_usbhc_in_transfer(ddf_fun_t *fun,
    ipc_callid_t callid, ipc_call_t *call,
    usbhc_iface_transfer_in_t transfer_func)
{
	if (!transfer_func) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	size_t len;
	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &len)) {
		async_answer_0(callid, EPARTY);
		return;
	}

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}
	trans->data_caller = data_callid;
	trans->buffer = malloc(len);
	trans->size = len;

	int rc = transfer_func(fun, target,
	    trans->buffer, len,
	    callback_in, trans);

	if (rc != EOK) {
		async_answer_0(data_callid, rc);
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}

void remote_usbhc_interrupt_out(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(fun, callid, call,
	    usb_iface->interrupt_out);
}

void remote_usbhc_interrupt_in(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_in_transfer(fun, callid, call,
	    usb_iface->interrupt_in);
}

void remote_usbhc_bulk_out(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(fun, callid, call,
	    usb_iface->bulk_out);
}

void remote_usbhc_bulk_in(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_in_transfer(fun, callid, call,
	    usb_iface->bulk_in);
}

void remote_usbhc_control_write(ddf_fun_t *fun, void *iface,
ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	if (!usb_iface->write) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};
	size_t data_buffer_len = DEV_IPC_GET_ARG3(*call);

	int rc;

	void *setup_packet = NULL;
	void *data_buffer = NULL;
	size_t setup_packet_len = 0;

	rc = async_data_write_accept(&setup_packet, false,
	    1, USB_MAX_PAYLOAD_SIZE, 0, &setup_packet_len);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	if (data_buffer_len > 0) {
		rc = async_data_write_accept(&data_buffer, false,
		    1, USB_MAX_PAYLOAD_SIZE, 0, &data_buffer_len);
		if (rc != EOK) {
			async_answer_0(callid, rc);
			free(setup_packet);
			return;
		}
	}

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(callid, ENOMEM);
		free(setup_packet);
		free(data_buffer);
		return;
	}
	trans->buffer = data_buffer;
	trans->size = data_buffer_len;

	assert(setup_packet_len == 8);
	uint64_t setup_buffer;
	memcpy(&setup_buffer, setup_packet, 8);
	free(setup_packet);
	rc = usb_iface->write(fun, target, setup_buffer, trans->buffer,
	    trans->size, callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}


void remote_usbhc_control_read(ddf_fun_t *fun, void *iface,
ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	if (!usb_iface->read) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	int rc;

	void *setup_packet = NULL;
	size_t setup_packet_len = 0;
	size_t data_len = 0;

	rc = async_data_write_accept(&setup_packet, false,
	    1, USB_MAX_PAYLOAD_SIZE, 0, &setup_packet_len);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(callid, EPARTY);
		free(setup_packet);
		return;
	}

	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		free(setup_packet);
		return;
	}
	trans->data_caller = data_callid;
	trans->size = data_len;
	trans->buffer = malloc(data_len);
	if (trans->buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		async_transaction_destroy(trans);
		return;
	}

	assert(setup_packet_len == 8);
	uint64_t setup_buffer;
	memcpy(&setup_buffer, setup_packet, 8);
	free(setup_packet);
	rc = usb_iface->read(fun, target, setup_buffer, trans->buffer,
	    trans->size, callback_in, trans);
	if (rc != EOK) {
		async_answer_0(data_callid, rc);
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}


void remote_usbhc_register_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->register_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

#define _INIT_FROM_HIGH_DATA2(type, var, arg_no) \
	type var = (type) DEV_IPC_GET_ARG##arg_no(*call) / (1 << 16)
#define _INIT_FROM_LOW_DATA2(type, var, arg_no) \
	type var = (type) DEV_IPC_GET_ARG##arg_no(*call) % (1 << 16)
#define _INIT_FROM_HIGH_DATA3(type, var, arg_no) \
	type var = (type) DEV_IPC_GET_ARG##arg_no(*call) / (1 << 16)
#define _INIT_FROM_MIDDLE_DATA3(type, var, arg_no) \
	type var = (type) (DEV_IPC_GET_ARG##arg_no(*call) / (1 << 8)) % (1 << 8)
#define _INIT_FROM_LOW_DATA3(type, var, arg_no) \
	type var = (type) DEV_IPC_GET_ARG##arg_no(*call) % (1 << 8)

	_INIT_FROM_HIGH_DATA2(usb_address_t, address, 1);
	_INIT_FROM_LOW_DATA2(usb_endpoint_t, endpoint, 1);

	_INIT_FROM_HIGH_DATA3(usb_speed_t, speed, 2);
	_INIT_FROM_MIDDLE_DATA3(usb_transfer_type_t, transfer_type, 2);
	_INIT_FROM_LOW_DATA3(usb_direction_t, direction, 2);

	_INIT_FROM_HIGH_DATA2(size_t, max_packet_size, 3);
	_INIT_FROM_LOW_DATA2(unsigned int, interval, 3);

#undef _INIT_FROM_HIGH_DATA2
#undef _INIT_FROM_LOW_DATA2
#undef _INIT_FROM_HIGH_DATA3
#undef _INIT_FROM_MIDDLE_DATA3
#undef _INIT_FROM_LOW_DATA3

	int rc = usb_iface->register_endpoint(fun, address, speed, endpoint,
	    transfer_type, direction, max_packet_size, interval);

	async_answer_0(callid, rc);
}


void remote_usbhc_unregister_endpoint(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->unregister_endpoint) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);
	usb_endpoint_t endpoint = (usb_endpoint_t) DEV_IPC_GET_ARG2(*call);
	usb_direction_t direction = (usb_direction_t) DEV_IPC_GET_ARG3(*call);

	int rc = usb_iface->unregister_endpoint(fun,
	    address, endpoint, direction);

	async_answer_0(callid, rc);
}

static void remote_usbhc_data_read(
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

	const usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};


	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	if (!async_data_read_receive(&trans->data_caller, &trans->size)) {
		async_answer_0(callid, EPARTY);
		return;
	}

	trans->buffer = malloc(trans->size);
	if (trans->buffer == NULL) {
		async_answer_0(trans->data_caller, ENOMEM);
		async_answer_0(callid, ENOMEM);
		async_transaction_destroy(trans);
	}

	const int rc = hc_iface->read(
	    fun, target, 0, trans->buffer, trans->size, callback_in, trans);

	if (rc != EOK) {
		async_answer_0(trans->data_caller, rc);
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}

static void remote_usbhc_data_write(
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

	const usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};


	async_transaction_t *trans = async_transaction_create(callid);
	if (trans == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	int rc = async_data_write_accept(&trans->buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE,
	    0, &trans->size);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
		return;
	}

	rc = hc_iface->write(
	    fun, target, 0, trans->buffer, trans->size, callback_out, trans);

	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_transaction_destroy(trans);
	}
}


/**
 * @}
 */
