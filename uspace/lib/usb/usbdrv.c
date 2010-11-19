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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief USB driver (implementation).
 */
#include "usbdrv.h"
#include <usb_iface.h>
#include <errno.h>

/** Information about pending transaction on HC. */
typedef struct {
	/** Phone to host controller driver. */
	int phone;
	/** Data buffer. */
	void *buffer;
	/** Buffer size. */
	size_t size;
	/** Storage for actual number of bytes transferred. */
	size_t *size_transferred;
	/** Initial call replay data. */
	ipc_call_t reply;
	/** Initial call identifier. */
	aid_t request;
} transfer_info_t;

/** Connect to host controller the device is physically attached to.
 *
 * @param handle Device handle.
 * @param flags Connection flags (blocking connection).
 * @return Phone to corresponding HC or error code.
 */
int usb_drv_hc_connect(device_t *dev, unsigned int flags)
{
	/*
	 * Call parent hub to obtain device handle of respective HC.
	 */
	return ENOTSUP;
}

/** Send data to HCD.
 *
 * @param phone Phone to HC.
 * @param method Method used for calling.
 * @param endpoint Device endpoint.
 * @param buffer Data buffer (NULL to skip data transfer phase).
 * @param size Buffer size (must be zero when @p buffer is NULL).
 * @param handle Storage for transaction handle (cannot be NULL).
 * @return Error status.
 * @retval EINVAL Invalid parameter.
 * @retval ENOMEM Not enough memory to complete the operation.
 */
static int async_send_buffer(int phone, int method,
    usb_endpoint_t endpoint,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	if (phone < 0) {
		return EINVAL;
	}

	if ((buffer == NULL) && (size > 0)) {
		return EINVAL;
	}

	if (handle == NULL) {
		return EINVAL;
	}

	transfer_info_t *transfer
	    = (transfer_info_t *) malloc(sizeof(transfer_info_t));
	if (transfer == NULL) {
		return ENOMEM;
	}

	transfer->size_transferred = NULL;
	transfer->buffer = NULL;
	transfer->size = 0;
	transfer->phone = phone;

	int rc;

	transfer->request = async_send_3(phone,
	    DEV_IFACE_ID(USB_DEV_IFACE),
	    method,
	    endpoint,
	    size,
	    &transfer->reply);

	if (size > 0) {
		rc = async_data_write_start(phone, buffer, size);
		if (rc != EOK) {
			async_wait_for(transfer->request, NULL);
			return rc;
		}
	}

	*handle = (usb_handle_t) transfer;

	return EOK;
}

/** Prepare data retrieval.
 *
 * @param phone Opened phone to HCD.
 * @param method Method used for calling.
 * @param endpoint Device endpoint.
 * @param buffer Buffer where to store retrieved data
 * 	(NULL to skip data transfer phase).
 * @param size Buffer size (must be zero when @p buffer is NULL).
 * @param actual_size Storage where actual number of bytes transferred will
 * 	be stored.
 * @param handle Storage for transaction handle (cannot be NULL).
 * @return Error status.
 * @retval EINVAL Invalid parameter.
 * @retval ENOMEM Not enough memory to complete the operation.
 */
static int async_recv_buffer(int phone, int method,
    usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	if (phone < 0) {
		return EINVAL;
	}

	if ((buffer == NULL) && (size > 0)) {
		return EINVAL;
	}

	if (handle == NULL) {
		return EINVAL;
	}

	transfer_info_t *transfer
	    = (transfer_info_t *) malloc(sizeof(transfer_info_t));
	if (transfer == NULL) {
		return ENOMEM;
	}

	transfer->size_transferred = actual_size;
	transfer->buffer = buffer;
	transfer->size = size;
	transfer->phone = phone;

	transfer->request = async_send_3(phone,
	    DEV_IFACE_ID(USB_DEV_IFACE),
	    method,
	    endpoint,
	    size,
	    &transfer->reply);

	*handle = (usb_handle_t) transfer;

	return EOK;
}

/** Read buffer from HCD.
 *
 * @param phone Opened phone to HCD.
 * @param hash Buffer hash (obtained after completing IN transaction).
 * @param buffer Buffer where to store data data.
 * @param size Buffer size.
 * @param actual_size Storage where actual number of bytes transferred will
 * 	be stored.
 * @return Error status.
 */
static int read_buffer_in(int phone, ipcarg_t hash,
    void *buffer, size_t size, size_t *actual_size)
{
	ipc_call_t answer_data;
	ipcarg_t answer_rc;
	aid_t req;
	int rc;

	req = async_send_2(phone,
	    DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_BUFFER,
	    hash,
	    &answer_data);

	rc = async_data_read_start(phone, buffer, size);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return EINVAL;
	}

	async_wait_for(req, &answer_rc);
	rc = (int)answer_rc;

	if (rc != EOK) {
		return rc;
	}

	*actual_size = IPC_GET_ARG1(answer_data);

	return EOK;
}

/** Blocks caller until given USB transaction is finished.
 * After the transaction is finished, the user can access all output data
 * given to initial call function.
 *
 * @param handle Transaction handle.
 * @return Error status.
 * @retval EOK No error.
 * @retval EBADMEM Invalid handle.
 * @retval ENOENT Data buffer associated with transaction does not exist.
 */
int usb_drv_async_wait_for(usb_handle_t handle)
{
	if (handle == 0) {
		return EBADMEM;
	}

	int rc = EOK;

	transfer_info_t *transfer = (transfer_info_t *) handle;

	ipcarg_t answer_rc;
	async_wait_for(transfer->request, &answer_rc);

	if (answer_rc != EOK) {
		rc = (int) answer_rc;
		goto leave;
	}

	/*
	 * If the buffer is not NULL, we must accept some data.
	 */
	if ((transfer->buffer != NULL) && (transfer->size > 0)) {
		/*
		 * The buffer hash identifies the data on the server
		 * side.
		 * We will use it when actually reading-in the data.
		 */
		ipcarg_t buffer_hash = IPC_GET_ARG1(transfer->reply);
		if (buffer_hash == 0) {
			rc = ENOENT;
			goto leave;
		}

		size_t actual_size;
		rc = read_buffer_in(transfer->phone, buffer_hash,
		    transfer->buffer, transfer->size, &actual_size);

		if (rc != EOK) {
			goto leave;
		}

		if (transfer->size_transferred) {
			*(transfer->size_transferred) = actual_size;
		}
	}

leave:
	free(transfer);

	return rc;
}

/** Send interrupt data to device. */
int usb_drv_async_interrupt_out(int phone, usb_endpoint_t endpoint,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USB_INTERRUPT_OUT,
	    endpoint,
	    buffer, size,
	    handle);
}

/** Request interrupt data from device. */
int usb_drv_async_interrupt_in(int phone, usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return async_recv_buffer(phone,
	    IPC_M_USB_INTERRUPT_IN,
	    endpoint,
	    buffer, size, actual_size,
	    handle);
}

/**
 * @}
 */
