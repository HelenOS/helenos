/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * IPC wrappers, device side.
 */
#include <errno.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <async.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>
#include <usb/debug.h>

/** Handle VHC request for device name.
 *
 * @param dev Target virtual device.
 * @param iid Caller id.
 * @param icall The call with the request.
 */
static void ipc_get_name(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	if (dev->name == NULL) {
		async_answer_0(iid, ENOENT);
	}

	size_t size = str_size(dev->name);

	ipc_callid_t callid;
	size_t accepted_size;
	if (!async_data_read_receive(&callid, &accepted_size)) {
		async_answer_0(iid, EINVAL);
		return;
	}

	if (accepted_size > size) {
		accepted_size = size;
	}
	async_data_read_finalize(callid, dev->name, accepted_size);

	async_answer_1(iid, EOK, accepted_size);
}

/** Handle VHC request for control read from the device.
 *
 * @param dev Target virtual device.
 * @param iid Caller id.
 * @param icall The call with the request.
 */
static void ipc_control_read(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	errno_t rc;

	void *setup_packet = NULL;
	size_t setup_packet_len = 0;
	size_t data_len = 0;

	rc = async_data_write_accept(&setup_packet, false,
	    1, 1024, 0, &setup_packet_len);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EPARTY);
		free(setup_packet);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(iid, ENOMEM);
		free(setup_packet);
		return;
	}

	size_t actual_len;
	rc = usbvirt_control_read(dev, setup_packet, setup_packet_len,
	    buffer, data_len, &actual_len);

	if (rc != EOK) {
		async_answer_0(data_callid, rc);
		async_answer_0(iid, rc);
		free(setup_packet);
		free(buffer);
		return;
	}

	async_data_read_finalize(data_callid, buffer, actual_len);
	async_answer_0(iid, EOK);

	free(setup_packet);
	free(buffer);
}

/** Handle VHC request for control write to the device.
 *
 * @param dev Target virtual device.
 * @param iid Caller id.
 * @param icall The call with the request.
 */
static void ipc_control_write(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	size_t data_buffer_len = IPC_GET_ARG1(*icall);
	errno_t rc;

	void *setup_packet = NULL;
	void *data_buffer = NULL;
	size_t setup_packet_len = 0;

	rc = async_data_write_accept(&setup_packet, false,
	    1, 0, 0, &setup_packet_len);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	if (data_buffer_len > 0) {
		rc = async_data_write_accept(&data_buffer, false,
		    1, 0, 0, &data_buffer_len);
		if (rc != EOK) {
			async_answer_0(iid, rc);
			free(setup_packet);
			return;
		}
	}

	rc = usbvirt_control_write(dev, setup_packet, setup_packet_len,
	    data_buffer, data_buffer_len);

	async_answer_0(iid, rc);

	free(setup_packet);
	if (data_buffer != NULL) {
		free(data_buffer);
	}
}

/** Handle VHC request for data read from the device (in transfer).
 *
 * @param dev Target virtual device.
 * @param iid Caller id.
 * @param icall The call with the request.
 */
static void ipc_data_in(usbvirt_device_t *dev,
    usb_transfer_type_t transfer_type,
    ipc_callid_t iid, ipc_call_t *icall)
{
	usb_endpoint_t endpoint = IPC_GET_ARG1(*icall);

	errno_t rc;

	size_t data_len = 0;
	ipc_callid_t data_callid;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EPARTY);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	size_t actual_len;
	rc = usbvirt_data_in(dev, transfer_type, endpoint,
	    buffer, data_len, &actual_len);

	if (rc != EOK) {
		async_answer_0(data_callid, rc);
		async_answer_0(iid, rc);
		free(buffer);
		return;
	}

	async_data_read_finalize(data_callid, buffer, actual_len);
	async_answer_0(iid, EOK);

	free(buffer);
}

/** Handle VHC request for data write to the device (out transfer).
 *
 * @param dev Target virtual device.
 * @param iid Caller id.
 * @param icall The call with the request.
 */
static void ipc_data_out(usbvirt_device_t *dev,
    usb_transfer_type_t transfer_type,
    ipc_callid_t iid, ipc_call_t *icall)
{
	usb_endpoint_t endpoint = IPC_GET_ARG1(*icall);

	void *data_buffer = NULL;
	size_t data_buffer_size = 0;

	errno_t rc = async_data_write_accept(&data_buffer, false,
	    1, 0, 0, &data_buffer_size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	rc = usbvirt_data_out(dev, transfer_type, endpoint,
	    data_buffer, data_buffer_size);

	async_answer_0(iid, rc);

	free(data_buffer);
}

/** Handle incoming IPC call for virtual USB device.
 *
 * @param dev Target USB device.
 * @param callid Caller id.
 * @param call Incoming call.
 * @return Whether the call was handled.
 */
bool usbvirt_ipc_handle_call(usbvirt_device_t *dev,
    ipc_callid_t callid, ipc_call_t *call)
{
	switch (IPC_GET_IMETHOD(*call)) {
	case IPC_M_USBVIRT_GET_NAME:
		ipc_get_name(dev, callid, call);
		break;

	case IPC_M_USBVIRT_CONTROL_READ:
		ipc_control_read(dev, callid, call);
		break;

	case IPC_M_USBVIRT_CONTROL_WRITE:
		ipc_control_write(dev, callid, call);
		break;

	case IPC_M_USBVIRT_INTERRUPT_IN:
		ipc_data_in(dev, USB_TRANSFER_INTERRUPT, callid, call);
		break;

	case IPC_M_USBVIRT_BULK_IN:
		ipc_data_in(dev, USB_TRANSFER_BULK, callid, call);
		break;

	case IPC_M_USBVIRT_INTERRUPT_OUT:
		ipc_data_out(dev, USB_TRANSFER_INTERRUPT, callid, call);
		break;

	case IPC_M_USBVIRT_BULK_OUT:
		ipc_data_out(dev, USB_TRANSFER_BULK, callid, call);
		break;


	default:
		return false;
	}

	return true;
}

/**
 * @}
 */
