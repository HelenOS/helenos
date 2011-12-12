/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB endpoint pipes miscellaneous functions.
 */
#include <usb/dev/pipes.h>
#include <usb/dev/request.h>
#include <errno.h>
#include <assert.h>

/** Prepare pipe for a long transfer.
 *
 * By a long transfer is mean transfer consisting of several
 * requests to the HC.
 * Calling such function is optional and it has positive effect of
 * improved performance because IPC session is initiated only once.
 *
 * @param pipe Pipe over which the transfer will happen.
 * @return Error code.
 */
int usb_pipe_start_long_transfer(usb_pipe_t *pipe)
{
	assert(pipe);
	assert(pipe->wire);
	assert(pipe->wire->hc_connection);
	return usb_hc_connection_open(pipe->wire->hc_connection);
}
/*----------------------------------------------------------------------------*/
/** Terminate a long transfer on a pipe.
 *
 * @see usb_pipe_start_long_transfer
 *
 * @param pipe Pipe where to end the long transfer.
 */
int usb_pipe_end_long_transfer(usb_pipe_t *pipe)
{
	assert(pipe);
	assert(pipe->wire);
	assert(pipe->wire->hc_connection);
	return usb_hc_connection_close(pipe->wire->hc_connection);
}
/*----------------------------------------------------------------------------*/
/** Request an in transfer, no checking of input parameters.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[out] buffer Buffer where to store the data.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transfered Number of bytes that were actually transfered.
 * @return Error code.
 */
static int usb_pipe_read_no_check(usb_pipe_t *pipe, uint64_t setup,
    void *buffer, size_t size, size_t *size_transfered)
{
	/* Isochronous transfer are not supported (yet) */
	if (pipe->transfer_type != USB_TRANSFER_INTERRUPT &&
	    pipe->transfer_type != USB_TRANSFER_BULK &&
	    pipe->transfer_type != USB_TRANSFER_CONTROL)
	    return ENOTSUP;

	return usb_hc_control_read(pipe->wire->hc_connection,
	    pipe->wire->address, pipe->endpoint_no, setup, buffer, size,
	    size_transfered);
}
/*----------------------------------------------------------------------------*/
/** Request an out transfer, no checking of input parameters.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer with data to transfer.
 * @param[in] size Size of the buffer (in bytes).
 * @return Error code.
 */
static int usb_pipe_write_no_check(usb_pipe_t *pipe, uint64_t setup,
    const void *buffer, size_t size)
{
	/* Only interrupt and bulk transfers are supported */
	if (pipe->transfer_type != USB_TRANSFER_INTERRUPT &&
	    pipe->transfer_type != USB_TRANSFER_BULK &&
	    pipe->transfer_type != USB_TRANSFER_CONTROL)
	    return ENOTSUP;

	return usb_hc_control_write(pipe->wire->hc_connection,
	    pipe->wire->address, pipe->endpoint_no, setup, buffer, size);
}
/*----------------------------------------------------------------------------*/
/** Try to clear endpoint halt of default control pipe.
 *
 * @param pipe Pipe for control endpoint zero.
 */
static void clear_self_endpoint_halt(usb_pipe_t *pipe)
{
	assert(pipe != NULL);

	if (!pipe->auto_reset_halt || (pipe->endpoint_no != 0)) {
		return;
	}


	/* Prevent infinite recursion. */
	pipe->auto_reset_halt = false;
	usb_request_clear_endpoint_halt(pipe, 0);
	pipe->auto_reset_halt = true;
}
/*----------------------------------------------------------------------------*/
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
int usb_pipe_control_read(usb_pipe_t *pipe,
    const void *setup_buffer, size_t setup_buffer_size,
    void *data_buffer, size_t data_buffer_size, size_t *data_transfered_size)
{
	assert(pipe);

	if ((setup_buffer == NULL) || (setup_buffer_size != 8)) {
		return EINVAL;
	}

	if ((data_buffer == NULL) || (data_buffer_size == 0)) {
		return EINVAL;
	}

	if ((pipe->direction != USB_DIRECTION_BOTH)
	    || (pipe->transfer_type != USB_TRANSFER_CONTROL)) {
		return EBADF;
	}

	uint64_t setup_packet;
	memcpy(&setup_packet, setup_buffer, 8);

	size_t act_size = 0;
	const int rc = usb_pipe_read_no_check(pipe, setup_packet,
	    data_buffer, data_buffer_size, &act_size);

	if (rc == ESTALL) {
		clear_self_endpoint_halt(pipe);
	}

	if (rc == EOK && data_transfered_size != NULL) {
		*data_transfered_size = act_size;
	}

	return rc;
}
/*----------------------------------------------------------------------------*/
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
int usb_pipe_control_write(usb_pipe_t *pipe,
    const void *setup_buffer, size_t setup_buffer_size,
    const void *data_buffer, size_t data_buffer_size)
{
	assert(pipe);

	if ((setup_buffer == NULL) || (setup_buffer_size != 8)) {
		return EINVAL;
	}

	if ((data_buffer == NULL) && (data_buffer_size > 0)) {
		return EINVAL;
	}

	if ((data_buffer != NULL) && (data_buffer_size == 0)) {
		return EINVAL;
	}

	if ((pipe->direction != USB_DIRECTION_BOTH)
	    || (pipe->transfer_type != USB_TRANSFER_CONTROL)) {
		return EBADF;
	}

	uint64_t setup_packet;
	memcpy(&setup_packet, setup_buffer, 8);

	const int rc = usb_pipe_write_no_check(pipe, setup_packet,
	    data_buffer, data_buffer_size);

	if (rc == ESTALL) {
		clear_self_endpoint_halt(pipe);
	}

	return rc;
}
/*----------------------------------------------------------------------------*/
/** Request a read (in) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[out] buffer Buffer where to store the data.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transfered Number of bytes that were actually transfered.
 * @return Error code.
 */
int usb_pipe_read(usb_pipe_t *pipe,
    void *buffer, size_t size, size_t *size_transfered)
{
	assert(pipe);

	if (buffer == NULL) {
		return EINVAL;
	}

	if (size == 0) {
		return EINVAL;
	}

	if (pipe->direction != USB_DIRECTION_IN) {
		return EBADF;
	}

	if (pipe->transfer_type == USB_TRANSFER_CONTROL) {
		return EBADF;
	}

	size_t act_size = 0;
	const int rc = usb_pipe_read_no_check(pipe, 0, buffer, size, &act_size);


	if (rc == EOK && size_transfered != NULL) {
		*size_transfered = act_size;
	}

	return rc;
}
/*----------------------------------------------------------------------------*/
/** Request a write (out) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer with data to transfer.
 * @param[in] size Size of the buffer (in bytes).
 * @return Error code.
 */
int usb_pipe_write(usb_pipe_t *pipe, const void *buffer, size_t size)
{
	assert(pipe);

	if (buffer == NULL || size == 0) {
		return EINVAL;
	}

	if (pipe->direction != USB_DIRECTION_OUT) {
		return EBADF;
	}

	if (pipe->transfer_type == USB_TRANSFER_CONTROL) {
		return EBADF;
	}

	return usb_pipe_write_no_check(pipe, 0, buffer, size);
}
/**
 * @}
 */
