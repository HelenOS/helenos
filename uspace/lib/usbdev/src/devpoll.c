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
#include <usb/dev/poll.h>
#include <usb/dev/request.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <errno.h>
#include <str_error.h>
#include <assert.h>

/** Maximum number of failed consecutive requests before announcing failure. */
#define MAX_FAILED_ATTEMPTS 3

/** Data needed for polling. */
typedef struct {
	/** Parameters for automated polling. */
	usb_device_auto_polling_t auto_polling;

	/** USB device to poll. */
	usb_device_t *dev;
	/** Device pipe to use for polling. */
	size_t pipe_index;
	/** Size of the recieved data. */
	size_t request_size;
	/** Data buffer. */
	uint8_t *buffer;
} polling_data_t;


/** Polling fibril.
 *
 * @param arg Pointer to polling_data_t.
 * @return Always EOK.
 */
static int polling_fibril(void *arg)
{
	assert(arg);
	const polling_data_t *data = arg;
	/* Helper to reduce typing. */
	const usb_device_auto_polling_t *params = &data->auto_polling;

	usb_pipe_t *pipe
	    = &data->dev->pipes[data->pipe_index].pipe;

	if (params->debug > 0) {
		const usb_endpoint_mapping_t *mapping
		    = &data->dev->pipes[data->pipe_index];
		usb_log_debug("Poll%p: started polling of `%s' - " \
		    "interface %d (%s,%d,%d), %zuB/%zu.\n",
		    data, ddf_dev_get_name(data->dev->ddf_dev),
		    (int) mapping->interface->interface_number,
		    usb_str_class(mapping->interface->interface_class),
		    (int) mapping->interface->interface_subclass,
		    (int) mapping->interface->interface_protocol,
		    data->request_size, pipe->max_packet_size);
	}

	usb_pipe_start_long_transfer(pipe);
	size_t failed_attempts = 0;
	while (failed_attempts <= params->max_failures) {
		size_t actual_size;
		const int rc = usb_pipe_read(pipe, data->buffer,
		    data->request_size, &actual_size);

		if (params->debug > 1) {
			if (rc == EOK) {
				usb_log_debug(
				    "Poll%p: received: '%s' (%zuB).\n",
				    data,
				    usb_debug_str_buffer(data->buffer,
				        actual_size, 16),
				    actual_size);
			} else {
				usb_log_debug(
				    "Poll%p: polling failed: %s.\n",
				    data, str_error(rc));
			}
		}

		/* If the pipe stalled, we can try to reset the stall. */
		if ((rc == ESTALL) && (params->auto_clear_halt)) {
			/*
			 * We ignore error here as this is usually a futile
			 * attempt anyway.
			 */
			usb_request_clear_endpoint_halt(
			    &data->dev->ctrl_pipe, pipe->endpoint_no);
		}

		if (rc != EOK) {
			++failed_attempts;
			const bool cont = (params->on_error == NULL) ? true :
			    params->on_error(data->dev, rc, params->arg);
			if (!cont) {
				failed_attempts = params->max_failures;
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
		async_usleep(params->delay);
	}

	usb_pipe_end_long_transfer(pipe);

	const bool failed = failed_attempts > 0;

	if (params->on_polling_end != NULL) {
		params->on_polling_end(data->dev, failed, params->arg);
	}

	if (params->debug > 0) {
		if (failed) {
			usb_log_error("Polling of device `%s' terminated: "
			    "recurring failures.\n", ddf_dev_get_name(
			    data->dev->ddf_dev));
		} else {
			usb_log_debug("Polling of device `%s' terminated: "
			    "driver request.\n", ddf_dev_get_name(
			    data->dev->ddf_dev));
		}
	}

	/* Free the allocated memory. */
	free(data->buffer);
	free(data);

	return EOK;
}

/** Start automatic device polling over interrupt in pipe.
 *
 * @warning It is up to the callback to produce delays between individual
 * requests.
 *
 * @warning There is no guarantee when the request to the device
 * will be sent for the first time (it is possible that this
 * first request would be executed prior to return from this function).
 *
 * @param dev Device to be periodically polled.
 * @param pipe_index Index of the endpoint pipe used for polling.
 * @param callback Callback when data are available.
 * @param request_size How many bytes to ask for in each request.
 * @param terminated_callback Callback when polling is terminated.
 * @param arg Custom argument (passed as is to the callbacks).
 * @return Error code.
 * @retval EOK New fibril polling the device was already started.
 */
int usb_device_auto_poll(usb_device_t *dev, size_t pipe_index,
    usb_polling_callback_t callback, size_t request_size,
    usb_polling_terminted_callback_t terminated_callback, void *arg)
{
	const usb_device_auto_polling_t auto_polling = {
		.debug = 1,
		.auto_clear_halt = true,
		.delay = 0,
		.max_failures = MAX_FAILED_ATTEMPTS,
		.on_data = callback,
		.on_polling_end = terminated_callback,
		.on_error = NULL,
		.arg = arg,
	};

	return usb_device_auto_polling(dev, pipe_index, &auto_polling,
	   request_size);
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
 * @param pipe_index Index of the endpoint pipe used for polling.
 * @param polling Polling settings.
 * @param request_size How many bytes to ask for in each request.
 * @param arg Custom argument (passed as is to the callbacks).
 * @return Error code.
 * @retval EOK New fibril polling the device was already started.
 */
int usb_device_auto_polling(usb_device_t *dev, size_t pipe_index,
    const usb_device_auto_polling_t *polling,
    size_t request_size)
{
	if ((dev == NULL) || (polling == NULL) || (polling->on_data == NULL)) {
		return EBADMEM;
	}

	if (pipe_index >= dev->pipes_count || request_size == 0) {
		return EINVAL;
	}
	if ((dev->pipes[pipe_index].pipe.transfer_type != USB_TRANSFER_INTERRUPT)
	    || (dev->pipes[pipe_index].pipe.direction != USB_DIRECTION_IN)) {
		return EINVAL;
	}

	polling_data_t *polling_data = malloc(sizeof(polling_data_t));
	if (polling_data == NULL) {
		return ENOMEM;
	}

	/* Fill-in the data. */
	polling_data->buffer = malloc(sizeof(request_size));
	if (polling_data->buffer == NULL) {
		free(polling_data);
		return ENOMEM;
	}
	polling_data->request_size = request_size;
	polling_data->dev = dev;
	polling_data->pipe_index = pipe_index;

	/* Copy provided settings. */
	polling_data->auto_polling = *polling;

	/* Negative value means use descriptor provided value. */
	if (polling->delay < 0) {
		polling_data->auto_polling.delay =
		    (int) dev->pipes[pipe_index].descriptor->poll_interval;
	}

	fid_t fibril = fibril_create(polling_fibril, polling_data);
	if (fibril == 0) {
		free(polling_data->buffer);
		free(polling_data);
		return ENOMEM;
	}
	fibril_add_ready(fibril);

	/* Fibril launched. That fibril will free the allocated data. */

	return EOK;
}

/**
 * @}
 */
