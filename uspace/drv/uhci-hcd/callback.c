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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <mem.h>

#include <usb/debug.h>

#include "callback.h"
int callback_init(callback_t *instance, device_t *dev,
  void *buffer, size_t size, usbhc_iface_transfer_in_callback_t func_in,
  usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(instance);
	assert(func_in == NULL || func_out == NULL);
	if (size > 0) {
		instance->new_buffer = malloc32(size);
		if (!instance->new_buffer) {
			usb_log_error("Failed to allocate device acessible buffer.\n");
			return ENOMEM;
		}
		if (func_out)
			memcpy(instance->new_buffer, buffer, size);
	} else {
		instance->new_buffer = NULL;
	}


	instance->callback_out = func_out;
	instance->callback_in = func_in;
	instance->old_buffer = buffer;
	instance->buffer_size = size;
	instance->dev = dev;
	instance->arg = arg;
	return EOK;
}
/*----------------------------------------------------------------------------*/
void callback_run(
callback_t *instance, usb_transaction_outcome_t outcome, size_t act_size)
{
	assert(instance);

	/* update the old buffer */
	if (instance->new_buffer &&
	  (instance->new_buffer != instance->old_buffer)) {
		memcpy(instance->old_buffer, instance->new_buffer, instance->buffer_size);
		free32(instance->new_buffer);
		instance->new_buffer = NULL;
	}

	if (instance->callback_in) {
		assert(instance->callback_out == NULL);
		usb_log_debug("Callback in: %p %x %d.\n",
		  instance->callback_in, outcome, act_size);
		instance->callback_in(
		  instance->dev, outcome, act_size, instance->arg);
	} else {
		assert(instance->callback_out);
		assert(instance->callback_in == NULL);
		usb_log_debug("Callback out: %p %p %x %p .\n",
		 instance->callback_out, instance->dev, outcome, instance->arg);
		instance->callback_out(
		  instance->dev, outcome, instance->arg);
	}
}
/**
 * @}
 */
