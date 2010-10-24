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
 * @brief Main handler for virtual USB device.
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

usbvirt_device_t *device = NULL;

static void handle_setup_transaction(ipc_callid_t iid, ipc_call_t icall)
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


static void handle_out_transaction(ipc_callid_t iid, ipc_call_t icall)
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



static void handle_in_transaction(ipc_callid_t iid, ipc_call_t icall)
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
		if (!async_data_read_receive(&iid, &receive_len)) {
			ipc_answer_0(iid, EINVAL);
			return;
		}
		async_data_read_finalize(iid, buffer, receive_len);
	}
	
	ipc_answer_0(iid, rc);
}



static void callback_connection(ipc_callid_t iid, ipc_call_t *icall)
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
			
			case IPC_M_USBVIRT_TRANSACTION_SETUP:
				handle_setup_transaction(callid, call);
				break;
			
			case IPC_M_USBVIRT_TRANSACTION_OUT:
				handle_out_transaction(callid, call);
				break;
				
			case IPC_M_USBVIRT_TRANSACTION_IN:
				handle_in_transaction(callid, call);
				break;
			
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

static int control_transfer_reply(struct usbvirt_device *device,
	    usb_endpoint_t endpoint, void *buffer, size_t size)
{
	usbvirt_control_transfer_t *transfer = &device->current_control_transfers[endpoint];
	if (transfer->data != NULL) {
		free(transfer->data);
	}
	transfer->data = malloc(size);
	memcpy(transfer->data, buffer, size);
	transfer->data_size = size;
	
	return EOK;
}

static void device_init(usbvirt_device_t *dev)
{
	dev->transaction_out = transaction_out;
	dev->transaction_setup = transaction_setup;
	dev->transaction_in = transaction_in;
	
	dev->control_transfer_reply = control_transfer_reply;
	
	dev->state = USBVIRT_STATE_DEFAULT;
	dev->address = 0;
	
	size_t i;
	for (i = 0; i < USB11_ENDPOINT_MAX; i++) {
		usbvirt_control_transfer_t *transfer = &dev->current_control_transfers[i];
		transfer->direction = 0;
		transfer->request = NULL;
		transfer->request_size = 0;
		transfer->data = NULL;
		transfer->data_size = 0;
	}
}

/** Create necessary phones for comunication with virtual HCD.
 * This function wraps following calls:
 * -# open <code>/dev/usb/<i>hcd_path</i></code> for reading
 * -# access phone of file opened in previous step
 * -# create callback through just opened phone
 * -# create handler for calling on data from host to function
 * -# return the (outgoing) phone
 *
 * @warning This function is wrapper for several actions and therefore
 * it is not possible - in case of error - to determine at which point
 * error occured.
 *
 * @param hcd_path HCD identification under devfs
 *     (without <code>/dev/usb/</code>).
 * @param dev Device to connect.
 * @return EOK on success or error code from errno.h.
 */
int usbvirt_connect(usbvirt_device_t *dev, const char *hcd_path)
{
	char dev_path[DEVMAP_NAME_MAXLEN + 1];
	snprintf(dev_path, DEVMAP_NAME_MAXLEN,
	    "/dev/%s/%s", NAMESPACE, hcd_path);
	
	int fd = open(dev_path, O_RDONLY);
	if (fd < 0) {
		return fd;
	}
	
	int hcd_phone = fd_phone(fd);
	
	if (hcd_phone < 0) {
		return hcd_phone;
	}
	
	ipcarg_t phonehash;
	int rc = ipc_connect_to_me(hcd_phone, 1, dev->device_id_, 0, &phonehash);
	if (rc != EOK) {
		return rc;
	}
	
	dev->vhcd_phone_ = hcd_phone;
	device_init(dev);
	
	device = dev;
	
	async_new_connection(phonehash, 0, NULL, callback_connection);
	
	return EOK;
}

/** Prepares device as local.
 * This is useful if you want to have a virtual device in the same task
 * as HCD.
 *
 * @param dev Device to connect.
 * @return Always EOK.
 */
int usbvirt_connect_local(usbvirt_device_t *dev)
{
	dev->vhcd_phone_ = -1;
	device_init(dev);
	
	device = dev;
	
	return EOK;
}

/** Disconnects device from HCD.
 *
 * @return Always EOK.
 */
int usbvirt_disconnect(void)
{
	ipc_hangup(device->vhcd_phone_);
	
	device = NULL;
	
	return EOK;
}


/**
 * @}
 */
