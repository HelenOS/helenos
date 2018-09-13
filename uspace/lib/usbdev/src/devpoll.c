/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2017 Petr Manek
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
 * USB device driver framework - automatic interrupt polling.
 */

#include <usb/dev/device.h>
#include <usb/dev/pipes.h>
#include <usb/dev/poll.h>
#include <usb/dev/request.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/usb.h>

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str_error.h>
#include <stddef.h>
#include <stdint.h>

/** Initialize the polling data structure, its internals and configuration
 *  with default values.
 *
 * @param polling Valid polling data structure.
 * @return Error code.
 * @retval EOK Polling data structure is ready to be used.
 */
int usb_polling_init(usb_polling_t *polling)
{
	if (!polling)
		return EBADMEM;

	/* Zero out everything */
	memset(polling, 0, sizeof(usb_polling_t));

	/* Internal initializers. */
	fibril_mutex_initialize(&polling->guard);
	fibril_condvar_initialize(&polling->cv);

	/* Default configuration. */
	polling->auto_clear_halt = true;
	polling->delay = -1;
	polling->max_failures = 3;

	return EOK;
}

/** Destroy the polling data structure.
 *  This function does nothing but a safety check whether the polling
 *  was joined successfully.
 *
 * @param polling Polling data structure.
 */
void usb_polling_fini(usb_polling_t *polling)
{
	/* Nothing done at the moment. */
	assert(polling);
	assert(!polling->running);
}

/** Polling fibril.
 *
 * @param arg Pointer to usb_polling_t.
 * @return Always EOK.
 */
static errno_t polling_fibril(void *arg)
{
	assert(arg);
	usb_polling_t *polling = arg;

	fibril_mutex_lock(&polling->guard);
	polling->running = true;
	fibril_mutex_unlock(&polling->guard);

	usb_pipe_t *pipe = &polling->ep_mapping->pipe;

	if (polling->debug > 0) {
		const usb_endpoint_mapping_t *mapping =
		    polling->ep_mapping;
		usb_log_debug("Poll (%p): started polling of `%s' - "
		    "interface %d (%s,%d,%d), %zuB/%zu.\n",
		    polling, usb_device_get_name(polling->device),
		    (int) mapping->interface->interface_number,
		    usb_str_class(mapping->interface->interface_class),
		    (int) mapping->interface->interface_subclass,
		    (int) mapping->interface->interface_protocol,
		    polling->request_size, pipe->desc.max_transfer_size);
	}

	size_t failed_attempts = 0;
	while (failed_attempts <= polling->max_failures) {
		size_t actual_size;
		const errno_t rc = usb_pipe_read(pipe, polling->buffer,
		    polling->request_size, &actual_size);

		if (rc == EOK) {
			if (polling->debug > 1) {
				usb_log_debug(
				    "Poll%p: received: '%s' (%zuB).\n",
				    polling,
				    usb_debug_str_buffer(polling->buffer,
				    actual_size, 16),
				    actual_size);
			}
		} else {
			usb_log_debug(
			    "Poll%p: polling failed: %s.\n",
			    polling, str_error(rc));
		}

		/* If the pipe stalled, we can try to reset the stall. */
		if (rc == ESTALL && polling->auto_clear_halt) {
			/*
			 * We ignore error here as this is usually a futile
			 * attempt anyway.
			 */
			usb_pipe_clear_halt(
			    usb_device_get_default_pipe(polling->device), pipe);
		}

		if (rc != EOK) {
			++failed_attempts;
			const bool carry_on = !polling->on_error ? true :
			    polling->on_error(polling->device, rc, polling->arg);

			if (!carry_on || polling->joining) {
				/* This is user requested abort, erases failures. */
				failed_attempts = 0;
				break;
			}
			continue;
		}

		/* We have the data, execute the callback now. */
		assert(polling->on_data);
		const bool carry_on = polling->on_data(polling->device,
		    polling->buffer, actual_size, polling->arg);

		if (!carry_on) {
			/* This is user requested abort, erases failures. */
			failed_attempts = 0;
			break;
		}

		/* Reset as something might be only a temporary problem. */
		failed_attempts = 0;

		/* Take a rest before next request. */

		// FIXME TODO: This is broken, the time is in ms not us.
		// but first we need to fix drivers to actually stop using this,
		// since polling delay should be implemented in HC schedule
		fibril_usleep(polling->delay);
	}

	const bool failed = failed_attempts > 0;

	if (polling->on_polling_end)
		polling->on_polling_end(polling->device, failed, polling->arg);

	if (polling->debug > 0) {
		if (failed) {
			usb_log_error("Polling of device `%s' terminated: "
			    "recurring failures.\n",
			    usb_device_get_name(polling->device));
		} else {
			usb_log_debug("Polling of device `%s' terminated: "
			    "driver request.\n",
			    usb_device_get_name(polling->device));
		}
	}

	fibril_mutex_lock(&polling->guard);
	polling->running = false;
	fibril_mutex_unlock(&polling->guard);

	/* Notify joiners, if any. */
	fibril_condvar_broadcast(&polling->cv);
	return EOK;
}

/** Start automatic device polling over interrupt in pipe.
 *
 * The polling settings is copied thus it is okay to destroy the structure
 * after this function returns.
 *
 * @warning There is no guarantee when the request to the device
 * will be sent for the first time (it is possible that this
 * first request would be executed prior to return from this function).
 *
 * @param polling Polling data structure.
 * @return Error code.
 * @retval EOK New fibril polling the device was already started.
 */
errno_t usb_polling_start(usb_polling_t *polling)
{
	if (!polling || !polling->device || !polling->ep_mapping || !polling->on_data)
		return EBADMEM;

	if (!polling->request_size)
		return EINVAL;

	if (!polling->ep_mapping || (polling->ep_mapping->pipe.desc.transfer_type != USB_TRANSFER_INTERRUPT) ||
	    (polling->ep_mapping->pipe.desc.direction != USB_DIRECTION_IN))
		return EINVAL;

	/* Negative value means use descriptor provided value. */
	if (polling->delay < 0)
		polling->delay = polling->ep_mapping->descriptor->poll_interval;

	polling->fibril = fibril_create(polling_fibril, polling);
	if (!polling->fibril)
		return ENOMEM;

	fibril_add_ready(polling->fibril);

	/* Fibril launched. That fibril will free the allocated data. */
	return EOK;
}

/** Close the polling pipe permanently and synchronously wait
 *  until the automatic polling fibril terminates.
 *
 *  It is safe to deallocate the polling data structure (and its
 *  data buffer) only after a successful call to this function.
 *
 *  @warning Call to this function will trigger execution of the
 *  on_error() callback with EINTR error code.
 *
 *  @parram polling Polling data structure.
 *  @return Error code.
 *  @retval EOK Polling fibril has been successfully terminated.
 */
errno_t usb_polling_join(usb_polling_t *polling)
{
	errno_t rc;
	if (!polling)
		return EBADMEM;

	/* Check if the fibril already terminated. */
	if (!polling->running)
		return EOK;

	/* Set the flag */
	polling->joining = true;

	/* Unregister the pipe. */
	rc = usb_device_unmap_ep(polling->ep_mapping);
	if (rc != EOK && rc != ENOENT && rc != EHANGUP)
		return rc;

	/* Wait for the fibril to terminate. */
	fibril_mutex_lock(&polling->guard);
	while (polling->running)
		fibril_condvar_wait(&polling->cv, &polling->guard);
	fibril_mutex_unlock(&polling->guard);

	return EOK;
}

/**
 * @}
 */
