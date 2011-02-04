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

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB driver (implementation).
 */
#include <usb/usbdrv.h>
#include <usbhc_iface.h>
#include <usb_iface.h>
#include <errno.h>
#include <str_error.h>

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

/** Find handle of host controller the device is physically attached to.
 *
 * @param[in] dev Device looking for its host controller.
 * @param[out] handle Host controller devman handle.
 * @return Error code.
 */
int usb_drv_find_hc(device_t *dev, devman_handle_t *handle)
{
	if (dev == NULL) {
		return EBADMEM;
	}
	if (handle == NULL) {
		return EBADMEM;
	}

	int parent_phone = devman_parent_device_connect(dev->handle,
	    IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	devman_handle_t h;
	int rc = async_req_1_1(parent_phone, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_HOST_CONTROLLER_HANDLE, &h);

	ipc_hangup(parent_phone);

	if (rc != EOK) {
		return rc;
	}

	*handle = h;

	return EOK;
}

/** Connect to host controller the device is physically attached to.
 *
 * @param dev Device asking for connection.
 * @param hc_handle Devman handle of the host controller.
 * @param flags Connection flags (blocking connection).
 * @return Phone to the HC or error code.
 */
int usb_drv_hc_connect(device_t *dev, devman_handle_t hc_handle,
    unsigned int flags)
{
	return devman_device_connect(hc_handle, flags);
}

/** Connect to host controller the device is physically attached to.
 *
 * @param dev Device asking for connection.
 * @param flags Connection flags (blocking connection).
 * @return Phone to corresponding HC or error code.
 */
int usb_drv_hc_connect_auto(device_t *dev, unsigned int flags)
{
	int rc;
	devman_handle_t hc_handle;

	/*
	 * Call parent hub to obtain device handle of respective HC.
	 */
	rc = usb_drv_find_hc(dev, &hc_handle);
	if (rc != EOK) {
		return rc;
	}
	
	return usb_drv_hc_connect(dev, hc_handle, flags);
}

/** Tell USB address assigned to given device.
 *
 * @param phone Phone to my HC.
 * @param dev Device in question.
 * @return USB address or error code.
 */
usb_address_t usb_drv_get_my_address(int phone, device_t *dev)
{
	sysarg_t address;
	int rc = async_req_2_1(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_GET_ADDRESS,
	    dev->handle, &address);

	if (rc != EOK) {
		printf("usb_drv_get_my_address over %d failed: %s\n", phone, str_error(rc));
		return rc;
	}

	return (usb_address_t) address;
}

/** Tell HC to reserve default address.
 *
 * @param phone Open phone to host controller driver.
 * @return Error code.
 */
int usb_drv_reserve_default_address(int phone)
{
	return async_req_1_0(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RESERVE_DEFAULT_ADDRESS);
}

/** Tell HC to release default address.
 *
 * @param phone Open phone to host controller driver.
 * @return Error code.
 */
int usb_drv_release_default_address(int phone)
{
	return async_req_1_0(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RELEASE_DEFAULT_ADDRESS);
}

/** Ask HC for free address assignment.
 *
 * @param phone Open phone to host controller driver.
 * @return Assigned USB address or negative error code.
 */
usb_address_t usb_drv_request_address(int phone)
{
	sysarg_t address;
	int rc = async_req_1_1(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_REQUEST_ADDRESS, &address);
	if (rc != EOK) {
		return rc;
	} else {
		return (usb_address_t) address;
	}
}

/** Inform HC about binding address with devman handle.
 *
 * @param phone Open phone to host controller driver.
 * @param address Address to be binded.
 * @param handle Devman handle of the device.
 * @return Error code.
 */
int usb_drv_bind_address(int phone, usb_address_t address,
    devman_handle_t handle)
{
	int rc = async_req_3_0(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_BIND_ADDRESS,
	    address, handle);

	return rc;
}

/** Inform HC about address release.
 *
 * @param phone Open phone to host controller driver.
 * @param address Address to be released.
 * @return Error code.
 */
int usb_drv_release_address(int phone, usb_address_t address)
{
	return async_req_2_0(phone, DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RELEASE_ADDRESS, address);
}

/** Send data to HCD.
 *
 * @param phone Phone to HC.
 * @param method Method used for calling.
 * @param target Targeted device.
 * @param buffer Data buffer (NULL to skip data transfer phase).
 * @param size Buffer size (must be zero when @p buffer is NULL).
 * @param handle Storage for transaction handle (cannot be NULL).
 * @return Error status.
 * @retval EINVAL Invalid parameter.
 * @retval ENOMEM Not enough memory to complete the operation.
 */
static int async_send_buffer(int phone, int method,
    usb_target_t target,
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

	transfer->request = async_send_4(phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    method,
	    target.address, target.endpoint,
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
 * @param target Targeted device.
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
    usb_target_t target,
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

	transfer->request = async_send_4(phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    method,
	    target.address, target.endpoint,
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
static int read_buffer_in(int phone, sysarg_t hash,
    void *buffer, size_t size, size_t *actual_size)
{
	ipc_call_t answer_data;
	sysarg_t answer_rc;
	aid_t req;
	int rc;

	req = async_send_2(phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_GET_BUFFER,
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

	sysarg_t answer_rc;
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
		sysarg_t buffer_hash = IPC_GET_ARG1(transfer->reply);
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
int usb_drv_async_interrupt_out(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USBHC_INTERRUPT_OUT,
	    target,
	    buffer, size,
	    handle);
}

/** Request interrupt data from device. */
int usb_drv_async_interrupt_in(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return async_recv_buffer(phone,
	    IPC_M_USBHC_INTERRUPT_IN,
	    target,
	    buffer, size, actual_size,
	    handle);
}

/** Start control write transfer. */
int usb_drv_async_control_write_setup(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USBHC_CONTROL_WRITE_SETUP,
	    target,
	    buffer, size,
	    handle);
}

/** Send data during control write transfer. */
int usb_drv_async_control_write_data(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USBHC_CONTROL_WRITE_DATA,
	    target,
	    buffer, size,
	    handle);
}

/** Finalize control write transfer. */
int usb_drv_async_control_write_status(int phone, usb_target_t target,
    usb_handle_t *handle)
{
	return async_recv_buffer(phone,
	    IPC_M_USBHC_CONTROL_WRITE_STATUS,
	    target,
	    NULL, 0, NULL,
	    handle);
}

/** Issue whole control write transfer. */
int usb_drv_async_control_write(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *buffer, size_t buffer_size,
    usb_handle_t *handle)
{
	// FIXME - check input parameters instead of asserting them
	assert(phone > 0);
	assert(setup_packet != NULL);
	assert(setup_packet_size > 0);
	assert(buffer != NULL);
	assert(buffer_size > 0);
	assert(handle != NULL);

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
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_CONTROL_WRITE,
	    target.address, target.endpoint,
	    &transfer->reply);

	rc = async_data_write_start(phone, setup_packet, setup_packet_size);
	if (rc != EOK) {
		async_wait_for(transfer->request, NULL);
		return rc;
	}

	rc = async_data_write_start(phone, buffer, buffer_size);
	if (rc != EOK) {
		async_wait_for(transfer->request, NULL);
		return rc;
	}

	*handle = (usb_handle_t) transfer;

	return EOK;
}

/** Start control read transfer. */
int usb_drv_async_control_read_setup(int phone, usb_target_t target,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USBHC_CONTROL_READ_SETUP,
	    target,
	    buffer, size,
	    handle);
}

/** Read data during control read transfer. */
int usb_drv_async_control_read_data(int phone, usb_target_t target,
    void *buffer, size_t size, size_t *actual_size,
    usb_handle_t *handle)
{
	return async_recv_buffer(phone,
	    IPC_M_USBHC_CONTROL_READ_DATA,
	    target,
	    buffer, size, actual_size,
	    handle);
}

/** Finalize control read transfer. */
int usb_drv_async_control_read_status(int phone, usb_target_t target,
    usb_handle_t *handle)
{
	return async_send_buffer(phone,
	    IPC_M_USBHC_CONTROL_READ_STATUS,
	    target,
	    NULL, 0,
	    handle);
}

/** Issue whole control read transfer. */
int usb_drv_async_control_read(int phone, usb_target_t target,
    void *setup_packet, size_t setup_packet_size,
    void *buffer, size_t buffer_size, size_t *actual_size,
    usb_handle_t *handle)
{
	// FIXME - check input parameters instead of asserting them
	assert(phone > 0);
	assert(setup_packet != NULL);
	assert(setup_packet_size > 0);
	assert(buffer != NULL);
	assert(buffer_size > 0);
	assert(handle != NULL);

	transfer_info_t *transfer
	    = (transfer_info_t *) malloc(sizeof(transfer_info_t));
	if (transfer == NULL) {
		return ENOMEM;
	}

	transfer->size_transferred = actual_size;
	transfer->buffer = buffer;
	transfer->size = buffer_size;
	transfer->phone = phone;

	int rc;

	transfer->request = async_send_4(phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_CONTROL_READ,
	    target.address, target.endpoint,
	    buffer_size,
	    &transfer->reply);

	rc = async_data_write_start(phone, setup_packet, setup_packet_size);
	if (rc != EOK) {
		async_wait_for(transfer->request, NULL);
		return rc;
	}

	*handle = (usb_handle_t) transfer;

	return EOK;
}

/**
 * @}
 */
