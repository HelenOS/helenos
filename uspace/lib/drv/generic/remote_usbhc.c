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
//static void remote_usb(device_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB interface operations. */
static remote_iface_func_ptr_t remote_usbhc_iface_ops [] = {
	remote_usbhc_get_address,
	remote_usbhc_get_buffer,
	remote_usbhc_interrupt_out,
	remote_usbhc_interrupt_in
};

/** Remote USB interface structure.
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

	devman_handle_t handle = IPC_GET_ARG1(*call);

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
	ipcarg_t buffer_hash = IPC_GET_ARG1(*call);
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
	async_data_read_finalize(callid, trans->buffer, accepted_size);

	ipc_answer_1(callid, EOK, accepted_size);

	free(trans->buffer);
	free(trans);
}


static void callback_out(device_t *device,
    usb_transaction_outcome_t outcome, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	// FIXME - answer according to outcome
	ipc_answer_0(trans->caller, EOK);

	free(trans);
}

static void callback_in(device_t *device,
    usb_transaction_outcome_t outcome, size_t actual_size, void *arg)
{
	async_transaction_t *trans = (async_transaction_t *)arg;

	// FIXME - answer according to outcome
	ipc_answer_1(trans->caller, EOK, (ipcarg_t)trans);

	trans->size = actual_size;
}

void remote_usbhc_interrupt_out(device_t *device, void *iface,
	    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	size_t expected_len = IPC_GET_ARG3(*call);
	usb_target_t target = {
		.address = IPC_GET_ARG1(*call),
		.endpoint = IPC_GET_ARG2(*call)
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

	if (!usb_iface->interrupt_out) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	trans->caller = callid;
	trans->buffer = NULL;
	trans->size = 0;

	int rc = usb_iface->interrupt_out(device, target, buffer, len,
	    callback_out, trans);

	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		free(trans);
	}
}

void remote_usbhc_interrupt_in(device_t *device, void *iface,
	    ipc_callid_t callid, ipc_call_t *call)
{
	usbhc_iface_t *usb_iface = (usbhc_iface_t *) iface;

	size_t len = IPC_GET_ARG3(*call);
	usb_target_t target = {
		.address = IPC_GET_ARG1(*call),
		.endpoint = IPC_GET_ARG2(*call)
	};

	if (!usb_iface->interrupt_in) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}

	async_transaction_t *trans = malloc(sizeof(async_transaction_t));
	trans->caller = callid;
	trans->buffer = malloc(len);
	trans->size = len;

	int rc = usb_iface->interrupt_in(device, target, trans->buffer, len,
	    callback_in, trans);

	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		free(trans->buffer);
		free(trans);
	}
}


/**
 * @}
 */
