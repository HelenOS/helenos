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

static usb_transaction_handle_t g_handle_seed = 1;
static usb_transaction_handle_t create_transaction_handle(int phone)
{
	return g_handle_seed++;
}

typedef struct {
	int phone;
	usb_transaction_handle_t handle;
} transaction_details_t;

/** Callback for outgoing transaction.
 */
static void out_callback(void * buffer, size_t len, usb_transaction_outcome_t outcome, void * arg)
{
	dprintf(2, "out_callback(buffer, %u, %d, %p)", len, outcome, arg);
	
	transaction_details_t * trans = (transaction_details_t *)arg;
	
	async_msg_2(trans->phone, IPC_M_USB_HCD_DATA_SENT, trans->handle, outcome);
	
	free(trans);
	if (buffer) {
		free(buffer);
	}
}

/** Callback for incoming transaction.
 */
static void in_callback(void * buffer, size_t len, usb_transaction_outcome_t outcome, void * arg)
{
	dprintf(2, "in_callback(buffer, %u, %d, %p)", len, outcome, arg);
	transaction_details_t * trans = (transaction_details_t *)arg;
	
	ipc_call_t answer_data;
	ipcarg_t answer_rc;
	aid_t req;
	int rc;
	
	req = async_send_3(trans->phone,
	    IPC_M_USB_HCD_DATA_RECEIVED,
	    trans->handle, outcome,
	    len,
	    &answer_data);
	
	if (len > 0) {
		rc = async_data_write_start(trans->phone, buffer, len);
		if (rc != EOK) {
			async_wait_for(req, NULL);
			goto leave;
		}
	}
	
	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;
	if (rc != EOK) {
		goto leave;
	}
	
leave:
	free(trans);
	if (buffer != NULL) {
		free(buffer);
	}
}

/** Handle data from host to function.
 */
static void handle_data_to_function(ipc_callid_t iid, ipc_call_t icall,
    bool setup_transaction, int callback_phone)
{
	size_t expected_len = IPC_GET_ARG3(icall);
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	
	dprintf(1, "pretending transfer to function (dev=%d:%d)",
	    target.address, target.endpoint);
	
	if (callback_phone == -1) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	usb_transaction_handle_t handle
	    = create_transaction_handle(callback_phone);
	
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
	
	transaction_details_t * trans = malloc(sizeof(transaction_details_t));
	trans->phone = callback_phone;
	trans->handle = handle;
	
	hc_add_transaction_to_device(setup_transaction, target,
	    buffer, len,
	    out_callback, trans);
	
	ipc_answer_1(iid, EOK, handle);
	dprintf(2, "transfer to function scheduled (handle %d)", handle);
}

/** Handle data from function to host.
 */
static void handle_data_from_function(ipc_callid_t iid, ipc_call_t icall, int callback_phone)
{
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	size_t len = IPC_GET_ARG3(icall);
	
	dprintf(1, "pretending transfer from function (dev=%d:%d)",
	    target.address, target.endpoint);
	
	if (callback_phone == -1) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	usb_transaction_handle_t handle
	    = create_transaction_handle(callback_phone);
	
	void * buffer = NULL;
	if (len > 0) {
		buffer = malloc(len);
	}
	
	transaction_details_t * trans = malloc(sizeof(transaction_details_t));
	trans->phone = callback_phone;
	trans->handle = handle;
	
	hc_add_transaction_from_device(target,
	    buffer, len,
	    in_callback, trans);
	
	ipc_answer_1(iid, EOK, handle);
	dprintf(2, "transfer from function scheduled (handle %d)", handle);
}



/** Connection handler for communcation with host.
 * By host is typically meant top-level USB driver.
 *
 * This function also takes care of proper phone hung-up.
 *
 * @param phone_hash Incoming phone hash.
 * @param host_phone Callback phone to host.
 */
void connection_handler_host(ipcarg_t phone_hash, int host_phone)
{
	assert(host_phone > 0);
	
	dprintf(0, "host connected through phone %#x", phone_hash);
	
	
	while (true) {
		ipc_callid_t callid;
		ipc_call_t call;
		
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				ipc_hangup(host_phone);
				ipc_answer_0(callid, EOK);
				dprintf(0, "phone%#x: host hung-up",
				    phone_hash);
				return;
			
			case IPC_M_CONNECT_TO_ME:
				ipc_answer_0(callid, ELIMIT);
				break;
			
			case IPC_M_USB_HCD_TRANSACTION_SIZE:
				ipc_answer_1(callid, EOK, USB_MAX_PAYLOAD_SIZE);
				break;
			
			
			case IPC_M_USB_HCD_INTERRUPT_OUT:
				handle_data_to_function(callid, call,
				    false, host_phone);
				break;
				
			case IPC_M_USB_HCD_INTERRUPT_IN:
				handle_data_from_function(callid, call, host_phone);
				break;
			
			case IPC_M_USB_HCD_CONTROL_WRITE_SETUP:
				handle_data_to_function(callid, call,
				    true, host_phone);
				break;
				
			case IPC_M_USB_HCD_CONTROL_WRITE_DATA:
				handle_data_to_function(callid, call,
				    false, host_phone);
				break;
				
			case IPC_M_USB_HCD_CONTROL_WRITE_STATUS:
				handle_data_from_function(callid, call, host_phone);
				break;
			
			case IPC_M_USB_HCD_CONTROL_READ_SETUP:
				handle_data_to_function(callid, call,
				    true, host_phone);
				break;
				
			case IPC_M_USB_HCD_CONTROL_READ_DATA:
			case IPC_M_USB_HCD_CONTROL_READ_STATUS:
			
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
