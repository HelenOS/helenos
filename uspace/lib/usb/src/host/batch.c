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
/** @addtogroup libusb
 * @{
 */
/** @file
 * USB transfer transaction structures (implementation).
 */
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/host/batch.h>

void usb_transfer_batch_init(
    usb_transfer_batch_t *instance,
    usb_target_t target,
    usb_transfer_type_t transfer_type,
    usb_speed_t speed,
    size_t max_packet_size,
    char *buffer,
    char *transport_buffer,
    size_t buffer_size,
    char *setup_buffer,
    size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
		endpoint_t *ep,
    void *private_data
    )
{
	assert(instance);
	link_initialize(&instance->link);
	instance->target = target;
	instance->transfer_type = transfer_type;
	instance->speed = speed;
	instance->direction = USB_DIRECTION_BOTH;
	instance->callback_in = func_in;
	instance->callback_out = func_out;
	instance->arg = arg;
	instance->buffer = buffer;
	instance->transport_buffer = transport_buffer;
	instance->buffer_size = buffer_size;
	instance->setup_buffer = setup_buffer;
	instance->setup_size = setup_size;
	instance->max_packet_size = max_packet_size;
	instance->fun = fun;
	instance->private_data = private_data;
	instance->transfered_size = 0;
	instance->next_step = NULL;
	instance->error = EOK;
	instance->ep = ep;
}
/*----------------------------------------------------------------------------*/
/** Mark batch as finished and continue with next step.
 *
 * @param[in] instance Batch structure to use.
 *
 */
void usb_transfer_batch_finish(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step(instance);
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

	/* We are data in, we need data */
	memcpy(instance->buffer, instance->transport_buffer,
	    instance->buffer_size);

	usb_log_debug("Batch %p done (T%d.%d, %s %s in, %zuB): %s (%d).\n",
	    instance,
	    instance->target.address, instance->target.endpoint,
	    usb_str_speed(instance->speed),
	    usb_str_transfer_type_short(instance->transfer_type),
	    instance->transfered_size,
	    str_error(instance->error), instance->error);

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

	usb_log_debug("Batch %p done (T%d.%d, %s %s out): %s (%d).\n",
	    instance,
	    instance->target.address, instance->target.endpoint,
	    usb_str_speed(instance->speed),
	    usb_str_transfer_type_short(instance->transfer_type),
	    str_error(instance->error), instance->error);

	instance->callback_out(instance->fun,
	    instance->error, instance->arg);
}
/**
 * @}
 */
