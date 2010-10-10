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
 * @brief Virtual host controller driver.
 */

#include <devmap.h>
#include <ipc/ipc.h>
#include <async.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>

#include <usb/hcd.h>
#include <usb/virtdev.h>
#include "vhcd.h"
#include "hc.h"
#include "devices.h"

static usb_transaction_handle_t g_handle_seed = 1;
static usb_transaction_handle_t create_transaction_handle(int phone)
{
	return g_handle_seed++;
}

typedef struct {
	int phone;
	usb_transaction_handle_t handle;
} transaction_details_t;

static void out_callback(void * buffer, size_t len, usb_transaction_outcome_t outcome, void * arg)
{
	dprintf("out_callback(buffer, %u, %d, %p)", len, outcome, arg);
	(void)len;
	transaction_details_t * trans = (transaction_details_t *)arg;
	
	async_msg_2(trans->phone, IPC_M_USB_HCD_DATA_SENT, trans->handle, outcome);
	
	free(trans);
	free(buffer);
}

static void in_callback(void * buffer, size_t len, usb_transaction_outcome_t outcome, void * arg)
{
	dprintf("in_callback(buffer, %u, %d, %p)", len, outcome, arg);
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
	
	rc = async_data_write_start(trans->phone, buffer, len);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		goto leave;
	}
	
	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;
	if (rc != EOK) {
		goto leave;
	}
	
leave:
	free(trans);
	free(buffer);
}



static void handle_data_to_function(ipc_callid_t iid, ipc_call_t icall, int callback_phone)
{
	usb_transfer_type_t transf_type = IPC_GET_ARG3(icall);
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	
	dprintf("pretending transfer to function (dev=%d:%d, type=%s)",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transf_type));
	
	if (callback_phone == -1) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	usb_transaction_handle_t handle
	    = create_transaction_handle(callback_phone);
	
	size_t len;
	void * buffer;
	int rc = async_data_write_accept(&buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE,
	    0, &len);
	
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return;
	}
	
	transaction_details_t * trans = malloc(sizeof(transaction_details_t));
	trans->phone = callback_phone;
	trans->handle = handle;
	
	dprintf("adding transaction to HC", NAME);
	hc_add_transaction_to_device(transf_type, target,
	    buffer, len,
	    out_callback, trans);
	
	ipc_answer_1(iid, EOK, handle);
	dprintf("transfer to function scheduled (handle %d)", handle);
}

static void handle_data_from_function(ipc_callid_t iid, ipc_call_t icall, int callback_phone)
{
	usb_transfer_type_t transf_type = IPC_GET_ARG3(icall);
	usb_target_t target = {
		.address = IPC_GET_ARG1(icall),
		.endpoint = IPC_GET_ARG2(icall)
	};
	size_t len = IPC_GET_ARG4(icall);
	
	dprintf("pretending transfer from function (dev=%d:%d, type=%s)",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transf_type));
	
	if (callback_phone == -1) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	usb_transaction_handle_t handle
	    = create_transaction_handle(callback_phone);
	
	void * buffer = malloc(len);
	
	transaction_details_t * trans = malloc(sizeof(transaction_details_t));
	trans->phone = callback_phone;
	trans->handle = handle;
	
	dprintf("adding transaction to HC", NAME);
	hc_add_transaction_from_device(transf_type, target,
	    buffer, len,
	    in_callback, trans);
	
	ipc_answer_1(iid, EOK, handle);
	dprintf("transfer from function scheduled (handle %d)", handle);
}

static void handle_data_from_device(ipc_callid_t iid, ipc_call_t icall, virtdev_connection_t *dev)
{
	usb_endpoint_t endpoint = IPC_GET_ARG1(icall);
	usb_target_t target = {
		.address = dev->address,
		.endpoint = endpoint
	};
	size_t len;
	void * buffer;
	int rc = async_data_write_accept(&buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE,
	    0, &len);
	
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return;
	}
	
	rc = hc_fillin_transaction_from_device(USB_TRANSFER_INTERRUPT, target, buffer, len);
	
	ipc_answer_0(iid, rc);
}

static virtdev_connection_t *recognise_device(int id, int callback_phone)
{
	switch (id) {
		case USB_VIRTDEV_KEYBOARD_ID:
			return virtdev_add_device(
			    USB_VIRTDEV_KEYBOARD_ADDRESS, callback_phone);
		default:
			return NULL;
	}		
}

static void device_client_connection(int callback_phone, int device_id)
{
	virtdev_connection_t *dev = recognise_device(device_id, callback_phone);
	if (!dev) {
		if (callback_phone != -1) {
			ipc_hangup(callback_phone);
		}
		return;
	}
	dprintf("device %d connected", device_id);
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				if (callback_phone != -1) {
					ipc_hangup(callback_phone);
				}
				ipc_answer_0(callid, EOK);
				dprintf("hung-up on device %d", device_id);
				virtdev_destroy_device(dev);
				return;
			case IPC_M_CONNECT_TO_ME:
				ipc_answer_0(callid, ELIMIT);
				break;
			case IPC_M_USB_VIRTDEV_DATA_FROM_DEVICE:
				dprintf("data from device %d", device_id);
				handle_data_from_device(callid, call, dev);
				break;
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipcarg_t phone_hash = icall->in_phone_hash;
	
	ipc_answer_0(iid, EOK);
	printf("%s: new client connected (phone %#x).\n", NAME, phone_hash);
	
	int callback_phone = -1;
	
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		
		callid = async_get_call(&call);
		dprintf("%#x calls method %d", phone_hash, IPC_GET_METHOD(call));
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				if (callback_phone != -1) {
					ipc_hangup(callback_phone);
				}
				ipc_answer_0(callid, EOK);
				printf("%s: hung-up on %#x.\n", NAME, phone_hash);
				return;
			
			case IPC_M_CONNECT_TO_ME:
				if (callback_phone != -1) {
					ipc_answer_0(callid, ELIMIT);
				} else {
					callback_phone = IPC_GET_ARG5(call);
					ipc_answer_0(callid, EOK);
				}
				if (IPC_GET_ARG1(call) == 1) {
					/* Virtual device was just connected
					 * to us. This is handled elsewhere.
					 */
					device_client_connection(callback_phone,
					    IPC_GET_ARG2(call));
					return;
				}
				break;
			
			case IPC_M_USB_HCD_SEND_DATA:
				handle_data_to_function(callid, call, callback_phone);
				break;
			
			case IPC_M_USB_HCD_RECEIVE_DATA:
				handle_data_from_function(callid, call, callback_phone);
				break;
			
			case IPC_M_USB_HCD_TRANSACTION_SIZE:
				ipc_answer_1(callid, EOK, USB_MAX_PAYLOAD_SIZE);
				break;
			
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

int main(int argc, char * argv[])
{
	printf("%s: Virtual USB host controller driver.\n", NAME);
	
	int rc;
	
	rc = devmap_driver_register(NAME, client_connection); 
	if (rc != EOK) {
		printf("%s: unable to register driver (%s).\n",
		    NAME, str_error(rc));
		return 1;
	}

	rc = devmap_device_register(DEVMAP_PATH, NULL);
	if (rc != EOK) {
		printf("%s: unable to register device %s (%s).\n",
		    NAME, DEVMAP_PATH, str_error(rc));
		return 1;
	}
	
	printf("%s: accepting connections.\n", NAME);
	hc_manager();
	
	return 0;
}


/**
 * @}
 */
