/*
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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * USB transfer transaction structures (implementation).
 */

#include <usb/host/usb_transfer_batch.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <usbhc_iface.h>

/** Allocate and initialize usb_transfer_batch structure.
 * @param ep endpoint used by the transfer batch.
 * @param buffer data to send/recieve.
 * @param buffer_size Size of data buffer.
 * @param setup_buffer Data to send in SETUP stage of control transfer.
 * @param func_in callback on IN transfer completion.
 * @param func_out callback on OUT transfer completion.
 * @param fun DDF function (passed to callback function).
 * @param arg Argument to pass to the callback function.
 * @param private_data driver specific per batch data.
 * @param private_data_dtor Function to properly destroy private_data.
 * @return Pointer to valid usb_transfer_batch_t structure, NULL on failure.
 */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *ep, char *buffer,
    size_t buffer_size,
    uint64_t setup_buffer,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg)
{
	if (func_in == NULL && func_out == NULL)
		return NULL;
	if (func_in != NULL && func_out != NULL)
		return NULL;

	usb_transfer_batch_t *instance = malloc(sizeof(usb_transfer_batch_t));
	if (instance) {
		instance->ep = ep;
		instance->callback_in = func_in;
		instance->callback_out = func_out;
		instance->arg = arg;
		instance->buffer = buffer;
		instance->buffer_size = buffer_size;
		instance->setup_size = 0;
		instance->transfered_size = 0;
		instance->error = EOK;
		if (ep && ep->transfer_type == USB_TRANSFER_CONTROL) {
			memcpy(instance->setup_buffer, &setup_buffer,
			    USB_SETUP_PACKET_SIZE);
			instance->setup_size = USB_SETUP_PACKET_SIZE;
		}
		if (instance->ep)
			endpoint_use(instance->ep);
	}
	return instance;
}

/** Correctly dispose all used data structures.
 *
 * @param[in] instance Batch structure to use.
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *instance)
{
	if (!instance)
		return;
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " disposing.\n",
	    instance, USB_TRANSFER_BATCH_ARGS(*instance));
	if (instance->ep) {
		endpoint_release(instance->ep);
	}
	free(instance);
}

/** Prepare data and call the right callback.
 *
 * @param[in] instance Batch structure to use.
 * @param[in] data Data to copy to the output buffer.
 * @param[in] size Size of @p data.
 * @param[in] error Error value to use.
 */
void usb_transfer_batch_finish_error(const usb_transfer_batch_t *instance,
    const void *data, size_t size, errno_t error)
{
	assert(instance);
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " finishing.\n",
	    instance, USB_TRANSFER_BATCH_ARGS(*instance));

	/* NOTE: Only one of these pointers should be set. */
        if (instance->callback_out) {
		instance->callback_out(error, instance->arg);
	}

        if (instance->callback_in) {
		/* We care about the data and there are some to copy */
		const size_t safe_size = min(size, instance->buffer_size);
		if (data) {
	                memcpy(instance->buffer, data, safe_size);
		}
		instance->callback_in(error, safe_size, instance->arg);
	}
}
/**
 * @}
 */
