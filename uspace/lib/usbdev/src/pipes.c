/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty, Michal Staruch
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
 * USB endpoint pipes functions.
 */
#include <usb/dev/pipes.h>
#include <usb/dev/request.h>
#include <usb/usb.h>
#include <usb/dma_buffer.h>

#include <assert.h>
#include <bitops.h>
#include <async.h>
#include <as.h>
#include <errno.h>
#include <mem.h>

/** Try to clear endpoint halt of default control pipe.
 *
 * @param pipe Pipe for control endpoint zero.
 */
static void clear_self_endpoint_halt(usb_pipe_t *pipe)
{
	assert(pipe != NULL);

	if (!pipe->auto_reset_halt || (pipe->desc.endpoint_no != 0)) {
		return;
	}

	/* Prevent infinite recursion. */
	pipe->auto_reset_halt = false;
	usb_pipe_clear_halt(pipe, pipe);
	pipe->auto_reset_halt = true;
}

/* Helper structure to avoid passing loads of arguments through */
typedef struct {
	usb_pipe_t *pipe;
	usb_direction_t dir;
	bool is_control;	// Only for checking purposes

	usbhc_iface_transfer_request_t req;

	size_t transferred_size;
} transfer_t;

/**
 * Issue a transfer in a separate exchange.
 */
static errno_t transfer_common(transfer_t *t)
{
	if (!t->pipe)
		return EBADMEM;

	/* Only control writes make sense without buffer */
	if ((t->dir != USB_DIRECTION_OUT || !t->is_control) && t->req.size == 0)
		return EINVAL;

	/* Nonzero size requires buffer */
	if (!dma_buffer_is_set(&t->req.buffer) && t->req.size != 0)
		return EINVAL;

	/* Check expected direction */
	if (t->pipe->desc.direction != USB_DIRECTION_BOTH &&
	    t->pipe->desc.direction != t->dir)
		return EBADF;

	/* Check expected transfer type */
	if ((t->pipe->desc.transfer_type == USB_TRANSFER_CONTROL) != t->is_control)
		return EBADF;

	async_exch_t *exch = async_exchange_begin(t->pipe->bus_session);
	if (!exch)
		return ENOMEM;

	t->req.dir = t->dir;
	t->req.endpoint = t->pipe->desc.endpoint_no;

	const errno_t rc = usbhc_transfer(exch, &t->req, &t->transferred_size);

	async_exchange_end(exch);

	if (rc == ESTALL)
		clear_self_endpoint_halt(t->pipe);

	return rc;
}

/**
 * Setup the transfer request inside transfer according to dma buffer provided.
 *
 * TODO: The buffer could have been allocated as a more strict one. Currently,
 * we assume that the policy is just the requested one.
 */
static void setup_dma_buffer(transfer_t *t, void *base, void *ptr, size_t size)
{
	t->req.buffer.virt = base;
	t->req.buffer.policy = t->pipe->desc.transfer_buffer_policy;
	t->req.offset = ptr - base;
	t->req.size = size;
}

/**
 * Compatibility wrapper for reads/writes without preallocated buffer.
 */
static errno_t transfer_wrap_dma(transfer_t *t, void *buf, size_t size)
{
	if (size == 0) {
		setup_dma_buffer(t, NULL, NULL, 0);
		return transfer_common(t);
	}

	void *dma_buf = usb_pipe_alloc_buffer(t->pipe, size);
	setup_dma_buffer(t, dma_buf, dma_buf, size);

	if (t->dir == USB_DIRECTION_OUT)
		memcpy(dma_buf, buf, size);

	const errno_t err = transfer_common(t);

	if (!err && t->dir == USB_DIRECTION_IN)
		memcpy(buf, dma_buf, t->transferred_size);

	usb_pipe_free_buffer(t->pipe, dma_buf);
	return err;
}

static errno_t prepare_control(transfer_t *t, const void *setup, size_t setup_size)
{
	if ((setup == NULL) || (setup_size != 8))
		return EINVAL;

	memcpy(&t->req.setup, setup, 8);
	return EOK;
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
 * @param[out] data_transferred_size Number of bytes that were actually
 *                                  transferred during the DATA stage.
 * @return Error code.
 */
errno_t usb_pipe_control_read(usb_pipe_t *pipe,
    const void *setup_buffer, size_t setup_buffer_size,
    void *buffer, size_t buffer_size, size_t *transferred_size)
{
	errno_t err;
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_IN,
		.is_control = true,
	};

	if ((err = prepare_control(&transfer, setup_buffer, setup_buffer_size)))
		return err;

	if ((err = transfer_wrap_dma(&transfer, buffer, buffer_size)))
		return err;

	if (transferred_size)
		*transferred_size = transfer.transferred_size;

	return EOK;
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
errno_t usb_pipe_control_write(usb_pipe_t *pipe,
    const void *setup_buffer, size_t setup_buffer_size,
    const void *buffer, size_t buffer_size)
{
	assert(pipe);
	errno_t err;
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_OUT,
		.is_control = true,
	};

	if ((err = prepare_control(&transfer, setup_buffer, setup_buffer_size)))
		return err;

	return transfer_wrap_dma(&transfer, (void *) buffer, buffer_size);
}

/**
 * Allocate a buffer for data transmission, that satisfies the constraints
 * imposed by the host controller.
 *
 * @param[in] pipe Pipe for which the buffer is allocated
 * @param[in] size Size of the required buffer
 */
void *usb_pipe_alloc_buffer(usb_pipe_t *pipe, size_t size)
{
	dma_buffer_t buf;
	if (dma_buffer_alloc_policy(&buf, size, pipe->desc.transfer_buffer_policy))
		return NULL;

	return buf.virt;
}

void usb_pipe_free_buffer(usb_pipe_t *pipe, void *buffer)
{
	dma_buffer_t buf;
	buf.virt = buffer;
	dma_buffer_free(&buf);
}

/** Request a read (in) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[out] buffer Buffer where to store the data.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transferred Number of bytes that were actually transferred.
 * @return Error code.
 */
errno_t usb_pipe_read(usb_pipe_t *pipe,
    void *buffer, size_t size, size_t *size_transferred)
{
	assert(pipe);
	errno_t err;
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_IN,
	};

	if ((err = transfer_wrap_dma(&transfer, buffer, size)))
		return err;

	if (size_transferred)
		*size_transferred = transfer.transferred_size;

	return EOK;
}

/** Request a write (out) transfer on an endpoint pipe.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer with data to transfer.
 * @param[in] size Size of the buffer (in bytes).
 * @return Error code.
 */
errno_t usb_pipe_write(usb_pipe_t *pipe, const void *buffer, size_t size)
{
	assert(pipe);
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_OUT,
	};

	return transfer_wrap_dma(&transfer, (void *) buffer, size);
}

/**
 * Request a read (in) transfer on an endpoint pipe, declaring that buffer
 * is pointing to a memory area previously allocated by usb_pipe_alloc_buffer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer, previously allocated with usb_pipe_alloc_buffer.
 * @param[in] size Size of the buffer (in bytes).
 * @param[out] size_transferred Number of bytes that were actually transferred.
 * @return Error code.
 */
errno_t usb_pipe_read_dma(usb_pipe_t *pipe, void *base, void *ptr, size_t size,
    size_t *size_transferred)
{
	assert(pipe);
	errno_t err;
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_IN,
	};

	setup_dma_buffer(&transfer, base, ptr, size);

	if ((err = transfer_common(&transfer)))
		return err;

	if (size_transferred)
		*size_transferred = transfer.transferred_size;

	return EOK;
}

/**
 * Request a write (out) transfer on an endpoint pipe, declaring that buffer
 * is pointing to a memory area previously allocated by usb_pipe_alloc_buffer.
 *
 * @param[in] pipe Pipe used for the transfer.
 * @param[in] buffer Buffer, previously allocated with usb_pipe_alloc_buffer.
 * @param[in] size Size of the buffer (in bytes).
 * @return Error code.
 */
errno_t usb_pipe_write_dma(usb_pipe_t *pipe, void *base, void *ptr, size_t size)
{
	assert(pipe);
	transfer_t transfer = {
		.pipe = pipe,
		.dir = USB_DIRECTION_OUT,
	};

	setup_dma_buffer(&transfer, base, ptr, size);

	return transfer_common(&transfer);
}

/** Initialize USB endpoint pipe.
 *
 * @param pipe Endpoint pipe to be initialized.
 * @param bus_session Endpoint pipe to be initialized.
 * @return Error code.
 */
errno_t usb_pipe_initialize(usb_pipe_t *pipe, usb_dev_session_t *bus_session)
{
	assert(pipe);

	pipe->auto_reset_halt = false;
	pipe->bus_session = bus_session;

	return EOK;
}

static const usb_pipe_desc_t default_control_pipe = {
	.endpoint_no = 0,
	.transfer_type = USB_TRANSFER_CONTROL,
	.direction = USB_DIRECTION_BOTH,
	.max_transfer_size = CTRL_PIPE_MIN_PACKET_SIZE,
	.transfer_buffer_policy = DMA_POLICY_STRICT,
};

/** Initialize USB default control pipe.
 *
 * This one is special because it must not be registered, it is registered
 * automatically.
 *
 * @param pipe Endpoint pipe to be initialized.
 * @param bus_session Endpoint pipe to be initialized.
 * @return Error code.
 */
errno_t usb_pipe_initialize_default_control(usb_pipe_t *pipe,
    usb_dev_session_t *bus_session)
{
	const errno_t ret = usb_pipe_initialize(pipe, bus_session);
	if (ret)
		return ret;

	pipe->desc = default_control_pipe;
	pipe->auto_reset_halt = true;

	return EOK;
}

/** Register endpoint with the host controller.
 *
 * @param pipe Pipe to be registered.
 * @param ep_desc Matched endpoint descriptor
 * @param comp_desc Matched superspeed companion descriptro, if any
 * @return Error code.
 */
errno_t usb_pipe_register(usb_pipe_t *pipe,
    const usb_standard_endpoint_descriptor_t *ep_desc,
    const usb_superspeed_endpoint_companion_descriptor_t *comp_desc)
{
	assert(pipe);
	assert(pipe->bus_session);
	assert(ep_desc);

	async_exch_t *exch = async_exchange_begin(pipe->bus_session);
	if (!exch)
		return ENOMEM;

	usb_endpoint_descriptors_t descriptors = { 0 };

#define COPY(field) descriptors.endpoint.field = ep_desc->field
	COPY(endpoint_address);
	COPY(attributes);
	COPY(max_packet_size);
	COPY(poll_interval);
#undef COPY

#define COPY(field) descriptors.companion.field = comp_desc->field
	if (comp_desc) {
		COPY(max_burst);
		COPY(attributes);
		COPY(bytes_per_interval);
	}
#undef COPY

	const errno_t ret = usbhc_register_endpoint(exch,
	    &pipe->desc, &descriptors);
	async_exchange_end(exch);
	return ret;
}

/** Revert endpoint registration with the host controller.
 *
 * @param pipe Pipe to be unregistered.
 * @return Error code.
 */
errno_t usb_pipe_unregister(usb_pipe_t *pipe)
{
	assert(pipe);
	assert(pipe->bus_session);
	async_exch_t *exch = async_exchange_begin(pipe->bus_session);
	if (!exch)
		return ENOMEM;

	const errno_t ret = usbhc_unregister_endpoint(exch, &pipe->desc);

	async_exchange_end(exch);
	return ret;
}

/**
 * @}
 */
