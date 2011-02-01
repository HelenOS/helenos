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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Communication between device drivers and host controller driver.
 *
 * Note on synchronousness of the operations: there is ABSOLUTELY NO
 * guarantee that a call to particular function will not trigger a fibril
 * switch.
 * The initialization functions may actually involve contacting some other
 * task, starting/ending a session might involve asynchronous IPC and since
 * the transfer functions uses IPC, asynchronous nature of them is obvious.
 * The pseudo synchronous versions for the transfers internally call the
 * asynchronous ones and so fibril switch is possible in them as well.
 */
#include <usb/usb.h>
#include <usb/pipes.h>
#include <errno.h>
#include <assert.h>
#include <usb/usbdrv.h>

#define _PREPARE_TARGET(varname, pipe) \
	usb_target_t varname = { \
		.address = (pipe)->wire->address, \
		.endpoint = (pipe)->endpoint_no \
	}

/** Initialize connection to USB device.
 *
 * @param connection Connection structure to be initialized.
 * @param device Generic device backing the USB device.
 * @return Error code.
 */
int usb_device_connection_initialize_from_device(
    usb_device_connection_t *connection, device_t *device)
{
	assert(connection);
	assert(device);

	int rc;
	devman_handle_t hc_handle;
	usb_address_t my_address;

	rc = usb_drv_find_hc(device, &hc_handle);
	if (rc != EOK) {
		return rc;
	}

	int hc_phone = devman_device_connect(hc_handle, 0);
	if (hc_phone < 0) {
		return hc_phone;
	}

	my_address = usb_drv_get_my_address(hc_phone, device);
	if (my_address < 0) {
		rc = my_address;
		goto leave;
	}

	rc = usb_device_connection_initialize(connection,
	    hc_handle, my_address);

leave:
	ipc_hangup(hc_phone);
	return rc;
}

/** Initialize connection to USB device.
 *
 * @param connection Connection structure to be initialized.
 * @param host_controller_handle Devman handle of host controller device is
 * 	connected to.
 * @param device_address Device USB address.
 * @return Error code.
 */
int usb_device_connection_initialize(usb_device_connection_t *connection,
    devman_handle_t host_controller_handle, usb_address_t device_address)
{
	assert(connection);

	if ((device_address < 0) || (device_address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}

	connection->hc_handle = host_controller_handle;
	connection->address = device_address;

	return EOK;
}

/** Initialize USB endpoint pipe.
 *
 * @param pipe Endpoint pipe to be initialized.
 * @param connection Connection to the USB device backing this pipe (the wire).
 * @param endpoint_no Endpoint number (in USB 1.1 in range 0 to 15).
 * @param transfer_type Transfer type (e.g. interrupt or bulk).
 * @param direction Endpoint direction (in/out).
 * @return Error code.
 */
int usb_endpoint_pipe_initialize(usb_endpoint_pipe_t *pipe,
    usb_device_connection_t *connection, usb_endpoint_t endpoint_no,
    usb_transfer_type_t transfer_type, usb_direction_t direction)
{
	assert(pipe);
	assert(connection);

	pipe->wire = connection;
	pipe->hc_phone = -1;
	pipe->endpoint_no = endpoint_no;
	pipe->transfer_type = transfer_type;
	pipe->direction = direction;

	return EOK;
}


/** Initialize USB endpoint pipe as the default zero control pipe.
 *
 * @param pipe Endpoint pipe to be initialized.
 * @param connection Connection to the USB device backing this pipe (the wire).
 * @return Error code.
 */
int usb_endpoint_pipe_initialize_default_control(usb_endpoint_pipe_t *pipe,
    usb_device_connection_t *connection)
{
	assert(pipe);
	assert(connection);

	int rc = usb_endpoint_pipe_initialize(pipe, connection,
	    0, USB_TRANSFER_CONTROL, USB_DIRECTION_BOTH);

	return rc;
}


/** Start a session on the endpoint pipe.
 *
 * A session is something inside what any communication occurs.
 * It is expected that sessions would be started right before the transfer
 * and ended - see usb_endpoint_pipe_end_session() - after the last
 * transfer.
 * The reason for this is that session actually opens some communication
 * channel to the host controller (or to the physical hardware if you
 * wish) and thus it involves acquiring kernel resources.
 * Since they are limited, sessions shall not be longer than strictly
 * necessary.
 *
 * @param pipe Endpoint pipe to start the session on.
 * @return Error code.
 */
int usb_endpoint_pipe_start_session(usb_endpoint_pipe_t *pipe)
{
	assert(pipe);

	if (pipe->hc_phone >= 0) {
		return EBUSY;
	}

	int phone = devman_device_connect(pipe->wire->hc_handle, 0);
	if (phone < 0) {
		return phone;
	}

	pipe->hc_phone = phone;

	return EOK;
}


/** Ends a session on the endpoint pipe.
 *
 * @see usb_endpoint_pipe_start_session
 *
 * @param pipe Endpoint pipe to end the session on.
 * @return Error code.
 */
int usb_endpoint_pipe_end_session(usb_endpoint_pipe_t *pipe)
{
	assert(pipe);

	if (pipe->hc_phone < 0) {
		return ENOENT;
	}

	int rc = ipc_hangup(pipe->hc_phone);
	if (rc != EOK) {
		return rc;
	}

	pipe->hc_phone = -1;

	return EOK;
}


/** Request a read (in) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[out] buffer Buffer where to store the data.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transfered Number of bytes that were actually transfered.
 * @return Error code.
 */
int usb_endpoint_pipe_read(usb_endpoint_pipe_t *pipe,
    void *buffer, size_t size, size_t *size_transfered)
{
	assert(pipe);

	int rc;
	usb_handle_t handle;

	rc = usb_endpoint_pipe_async_read(pipe, buffer, size, size_transfered,
	    &handle);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_endpoint_pipe_wait_for(pipe, handle);
	return rc;
}

/** Request a write (out) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer with data to transfer.
 * @param[in] size Size of the buffer (in bytes).
 * @return Error code.
 */
int usb_endpoint_pipe_write(usb_endpoint_pipe_t *pipe,
    void *buffer, size_t size)
{
	assert(pipe);

	int rc;
	usb_handle_t handle;

	rc = usb_endpoint_pipe_async_write(pipe, buffer, size, &handle);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_endpoint_pipe_wait_for(pipe, handle);
	return rc;
}


/** Request a control read transfer on an endpoint pipe.
 *
 * This function encapsulates all three stages of a control transfer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] setup_buffer Buffer with the setup packet.
 * @param[in] setup_buffer_size Size of the setup packet (in bytes).
 * @param[out] data_buffer Buffer for incoming data.
 * @param[in] data_buffer_size Size of the buffer for incoming data (in bytes).
 * @param[out] data_transfered_size Number of bytes that were actually
 *                                  transfered during the DATA stage.
 * @return Error code.
 */
int usb_endpoint_pipe_control_read(usb_endpoint_pipe_t *pipe,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size, size_t *data_transfered_size)
{
	assert(pipe);

	int rc;
	usb_handle_t handle;

	rc = usb_endpoint_pipe_async_control_read(pipe,
	    setup_buffer, setup_buffer_size,
	    data_buffer, data_buffer_size, data_transfered_size,
	    &handle);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_endpoint_pipe_wait_for(pipe, handle);
	return rc;
}


/** Request a control write transfer on an endpoint pipe.
 *
 * This function encapsulates all three stages of a control transfer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] setup_buffer Buffer with the setup packet.
 * @param[in] setup_buffer_size Size of the setup packet (in bytes).
 * @param[in] data_buffer Buffer with data to be sent.
 * @param[in] data_buffer_size Size of the buffer with outgoing data (in bytes).
 * @return Error code.
 */
int usb_endpoint_pipe_control_write(usb_endpoint_pipe_t *pipe,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size)
{
	assert(pipe);

	int rc;
	usb_handle_t handle;

	rc = usb_endpoint_pipe_async_control_write(pipe,
	    setup_buffer, setup_buffer_size,
	    data_buffer, data_buffer_size,
	    &handle);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_endpoint_pipe_wait_for(pipe, handle);
	return rc;
}


/** Request a read (in) transfer on an endpoint pipe (asynchronous version).
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[out] buffer Buffer where to store the data.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transfered Number of bytes that were actually transfered.
 * @param[out] handle Handle of the transfer.
 * @return Error code.
 */
int usb_endpoint_pipe_async_read(usb_endpoint_pipe_t *pipe,
    void *buffer, size_t size, size_t *size_transfered,
    usb_handle_t *handle)
{
	assert(pipe);

	if (pipe->hc_phone < 0) {
		return EBADF;
	}

	if (pipe->direction != USB_DIRECTION_IN) {
		return EBADF;
	}

	int rc;
	_PREPARE_TARGET(target, pipe);

	switch (pipe->transfer_type) {
		case USB_TRANSFER_INTERRUPT:
			rc = usb_drv_async_interrupt_in(pipe->hc_phone, target,
			    buffer, size, size_transfered, handle);
			break;
		case USB_TRANSFER_CONTROL:
			rc = EBADF;
			break;
		default:
			rc = ENOTSUP;
			break;
	}

	return rc;
}


/** Request a write (out) transfer on an endpoint pipe (asynchronous version).
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer with data to transfer.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] handle Handle of the transfer.
 * @return Error code.
 */
int usb_endpoint_pipe_async_write(usb_endpoint_pipe_t *pipe,
    void *buffer, size_t size,
    usb_handle_t *handle)
{
	assert(pipe);

	if (pipe->hc_phone < 0) {
		return EBADF;
	}

	if (pipe->direction != USB_DIRECTION_OUT) {
		return EBADF;
	}

	int rc;
	_PREPARE_TARGET(target, pipe);

	switch (pipe->transfer_type) {
		case USB_TRANSFER_INTERRUPT:
			rc = usb_drv_async_interrupt_out(pipe->hc_phone, target,
			    buffer, size, handle);
			break;
		case USB_TRANSFER_CONTROL:
			rc = EBADF;
			break;
		default:
			rc = ENOTSUP;
			break;
	}

	return rc;
}


/** Request a control read transfer on an endpoint pipe (asynchronous version).
 *
 * This function encapsulates all three stages of a control transfer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] setup_buffer Buffer with the setup packet.
 * @param[in] setup_buffer_size Size of the setup packet (in bytes).
 * @param[out] data_buffer Buffer for incoming data.
 * @param[in] data_buffer_size Size of the buffer for incoming data (in bytes).
 * @param[out] data_transfered_size Number of bytes that were actually
 *                                  transfered during the DATA stage.
 * @param[out] handle Handle of the transfer.
 * @return Error code.
 */
int usb_endpoint_pipe_async_control_read(usb_endpoint_pipe_t *pipe,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size, size_t *data_transfered_size,
    usb_handle_t *handle)
{
	assert(pipe);

	if (pipe->hc_phone < 0) {
		return EBADF;
	}

	if ((pipe->direction != USB_DIRECTION_BOTH)
	    || (pipe->transfer_type != USB_TRANSFER_CONTROL)) {
		return EBADF;
	}

	int rc;
	_PREPARE_TARGET(target, pipe);

	rc = usb_drv_async_control_read(pipe->hc_phone, target,
	    setup_buffer, setup_buffer_size,
	    data_buffer, data_buffer_size, data_transfered_size,
	    handle);

	return rc;
}


/** Request a control write transfer on an endpoint pipe (asynchronous version).
 *
 * This function encapsulates all three stages of a control transfer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] setup_buffer Buffer with the setup packet.
 * @param[in] setup_buffer_size Size of the setup packet (in bytes).
 * @param[in] data_buffer Buffer with data to be sent.
 * @param[in] data_buffer_size Size of the buffer with outgoing data (in bytes).
 * @param[out] handle Handle of the transfer.
 * @return Error code.
 */
int usb_endpoint_pipe_async_control_write(usb_endpoint_pipe_t *pipe,
    void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size,
    usb_handle_t *handle)
{
	assert(pipe);

	if (pipe->hc_phone < 0) {
		return EBADF;
	}

	if ((pipe->direction != USB_DIRECTION_BOTH)
	    || (pipe->transfer_type != USB_TRANSFER_CONTROL)) {
		return EBADF;
	}

	int rc;
	_PREPARE_TARGET(target, pipe);

	rc = usb_drv_async_control_write(pipe->hc_phone, target,
	    setup_buffer, setup_buffer_size,
	    data_buffer, data_buffer_size,
	    handle);

	return rc;
}

/** Wait for transfer completion.
 *
 * The function blocks the caller fibril until the transfer associated
 * with given @p handle is completed.
 *
 * @param[in] pipe Pipe the transfer executed on.
 * @param[in] handle Transfer handle.
 * @return Error code.
 */
int usb_endpoint_pipe_wait_for(usb_endpoint_pipe_t *pipe, usb_handle_t handle)
{
	return usb_drv_async_wait_for(handle);
}


/**
 * @}
 */
