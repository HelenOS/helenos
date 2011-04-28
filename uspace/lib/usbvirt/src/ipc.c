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
 *
 */
#include <errno.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <async.h>
#include <devman.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>

#include <usb/debug.h>

static usbvirt_device_t *DEV = NULL;

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

static void ipc_control_read(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	//usb_endpoint_t endpoint = IPC_GET_ARG1(*icall);

	int rc;

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

static void ipc_control_write(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	size_t data_buffer_len = IPC_GET_ARG2(*icall);
	int rc;

	void *setup_packet = NULL;
	void *data_buffer = NULL;
	size_t setup_packet_len = 0;

	rc = async_data_write_accept(&setup_packet, false,
	    1, 1024, 0, &setup_packet_len);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	if (data_buffer_len > 0) {
		rc = async_data_write_accept(&data_buffer, false,
		    1, 1024, 0, &data_buffer_len);
		if (rc != EOK) {
			async_answer_0(iid, rc);
			free(setup_packet);
			return;
		}
	}

	rc = usbvirt_control_write(dev, setup_packet, setup_packet_len,
	    data_buffer, data_buffer_len);

	async_answer_0(iid, rc);
}

static void ipc_interrupt_in(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	usb_endpoint_t endpoint = IPC_GET_ARG1(*icall);
	usb_transfer_type_t transfer_type = IPC_GET_ARG2(*icall);

	usb_log_debug("ipc_interrupt_in(.%d, %s)\n",
	    endpoint, usb_str_transfer_type_short(transfer_type));

	int rc;

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

static void ipc_interrupt_out(usbvirt_device_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	printf("ipc_interrupt_out()\n");
	async_answer_0(iid, ENOTSUP);
}


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
			ipc_interrupt_in(dev, callid, call);
			break;

		case IPC_M_USBVIRT_INTERRUPT_OUT:
			ipc_interrupt_out(dev, callid, call);
			break;

		default:
			return false;
	}

	return true;
}

static void callback_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	assert(DEV != NULL);

	async_answer_0(iid, EOK);

	while (true) {
		ipc_callid_t callid;
		ipc_call_t call;

		callid = async_get_call(&call);
		bool processed = usbvirt_ipc_handle_call(DEV, callid, &call);
		if (!processed) {
			switch (IPC_GET_IMETHOD(call)) {
				case IPC_M_PHONE_HUNGUP:
					async_answer_0(callid, EOK);
					return;
				default:
					async_answer_0(callid, EINVAL);
					break;
			}
		}
	}
}

int usbvirt_device_plug(usbvirt_device_t *dev, const char *vhc_path)
{
	int rc;
	devman_handle_t handle;

	rc = devman_device_get_handle(vhc_path, &handle, 0);
	if (rc != EOK) {
		return rc;
	}

	int hcd_phone = devman_device_connect(handle, 0);

	if (hcd_phone < 0) {
		return hcd_phone;
	}

	DEV = dev;

	rc = async_connect_to_me(hcd_phone, 0, 0, 0, callback_connection);
	if (rc != EOK) {
		DEV = NULL;
		return rc;
	}



	return EOK;
}



int usbvirt_ipc_send_control_read(int phone, usb_endpoint_t ep,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size, size_t *data_transfered_size)
{
	aid_t opening_request = async_send_1(phone,
	    IPC_M_USBVIRT_CONTROL_READ, ep, NULL);
	if (opening_request == 0) {
		return ENOMEM;
	}

	int rc = async_data_write_start(phone,
	    setup_buffer, setup_buffer_size);
	if (rc != EOK) {
		async_wait_for(opening_request, NULL);
		return rc;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(phone,
	    data_buffer, data_buffer_size,
	    &data_request_call);

	if (data_request == 0) {
		async_wait_for(opening_request, NULL);
		return ENOMEM;
	}

	sysarg_t data_request_rc;
	sysarg_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK) {
			return (int) opening_request_rc;
		} else {
			return (int) data_request_rc;
		}
	}
	if (opening_request_rc != EOK) {
		return (int) opening_request_rc;
	}

	*data_transfered_size = IPC_GET_ARG2(data_request_call);

	return EOK;
}

int usbvirt_ipc_send_control_write(int phone, usb_endpoint_t ep,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size)
{
	aid_t opening_request = async_send_2(phone,
	    IPC_M_USBVIRT_CONTROL_WRITE, ep, data_buffer_size,  NULL);
	if (opening_request == 0) {
		return ENOMEM;
	}

	int rc = async_data_write_start(phone,
	    setup_buffer, setup_buffer_size);
	if (rc != EOK) {
		async_wait_for(opening_request, NULL);
		return rc;
	}

	if (data_buffer_size > 0) {
		rc = async_data_write_start(phone,
		    data_buffer, data_buffer_size);

		if (rc != EOK) {
			async_wait_for(opening_request, NULL);
			return rc;
		}
	}

	sysarg_t opening_request_rc;
	async_wait_for(opening_request, &opening_request_rc);

	return (int) opening_request_rc;
}

int usbvirt_ipc_send_data_in(int phone, usb_endpoint_t ep,
    usb_transfer_type_t tr_type, void *data, size_t data_size, size_t *act_size)
{
	aid_t opening_request = async_send_2(phone,
	    IPC_M_USBVIRT_INTERRUPT_IN, ep, tr_type, NULL);
	if (opening_request == 0) {
		return ENOMEM;
	}

	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(phone,
	    data, data_size,  &data_request_call);

	if (data_request == 0) {
		async_wait_for(opening_request, NULL);
		return ENOMEM;
	}

	sysarg_t data_request_rc;
	sysarg_t opening_request_rc;
	async_wait_for(data_request, &data_request_rc);
	async_wait_for(opening_request, &opening_request_rc);

	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK) {
			return (int) opening_request_rc;
		} else {
			return (int) data_request_rc;
		}
	}
	if (opening_request_rc != EOK) {
		return (int) opening_request_rc;
	}

	if (act_size != NULL) {
		*act_size = IPC_GET_ARG2(data_request_call);
	}

	return EOK;
}

int usbvirt_ipc_send_data_out(int phone, usb_endpoint_t ep,
    usb_transfer_type_t tr_type, void *data, size_t data_size)
{
	return ENOTSUP;
}


/**
 * @}
 */
