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
	int debug;
	size_t max_failures;
	useconds_t delay;
	bool auto_clear_halt;
	bool (*on_data)(usb_device_t *, uint8_t *, size_t, void *);
	void (*on_polling_end)(usb_device_t *, bool, void *);
	bool (*on_error)(usb_device_t *, int, void *);

	usb_device_t *dev;
	size_t pipe_index;
	size_t request_size;
	uint8_t *buffer;
	void *custom_arg;
} polling_data_t;


/** Polling fibril.
 *
 * @param arg Pointer to polling_data_t.
 * @return Always EOK.
 */
static int polling_fibril(void *arg)
{
	polling_data_t *polling_data = (polling_data_t *) arg;
	assert(polling_data);

	usb_pipe_t *pipe
	    = polling_data->dev->pipes[polling_data->pipe_index].pipe;
	
	if (polling_data->debug > 0) {
		usb_endpoint_mapping_t *mapping
		    = &polling_data->dev->pipes[polling_data->pipe_index];
		usb_log_debug("Poll%p: started polling of `%s' - " \
		    "interface %d (%s,%d,%d), %zuB/%zu.\n",
		    polling_data,
		    polling_data->dev->ddf_dev->name,
		    (int) mapping->interface->interface_number,
		    usb_str_class(mapping->interface->interface_class),
		    (int) mapping->interface->interface_subclass,
		    (int) mapping->interface->interface_protocol,
		    polling_data->request_size, pipe->max_packet_size);
	}

	size_t failed_attempts = 0;
	while (failed_attempts <= polling_data->max_failures) {
		int rc;

		size_t actual_size;
		rc = usb_pipe_read(pipe, polling_data->buffer,
		    polling_data->request_size, &actual_size);

		if (polling_data->debug > 1) {
			if (rc == EOK) {
				usb_log_debug(
				    "Poll%p: received: '%s' (%zuB).\n",
				    polling_data,
				    usb_debug_str_buffer(polling_data->buffer,
				        actual_size, 16),
				    actual_size);
			} else {
				usb_log_debug(
				    "Poll%p: polling failed: %s.\n",
				    polling_data, str_error(rc));
			}
		}

		/* If the pipe stalled, we can try to reset the stall. */
		if ((rc == ESTALL) && (polling_data->auto_clear_halt)) {
			/*
			 * We ignore error here as this is usually a futile
			 * attempt anyway.
			 */
			usb_request_clear_endpoint_halt(
			    &polling_data->dev->ctrl_pipe,
			    pipe->endpoint_no);
		}

		if (rc != EOK) {
			if (polling_data->on_error != NULL) {
				bool cont = polling_data->on_error(
				    polling_data->dev, rc,
				    polling_data->custom_arg);
				if (!cont) {
					failed_attempts
					    = polling_data->max_failures;
				}
			}
			failed_attempts++;
			continue;
		}

		/* We have the data, execute the callback now. */
		bool carry_on = polling_data->on_data(polling_data->dev,
		    polling_data->buffer, actual_size,
		    polling_data->custom_arg);

		if (!carry_on) {
			failed_attempts = 0;
			break;
		}

		/* Reset as something might be only a temporary problem. */
		failed_attempts = 0;

		/* Take a rest before next request. */
		async_usleep(polling_data->delay);
	}

	if (polling_data->on_polling_end != NULL) {
		polling_data->on_polling_end(polling_data->dev,
		    failed_attempts > 0, polling_data->custom_arg);
	}

	if (polling_data->debug > 0) {
		if (failed_attempts > 0) {
			usb_log_error(
			    "Polling of device `%s' terminated: %s.\n",
			    polling_data->dev->ddf_dev->name,
			    "recurring failures");
		} else {
			usb_log_debug(
			    "Polling of device `%s' terminated by user.\n",
			    polling_data->dev->ddf_dev->name
			);
		}
	}

	/* Free the allocated memory. */
	free(polling_data->buffer);
	free(polling_data);

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
	if ((dev == NULL) || (callback == NULL)) {
		return EBADMEM;
	}
	if (request_size == 0) {
		return EINVAL;
	}
	if ((dev->pipes[pipe_index].pipe->transfer_type != USB_TRANSFER_INTERRUPT)
	    || (dev->pipes[pipe_index].pipe->direction != USB_DIRECTION_IN)) {
		return EINVAL;
	}

	usb_device_auto_polling_t *auto_polling
	    = malloc(sizeof(usb_device_auto_polling_t));
	if (auto_polling == NULL) {
		return ENOMEM;
	}

	auto_polling->debug = 1;
	auto_polling->auto_clear_halt = true;
	auto_polling->delay = 0;
	auto_polling->max_failures = MAX_FAILED_ATTEMPTS;
	auto_polling->on_data = callback;
	auto_polling->on_polling_end = terminated_callback;
	auto_polling->on_error = NULL;

	int rc = usb_device_auto_polling(dev, pipe_index, auto_polling,
	   request_size, arg);

	free(auto_polling);

	return rc;
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
    usb_device_auto_polling_t *polling,
    size_t request_size, void *arg)
{
	if (dev == NULL) {
		return EBADMEM;
	}
	if (pipe_index >= dev->pipes_count) {
		return EINVAL;
	}
	if ((dev->pipes[pipe_index].pipe->transfer_type != USB_TRANSFER_INTERRUPT)
	    || (dev->pipes[pipe_index].pipe->direction != USB_DIRECTION_IN)) {
		return EINVAL;
	}
	if ((polling == NULL) || (polling->on_data == NULL)) {
		return EBADMEM;
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
	polling_data->custom_arg = arg;

	polling_data->debug = polling->debug;
	polling_data->max_failures = polling->max_failures;
	if (polling->delay >= 0) {
		polling_data->delay = (useconds_t) polling->delay;
	} else {
		polling_data->delay = (useconds_t) dev->pipes[pipe_index]
		    .descriptor->poll_interval;
	}
	polling_data->auto_clear_halt = polling->auto_clear_halt;

	polling_data->on_data = polling->on_data;
	polling_data->on_polling_end = polling->on_polling_end;
	polling_data->on_error = polling->on_error;

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
