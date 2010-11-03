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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Connection handling of calls from host (implementation).
 */
#include <assert.h>
#include <errno.h>
#include <usb/hcd.h>

#include "vhcd.h"
#include "conn.h"
#include "hc.h"

typedef struct {
	ipc_callid_t caller;
	void *buffer;
	size_t size;
} async_transaction_t;

static void async_out_callback(void * buffer, size_t len,
    usb_transaction_outcome_t outcome, void * arg)
{
	async_transaction_t * trans = (async_transaction_t *)arg;
	
	dprintf(2, "async_out_callback(buffer, %u, %d, %p) -> %x",
	    len, outcome, arg, trans->caller);
	
	// FIXME - answer according to outcome
	ipc_answer_1(trans->caller, EOK, 0);
	
	free(trans);
	if (buffer) {
		free(buffer);
	}
	dprintf(4, "async_out_callback answered");
}

static void async_to_device(ipc_callid_t iid, ipc_call_t icall, bool setup_transaction)
{
	size_t expected_len = IPC_GET_ARG3(icall);
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	
	dprintf(1, "async_to_device: dev=%d:%d, size=%d, iid=%x",
	    target.address, target.endpoint, expected_len, iid);
	
	size_t len = 0;
	void * buffer = NULL;
	if (expected_len > 0) {
		int rc = async_data_write_accept(&buffer, false,
		    1, USB_MAX_PAYLOAD_SIZE,
		    0, &len);
		
		if (rc != EOK) {
			ipc_answer_0(iid, rc);
			return;
		}
	}
	
	async_transaction_t * trans = malloc(sizeof(async_transaction_t));
	trans->caller = iid;
	trans->buffer = NULL;
	trans->size = 0;
	
	hc_add_transaction_to_device(setup_transaction, target,
	    buffer, len,
	    async_out_callback, trans);
	
	dprintf(2, "async transaction to device scheduled (%p)", trans);
}

static void async_in_callback(void * buffer, size_t len,
    usb_transaction_outcome_t outcome, void * arg)
{	
	async_transaction_t * trans = (async_transaction_t *)arg;
	
	dprintf(2, "async_in_callback(buffer, %u, %d, %p) -> %x",
	    len, outcome, arg, trans->caller);
	
	trans->buffer = buffer;
	trans->size = len;
	
	ipc_callid_t caller = trans->caller;
	
	if (buffer == NULL) {
		free(trans);
		trans = NULL;
	}
	
	
	// FIXME - answer according to outcome
	ipc_answer_1(caller, EOK, (ipcarg_t)trans);
	dprintf(4, "async_in_callback answered (%#x)", (ipcarg_t)trans);
}

static void async_from_device(ipc_callid_t iid, ipc_call_t icall)
{
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	size_t len = IPC_GET_ARG3(icall);
	
	dprintf(1, "async_from_device: dev=%d:%d, size=%d, iid=%x",
	    target.address, target.endpoint, len, iid);
	
	void * buffer = NULL;
	if (len > 0) {
		buffer = malloc(len);
	}
	
	async_transaction_t * trans = malloc(sizeof(async_transaction_t));
	trans->caller = iid;
	trans->buffer = NULL;
	trans->size = 0;
	
	hc_add_transaction_from_device(target,
	    buffer, len,
	    async_in_callback, trans);
	
	dprintf(2, "async transfer from device scheduled (%p)", trans);
}

static void async_get_buffer(ipc_callid_t iid, ipc_call_t icall)
{
	ipcarg_t buffer_hash = IPC_GET_ARG1(icall);
	async_transaction_t * trans = (async_transaction_t *)buffer_hash;
	if (trans == NULL) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	if (trans->buffer == NULL) {
		ipc_answer_0(iid, EINVAL);
		free(trans);
		return;
	}
	
	ipc_callid_t callid;
	size_t accepted_size;
	if (!async_data_read_receive(&callid, &accepted_size)) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	if (accepted_size > trans->size) {
		accepted_size = trans->size;
	}
	async_data_read_finalize(callid, trans->buffer, accepted_size);
	
	ipc_answer_1(iid, EOK, accepted_size);
	
	free(trans->buffer);
	free(trans);
}


/** Connection handler for communcation with host.
 * By host is typically meant top-level USB driver.
 *
 * This function also takes care of proper phone hung-up.
 *
 * @param phone_hash Incoming phone hash.
 */
void connection_handler_host(ipcarg_t phone_hash)
{
	dprintf(0, "host connected through phone %#x", phone_hash);
	
	
	while (true) {
		ipc_callid_t callid;
		ipc_call_t call;
		
		callid = async_get_call(&call);
		
		dprintf(6, "host on %#x calls [%x: %u (%u, %u, %u, %u, %u)]",
		    phone_hash,
		    callid,
		    IPC_GET_METHOD(call),
		    IPC_GET_ARG1(call), IPC_GET_ARG2(call), IPC_GET_ARG3(call),
		    IPC_GET_ARG4(call), IPC_GET_ARG5(call));
		
		switch (IPC_GET_METHOD(call)) {

			/* standard IPC methods */

			case IPC_M_PHONE_HUNGUP:
				ipc_answer_0(callid, EOK);
				dprintf(0, "phone%#x: host hung-up",
				    phone_hash);
				return;
			
			case IPC_M_CONNECT_TO_ME:
				ipc_answer_0(callid, ELIMIT);
				break;
			

			/* USB methods */

			case IPC_M_USB_HCD_TRANSACTION_SIZE:
				ipc_answer_1(callid, EOK, USB_MAX_PAYLOAD_SIZE);
				break;
			
			case IPC_M_USB_HCD_GET_BUFFER_ASYNC:
				async_get_buffer(callid, call);
				break;
	
			case IPC_M_USB_HCD_INTERRUPT_OUT_ASYNC:
			case IPC_M_USB_HCD_CONTROL_WRITE_DATA_ASYNC:
			case IPC_M_USB_HCD_CONTROL_READ_STATUS_ASYNC:
				async_to_device(callid, call, false);
				break;
			
			case IPC_M_USB_HCD_CONTROL_WRITE_SETUP_ASYNC:
			case IPC_M_USB_HCD_CONTROL_READ_SETUP_ASYNC:
				async_to_device(callid, call, true);
				break;
			
			case IPC_M_USB_HCD_INTERRUPT_IN_ASYNC:
			case IPC_M_USB_HCD_CONTROL_WRITE_STATUS_ASYNC:
			case IPC_M_USB_HCD_CONTROL_READ_DATA_ASYNC:
				async_from_device(callid, call);
				break;
			

			/* end of known methods */
			
			default:
				dprintf_inval_call(2, call, phone_hash);
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

/**
 * @}
 */
