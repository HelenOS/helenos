/*
 * Copyright (c) 2010 Vojtech Horky
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

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>

#include "usbhc_iface.h"
#include "driver.h"

#define USB_MAX_PAYLOAD_SIZE 1020

static void remote_usbhc_get_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_get_buffer(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_interrupt_out(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_interrupt_in(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_write_setup(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_write_data(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_write_status(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_read_setup(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_read_data(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_control_read_status(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_reserve_default_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_release_default_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_request_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_bind_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_usbhc_release_address(device_t *, void *, ipc_callid_t, ipc_call_t *);
//static void remote_usbhc(device_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB host controller interface operations. */
static remote_iface_func_ptr_t remote_usbhc_iface_ops [] = {
	remote_usbhc_get_address,

	remote_usbhc_get_buffer,

	remote_usbhc_reserve_default_address,
	remote_usbhc_release_default_address,

	remote_usbhc_request_address,
	remote_usbhc_bind_address,
	remote_usbhc_release_address,

	remote_usbhc_interrupt_out,
	remote_usbhc_interrupt_in,

	remote_usbhc_control_write_setup,
	remote_usbhc_control_write_data,
	remote_usbhc_control_write_status,

	remote_usbhc_control_read_setup,
	remote_usbhc_control_read_data,
	remote_usbhc_control_read_status
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
	void *buffer;
	size_t size;
} async_transaction_t;

void remote_usbhc_get_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->tell_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	devman_handle_t handle = DEV_IPC_GET_ARG1(*call);

	usb_address_t address;
	int rc = usb_iface->tell_address(device, handle, &address);
	if (rc != EOK) {
		ipc_answer_0(callid, rc);
	} else {
		ipc_answer_1(callid, EOK, address);
	}
}

void remote_usbhc_get_buffer(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t buffer_hash = DEV_IPC_GET_ARG1(*call);
	async_transaction_t * trans = (async_transaction_t *)buffer_hash;
	if (trans == NULL) {
		ipc_answer_0(callid, ENOENT);
		return;
	}
	if (trans->buffer == NULL) {
		ipc_answer_0(callid, EINVAL);
		free(trans);
		return;
	}

	ipc_callid_t cid;
	size_t accepted_size;
	if (!async_data_read_receive(&cid, &accepted_size)) {
		ipc_answer_0(callid, EINVAL);
		return;
	}

	if (accepted_size > trans->size) {
		accepted_size = trans->size;
	}
	async_data_read_finalize(cid, trans->buffer, accepted_size);

	ipc_answer_1(callid, EOK, accepted_size);

	free(trans->buffer);
	free(trans);
}

void remote_usbhc_reserve_default_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->reserve_default_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	int rc = usb_iface->reserve_default_address(device);

	ipc_answer_0(callid, rc);
}

void remote_usbhc_release_default_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->release_default_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	int rc = usb_iface->release_default_address(device);

	ipc_answer_0(callid, rc);
}

void remote_usbhc_request_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->request_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address;
	int rc = usb_iface->request_address(device, &address);
	if (rc != EOK) {
		ipc_answer_0(callid, rc);
	} else {
		ipc_answer_1(callid, EOK, (sysarg_t) address);
	}
}

void remote_usbhc_bind_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->bind_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);
	devman_handle_t handle = (devman_handle_t) DEV_IPC_GET_ARG2(*call);

	int rc = usb_iface->bind_address(device, address, handle);

	ipc_answer_0(callid, rc);
}

void remote_usbhc_release_address(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	if (!usb_iface->release_address) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	usb_address_t address = (usb_address_t) DEV_IPC_GET_ARG1(*call);

	int rc = usb_iface->release_address(device, address);

	ipc_answer_0(callid, rc);
}


static void callback_out(device_t *device,
    usb_transaction_outcome_t outcome, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	// FIXME - answer according to outcome
	ipc_answer_0(trans->caller, outcome);

	free(trans);
}

static void callback_in(device_t *device,
    usb_transaction_outcome_t outcome, size_t actual_size, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	// FIXME - answer according to outcome
	ipc_answer_1(trans->caller, outcome, (sysarg_t)trans);

	trans->size = actual_size;
}

/** Process an outgoing transfer (both OUT and SETUP).
 *
 * @param device Target device.
 * @param callid Initiating caller.
 * @param call Initiating call.
 * @param transfer_func Transfer function (might be NULL).
 */
static void remote_usbhc_out_transfer(device_t *device,
    ipc_callid_t callid, ipc_call_t *call,
    usbhc_iface_transfer_out_t transfer_func)
{
	if (!transfer_func) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	size_t expected_len = DEV_IPC_GET_ARG3(*call);
	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	size_t len = 0;
	void *buffer = NULL;
	if (expected_len > 0) {
		int rc = async_data_write_accept(&buffer, false,
		    1, USB_MAX_PAYLOAD_SIZE,
		    0, &len);

		if (rc != EOK) {
			ipc_answer_0(callid, rc);
			return;
		}
	}

	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	trans->caller = callid;
	trans->buffer = buffer;
	trans->size = len;

	int rc = transfer_func(device, target, buffer, len,
	    callback_out, trans);

	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		if (buffer != NULL) {
			free(buffer);
		}
		free(trans);
	}
}

/** Process an incoming transfer.
 *
 * @param device Target device.
 * @param callid Initiating caller.
 * @param call Initiating call.
 * @param transfer_func Transfer function (might be NULL).
 */
static void remote_usbhc_in_transfer(device_t *device,
    ipc_callid_t callid, ipc_call_t *call,
    usbhc_iface_transfer_in_t transfer_func)
{
	if (!transfer_func) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	size_t len = DEV_IPC_GET_ARG3(*call);
	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	trans->caller = callid;
	trans->buffer = malloc(len);
	trans->size = len;

	int rc = transfer_func(device, target, trans->buffer, len,
	    callback_in, trans);

	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		free(trans->buffer);
		free(trans);
	}
}

/** Process status part of control transfer.
 *
 * @param device Target device.
 * @param callid Initiating caller.
 * @param call Initiating call.
 * @param direction Transfer direction (read ~ in, write ~ out).
 * @param transfer_in_func Transfer function for control read (might be NULL).
 * @param transfer_out_func Transfer function for control write (might be NULL).
 */
static void remote_usbhc_status_transfer(device_t *device,
    ipc_callid_t callid, ipc_call_t *call,
    usb_direction_t direction,
    int (*transfer_in_func)(device_t *, usb_target_t,
        usbhc_iface_transfer_in_callback_t, void *),
    int (*transfer_out_func)(device_t *, usb_target_t,
        usbhc_iface_transfer_out_callback_t, void *))
{
	switch (direction) {
		case USB_DIRECTION_IN:
			if (!transfer_in_func) {
				ipc_answer_0(callid, ENOTSUP);
				return;
			}
			break;
		case USB_DIRECTION_OUT:
			if (!transfer_out_func) {
				ipc_answer_0(callid, ENOTSUP);
				return;
			}
			break;
		default:
			assert(false && "unreachable code");
			break;
	}

	usb_target_t target = {
		.address = DEV_IPC_GET_ARG1(*call),
		.endpoint = DEV_IPC_GET_ARG2(*call)
	};

	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	trans->caller = callid;
	trans->buffer = NULL;
	trans->size = 0;

	int rc;
	switch (direction) {
		case USB_DIRECTION_IN:
			rc = transfer_in_func(device, target,
			    callback_in, trans);
			break;
		case USB_DIRECTION_OUT:
			rc = transfer_out_func(device, target,
			    callback_out, trans);
			break;
		default:
			assert(false && "unreachable code");
			break;
	}

	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		free(trans);
	}
	return;
}


void remote_usbhc_interrupt_out(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(device, callid, call,
	    usb_iface->interrupt_out);
}

void remote_usbhc_interrupt_in(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_in_transfer(device, callid, call,
	    usb_iface->interrupt_in);
}

void remote_usbhc_control_write_setup(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(device, callid, call,
	    usb_iface->control_write_setup);
}

void remote_usbhc_control_write_data(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(device, callid, call,
	    usb_iface->control_write_data);
}

void remote_usbhc_control_write_status(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_status_transfer(device, callid, call,
	    USB_DIRECTION_IN, usb_iface->control_write_status, NULL);
}

void remote_usbhc_control_read_setup(device_t *device, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_out_transfer(device, callid, call,
	    usb_iface->control_read_setup);
}

void remote_usbhc_control_read_data(device_t *device, void *iface,
	    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_in_transfer(device, callid, call,
	    usb_iface->control_read_data);
}

void remote_usbhc_control_read_status(device_t *device, void *iface,
	    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;
	assert(usb_iface != NULL);

	return remote_usbhc_status_transfer(device, callid, call,
	    USB_DIRECTION_OUT, NULL, usb_iface->control_read_status);
}



/**
 * @}
 */
