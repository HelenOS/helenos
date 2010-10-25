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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Callback connection handling.
 */
#include <devmap.h>
#include <fcntl.h>
#include <vfs/vfs.h>
#include <errno.h>
#include <stdlib.h>
#include <mem.h>

#include "hub.h"
#include "device.h"
#include "private.h"

#define NAMESPACE "usb"

/** Wrapper for SETUP transaction over telephone. */
static void handle_setup_transaction(usbvirt_device_t *device,
    ipc_callid_t iid, ipc_call_t icall)
{
	usb_address_t address = IPC_GET_ARG1(icall);
	usb_endpoint_t endpoint = IPC_GET_ARG2(icall);
	size_t expected_len = IPC_GET_ARG3(icall);
	
	if (address != device->address) {
		ipc_answer_0(iid, EADDRNOTAVAIL);
		return;
	}
	
	if ((endpoint < 0) || (endpoint >= USB11_ENDPOINT_MAX)) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	if (expected_len == 0) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	size_t len = 0;
	void * buffer = NULL;
	int rc = async_data_write_accept(&buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE, 0, &len);
		
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return;
	}
	
	rc = device->transaction_setup(device, endpoint, buffer, len);
	
	ipc_answer_0(iid, rc);
}

/** Wrapper for OUT transaction over telephone. */
static void handle_out_transaction(usbvirt_device_t *device,
    ipc_callid_t iid, ipc_call_t icall)
{
	usb_address_t address = IPC_GET_ARG1(icall);
	usb_endpoint_t endpoint = IPC_GET_ARG2(icall);
	size_t expected_len = IPC_GET_ARG3(icall);
	
	if (address != device->address) {
		ipc_answer_0(iid, EADDRNOTAVAIL);
		return;
	}
	
	if ((endpoint < 0) || (endpoint >= USB11_ENDPOINT_MAX)) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = EOK;
	
	size_t len = 0;
	void *buffer = NULL;
	
	if (expected_len > 0) {
		rc = async_data_write_accept(&buffer, false,
		    1, USB_MAX_PAYLOAD_SIZE, 0, &len);
			
		if (rc != EOK) {
			ipc_answer_0(iid, rc);
			return;
		}
	}
	
	rc = device->transaction_out(device, endpoint, buffer, len);
	
	if (buffer != NULL) {
		free(buffer);
	}
	
	ipc_answer_0(iid, rc);
}


/** Wrapper for IN transaction over telephone. */
static void handle_in_transaction(usbvirt_device_t *device,
    ipc_callid_t iid, ipc_call_t icall)
{
	usb_address_t address = IPC_GET_ARG1(icall);
	usb_endpoint_t endpoint = IPC_GET_ARG2(icall);
	size_t expected_len = IPC_GET_ARG3(icall);
	
	if (address != device->address) {
		ipc_answer_0(iid, EADDRNOTAVAIL);
		return;
	}
	
	if ((endpoint < 0) || (endpoint >= USB11_ENDPOINT_MAX)) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = EOK;
	
	void *buffer = expected_len > 0 ? malloc(expected_len) : NULL;
	size_t len;
	
	rc = device->transaction_in(device, endpoint, buffer, expected_len, &len);
	/*
	 * If the request was processed, we will send data back.
	 */
	if (rc == EOK) {
		size_t receive_len;
		ipc_callid_t callid;
		if (!async_data_read_receive(&callid, &receive_len)) {
			ipc_answer_0(iid, EINVAL);
			return;
		}
		async_data_read_finalize(callid, buffer, receive_len);
	}
	
	ipc_answer_0(iid, rc);
}

/** Wrapper for getting device name. */
static void handle_get_name(usbvirt_device_t *device,
    ipc_callid_t iid, ipc_call_t icall)
{
	if (device->name == NULL) {
		ipc_answer_0(iid, ENOENT);
	}
	
	size_t size = str_size(device->name);
	
	ipc_callid_t callid;
	size_t accepted_size;
	if (!async_data_read_receive(&callid, &accepted_size)) {
		ipc_answer_0(iid, EINVAL);
		return;
	}
	
	if (accepted_size > size) {
		accepted_size = size;
	}
	async_data_read_finalize(callid, device->name, accepted_size);
	
	ipc_answer_1(iid, EOK, accepted_size);
}

/** Callback connection for a given device. */
void device_callback_connection(usbvirt_device_t *device, ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_answer_0(iid, EOK);
	
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				ipc_answer_0(callid, EOK);
				return;
			
			case IPC_M_USBVIRT_GET_NAME:
				handle_get_name(device, callid, call);
				break;
			
			case IPC_M_USBVIRT_TRANSACTION_SETUP:
				handle_setup_transaction(device, callid, call);
				break;
			
			case IPC_M_USBVIRT_TRANSACTION_OUT:
				handle_out_transaction(device, callid, call);
				break;
				
			case IPC_M_USBVIRT_TRANSACTION_IN:
				handle_in_transaction(device, callid, call);
				break;
			
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}


/**
 * @}
 */
