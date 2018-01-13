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

/** Private automated polling instance data. */
struct usb_device_polling {
	/** Parameters for automated polling. */
	usb_device_polling_config_t config;

	/** USB device to poll. */
	usb_device_t *dev;

	/** Device enpoint mapping to use for polling. */
	usb_endpoint_mapping_t *ep_mapping;

	/** Size of the recieved data. */
	size_t request_size;

	/** Data buffer. */
	uint8_t *buffer;

	/** True if polling is currently in operation. */
	volatile bool running;

	/** True if polling should terminate as soon as possible. */
	volatile bool joining;

	/** Synchronization primitives for joining polling end. */
	fibril_mutex_t guard;
	fibril_condvar_t cv;
};


static void polling_fini(usb_device_polling_t *polling)
{
	/* Free the allocated memory. */
	free(polling->buffer);
	free(polling);
}


/** Polling fibril.
 *
 * @param arg Pointer to usb_device_polling_t.
 * @return Always EOK.
 */
static int polling_fibril(void *arg)
{
	assert(arg);
	usb_device_polling_t *data = arg;
	data->running = true;

	/* Helper to reduce typing. */
	const usb_device_polling_config_t *params = &data->config;

	usb_pipe_t *pipe = &data->ep_mapping->pipe;

	if (params->debug > 0) {
		const usb_endpoint_mapping_t *mapping =
		    data->ep_mapping;
		usb_log_debug("Poll (%p): started polling of `%s' - " \
		    "interface %d (%s,%d,%d), %zuB/%zu.\n",
		    data, usb_device_get_name(data->dev),
		    (int) mapping->interface->interface_number,
		    usb_str_class(mapping->interface->interface_class),
		    (int) mapping->interface->interface_subclass,
		    (int) mapping->interface->interface_protocol,
		    data->request_size, pipe->desc.max_transfer_size);
	}

	size_t failed_attempts = 0;
	while (failed_attempts <= params->max_failures) {
		size_t actual_size;
		const int rc = usb_pipe_read(pipe, data->buffer,
		    data->request_size, &actual_size);

		if (rc == EOK) {
			if (params->debug > 1) {
				usb_log_debug(
				    "Poll%p: received: '%s' (%zuB).\n",
				    data,
				    usb_debug_str_buffer(data->buffer,
				        actual_size, 16),
				    actual_size);
			}
		} else {
				usb_log_debug(
				    "Poll%p: polling failed: %s.\n",
				    data, str_error(rc));
		}

		/* If the pipe stalled, we can try to reset the stall. */
		if ((rc == ESTALL) && (params->auto_clear_halt)) {
			/*
			 * We ignore error here as this is usually a futile
			 * attempt anyway.
			 */
			usb_request_clear_endpoint_halt(
			    usb_device_get_default_pipe(data->dev),
			    pipe->desc.endpoint_no);
		}

		if (rc != EOK) {
			++failed_attempts;
			const bool cont = (params->on_error == NULL) ? true :
			    params->on_error(data->dev, rc, params->arg);
			if (!cont || data->joining) {
				/* This is user requested abort, erases failures. */
				failed_attempts = 0;
				break;
			}
			continue;
		}

		/* We have the data, execute the callback now. */
		assert(params->on_data);
		const bool carry_on = params->on_data(
		    data->dev, data->buffer, actual_size, params->arg);

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
		async_usleep(params->delay);
	}

	const bool failed = failed_attempts > 0;

	if (params->on_polling_end != NULL) {
		params->on_polling_end(data->dev, failed, params->arg);
	}

	if (params->debug > 0) {
		if (failed) {
			usb_log_error("Polling of device `%s' terminated: "
			    "recurring failures.\n",
			    usb_device_get_name(data->dev));
		} else {
			usb_log_debug("Polling of device `%s' terminated: "
			    "driver request.\n",
			    usb_device_get_name(data->dev));
		}
	}

	data->running = false;

	/* Notify joiners, if any. */
	fibril_condvar_broadcast(&data->cv);

	/* Free allocated memory. */
	if (!data->joining) {
		polling_fini(data);
	}

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
 * @param dev Device to be periodically polled.
 * @param epm Endpoint mapping to use.
 * @param config Polling settings.
 * @param req_size How many bytes to ask for in each request.
 * @return Error code.
 * @retval EOK New fibril polling the device was already started.
 */
int usb_device_poll(usb_device_t *dev, usb_endpoint_mapping_t *epm,
    const usb_device_polling_config_t *config, size_t req_size,
    usb_device_polling_t **handle)
{
	int rc;
	if (!dev || !config || !config->on_data)
		return EBADMEM;

	if (!req_size)
		return EINVAL;

	if (!epm || (epm->pipe.desc.transfer_type != USB_TRANSFER_INTERRUPT) ||
	    (epm->pipe.desc.direction != USB_DIRECTION_IN))
		return EINVAL;

	usb_device_polling_t *instance = malloc(sizeof(usb_device_polling_t));
	if (!instance)
		return ENOMEM;

	/* Fill-in the data. */
	instance->buffer = malloc(req_size);
	if (!instance->buffer) {
		rc = ENOMEM;
		goto err_instance;
	}
	instance->request_size = req_size;
	instance->dev = dev;
	instance->ep_mapping = epm;
	instance->joining = false;
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->cv);

	/* Copy provided settings. */
	instance->config = *config;

	/* Negative value means use descriptor provided value. */
	if (config->delay < 0) {
		instance->config.delay = epm->descriptor->poll_interval;
	}

	fid_t fibril = fibril_create(polling_fibril, instance);
	if (!fibril) {
		rc = ENOMEM;
		goto err_buffer;
	}
	fibril_add_ready(fibril);

	if (handle)
		*handle = instance;

	/* Fibril launched. That fibril will free the allocated data. */
	return EOK;

err_buffer:
	free(instance->buffer);
err_instance:
	free(instance);
	return rc;
}

int usb_device_poll_join(usb_device_polling_t *polling)
{
	int rc;
	if (!polling)
		return EBADMEM;

	/* Set the flag */
	polling->joining = true;

	/* Unregister the pipe. */
	if ((rc = usb_device_unmap_ep(polling->ep_mapping))) {
		return rc;
	}

	/* Wait for the fibril to terminate. */
	fibril_mutex_lock(&polling->guard);
	while (polling->running)
		fibril_condvar_wait(&polling->cv, &polling->guard);
	fibril_mutex_unlock(&polling->guard);

	/* Free the instance. */
	polling_fini(polling);

	return EOK;
}

/**
 * @}
 */
