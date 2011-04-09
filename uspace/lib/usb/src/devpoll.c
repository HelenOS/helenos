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

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB device driver framework - automatic interrupt polling.
 */
#include <usb/devdrv.h>
#include <usb/request.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <assert.h>

/** Maximum number of failed consecutive requests before announcing failure. */
#define MAX_FAILED_ATTEMPTS 3

/** Data needed for polling. */
typedef struct {
	usb_device_t *dev;
	size_t pipe_index;
	usb_polling_callback_t callback;
	usb_polling_terminted_callback_t terminated_callback;
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
	
	usb_log_debug("Pipe interface number: %d, protocol: %d, subclass: %d, max packet size: %d\n", 
	    polling_data->dev->pipes[polling_data->pipe_index].interface_no,
	    polling_data->dev->pipes[polling_data->pipe_index].description->interface_protocol,
	    polling_data->dev->pipes[polling_data->pipe_index].description->interface_subclass,
	    pipe->max_packet_size);

	size_t failed_attempts = 0;
	while (failed_attempts < MAX_FAILED_ATTEMPTS) {
		int rc;

		size_t actual_size;
		rc = usb_pipe_read(pipe, polling_data->buffer,
		    polling_data->request_size, &actual_size);

		
//		if (rc == ESTALL) {
//			usb_log_debug("Seding clear feature...\n");
//			usb_request_clear_feature(pipe, USB_REQUEST_TYPE_STANDARD,
//			  USB_REQUEST_RECIPIENT_ENDPOINT, 0, pipe->endpoint_no);
//			continue;
//		}

		if (rc != EOK) {
			failed_attempts++;
			continue;
		}

		/* We have the data, execute the callback now. */
		bool carry_on = polling_data->callback(polling_data->dev,
		    polling_data->buffer, actual_size,
		    polling_data->custom_arg);

		if (!carry_on) {
			failed_attempts = 0;
			break;
		}

		/* Reset as something might be only a temporary problem. */
		failed_attempts = 0;
	}

	if (failed_attempts > 0) {
		usb_log_error(
		    "Polling of device `%s' terminated: recurring failures.\n",
		    polling_data->dev->ddf_dev->name);
	}

	if (polling_data->terminated_callback != NULL) {
		polling_data->terminated_callback(polling_data->dev,
		    failed_attempts > 0, polling_data->custom_arg);
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

	polling_data_t *polling_data = malloc(sizeof(polling_data_t));
	if (polling_data == NULL) {
		return ENOMEM;
	}

	/* Allocate now to prevent immediate failure in the polling fibril. */
	polling_data->buffer = malloc(request_size);
	if (polling_data->buffer == NULL) {
		free(polling_data);
		return ENOMEM;
	}
	polling_data->dev = dev;
	polling_data->pipe_index = pipe_index;
	polling_data->callback = callback;
	polling_data->terminated_callback = terminated_callback;
	polling_data->request_size = request_size;
	polling_data->custom_arg = arg;

	fid_t fibril = fibril_create(polling_fibril, polling_data);
	if (fibril == 0) {
		free(polling_data->buffer);
		free(polling_data);
		/* FIXME: better error code. */
		return ENOMEM;
	}
	fibril_add_ready(fibril);

	/* The allocated buffer etc. will be freed by the fibril. */

	return EOK;
}

/**
 * @}
 */
