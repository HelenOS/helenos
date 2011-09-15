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
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/host/hcd.h>

usb_transfer_batch_t * usb_transfer_batch_get(
    endpoint_t *ep,
    char *buffer,
    size_t buffer_size,
    uint64_t setup_buffer,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
    void *private_data,
    void (*private_data_dtor)(void *)
    )
{
	usb_transfer_batch_t *instance = malloc(sizeof(usb_transfer_batch_t));
	if (instance) {
		instance->ep = ep;
		instance->callback_in = func_in;
		instance->callback_out = func_out;
		instance->arg = arg;
		instance->buffer = buffer;
		instance->buffer_size = buffer_size;
		instance->setup_size = 0;
		instance->fun = fun;
		instance->private_data = private_data;
		instance->private_data_dtor = private_data_dtor;
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
/*----------------------------------------------------------------------------*/
/** Mark batch as finished and continue with next step.
 *
 * @param[in] instance Batch structure to use.
 *
 */
void usb_transfer_batch_finish(
    usb_transfer_batch_t *instance, const void *data, size_t size)
{
	assert(instance);
	assert(instance->ep);
	/* we care about the data and there are some to copy */
        if (instance->ep->direction != USB_DIRECTION_OUT
	    && data) {
		const size_t min_size =
		    size < instance->buffer_size ? size : instance->buffer_size;
                memcpy(instance->buffer, data, min_size);
        }
        if (instance->callback_out)
                usb_transfer_batch_call_out(instance);
        if (instance->callback_in)
                usb_transfer_batch_call_in(instance);

}
/*----------------------------------------------------------------------------*/
/** Prepare data, get error status and call callback in.
 *
 * @param[in] instance Batch structure to use.
 * Copies data from transport buffer, and calls callback with appropriate
 * parameters.
 */
void usb_transfer_batch_call_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	assert(instance->callback_in);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " completed (%zuB): %s.\n",
	    instance, USB_TRANSFER_BATCH_ARGS(*instance),
	    instance->transfered_size, str_error(instance->error));

	instance->callback_in(instance->fun, instance->error,
	    instance->transfered_size, instance->arg);
}
/*----------------------------------------------------------------------------*/
/** Get error status and call callback out.
 *
 * @param[in] instance Batch structure to use.
 */
void usb_transfer_batch_call_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	assert(instance->callback_out);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " completed: %s.\n",
	    instance, USB_TRANSFER_BATCH_ARGS(*instance),
	    str_error(instance->error));

	if (instance->ep->transfer_type == USB_TRANSFER_CONTROL
	    && instance->error == EOK) {
		const usb_target_t target =
		    {{ instance->ep->address, instance->ep->endpoint }};
		reset_ep_if_need(
		    fun_to_hcd(instance->fun), target, instance->setup_buffer);
	}

	instance->callback_out(instance->fun,
	    instance->error, instance->arg);
}
/*----------------------------------------------------------------------------*/
/** Correctly dispose all used data structures.
 *
 * @param[in] instance Batch structure to use.
 */
void usb_transfer_batch_dispose(usb_transfer_batch_t *instance)
{
	if (!instance)
		return;
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " disposing.\n",
	    instance, USB_TRANSFER_BATCH_ARGS(*instance));
	if (instance->ep) {
		endpoint_release(instance->ep);
	}
	if (instance->private_data) {
		assert(instance->private_data_dtor);
		instance->private_data_dtor(instance->private_data);
	}
	free(instance);
}
/**
 * @}
 */
