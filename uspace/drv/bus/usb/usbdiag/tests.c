/*
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

/** @addtogroup drvusbdiag
 * @{
 */
/**
 * @file
 * Code for executing diagnostic tests.
 */
#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usbdiag_iface.h>
#include <time.h>
#include "device.h"
#include "tests.h"

#define NAME "usbdiag"

static const uint32_t test_data_src = 0xDEADBEEF;

static errno_t test_in(usb_pipe_t *pipe, const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	if (!pipe)
		return EBADMEM;

	bool validate = params->validate_data;
	size_t size = params->transfer_size;
	if (!size)
		size = pipe->desc.max_transfer_size;

	const uint32_t test_data = uint32_host2usb(test_data_src);
	if (validate && size % sizeof(test_data))
		return EINVAL;

	size_t test_data_size = size / sizeof(test_data);
	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s IN test with duration %ld ms.", usb_str_transfer_type(pipe->desc.transfer_type), params->min_duration);

	errno_t rc = EOK;
	uint32_t transfer_count = 0;

	struct timeval start_time, final_time, stop_time;
	gettimeofday(&start_time, NULL);
	gettimeofday(&stop_time, NULL);

	tv_add_diff(&stop_time, params->min_duration * 1000);
	gettimeofday(&final_time, NULL);

	while (!tv_gt(&final_time, &stop_time)) {
		++transfer_count;

		// Read device's response.
		size_t remaining = size;
		size_t transferred;

		while (remaining > 0) {
			if ((rc = usb_pipe_read_dma(pipe, buffer, buffer + size - remaining, remaining, &transferred))) {
				usb_log_error("Read of %s IN endpoint failed with error: %s", usb_str_transfer_type(pipe->desc.transfer_type), str_error(rc));
				break;
			}

			if (transferred > remaining) {
				usb_log_error("Read of %s IN endpoint returned more data than expected.", usb_str_transfer_type(pipe->desc.transfer_type));
				rc = EINVAL;
				break;
			}

			remaining -= transferred;
		}

		if (rc)
			break;

		if (validate) {
			uint32_t *beef_buffer = (uint32_t *) buffer;

			/* Check if the beef is really dead. */
			for (size_t i = 0; i < test_data_size; ++i) {
				if (beef_buffer[i] != test_data) {
					usb_log_error("Read of %s IN endpoint returned "
						"invalid data at address %zu. [ 0x%X != 0x%X ]",
						usb_str_transfer_type(pipe->desc.transfer_type), i * sizeof(test_data), beef_buffer[i], test_data);
					rc = EINVAL;
				}
			}

			if (rc)
				break;
		}

		gettimeofday(&final_time, NULL);
	}

	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Test on %s IN endpoint completed in %lu ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	results->act_duration = in_duration;
	results->transfer_count = transfer_count;
	results->transfer_size = size;

	usb_pipe_free_buffer(pipe, buffer);

	return rc;
}

static errno_t test_out(usb_pipe_t *pipe, const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	if (!pipe)
		return EBADMEM;

	bool validate = params->validate_data;
	size_t size = params->transfer_size;
	if (!size)
		size = pipe->desc.max_transfer_size;

	const uint32_t test_data = uint32_host2usb(test_data_src);

	if (validate && size % sizeof(test_data))
		return EINVAL;

	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	if (validate) {
		for (size_t i = 0; i < size; i += sizeof(test_data)) {
			memcpy(buffer + i, &test_data, sizeof(test_data));
		}
	}

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s OUT test.", usb_str_transfer_type(pipe->desc.transfer_type));

	errno_t rc = EOK;
	uint32_t transfer_count = 0;

	struct timeval start_time, final_time, stop_time;
	gettimeofday(&start_time, NULL);
	gettimeofday(&stop_time, NULL);

	tv_add_diff(&stop_time, params->min_duration * 1000);
	gettimeofday(&final_time, NULL);

	while (!tv_gt(&final_time, &stop_time)) {
		++transfer_count;

		// Write buffer to device.
		if ((rc = usb_pipe_write_dma(pipe, buffer, buffer, size))) {
			usb_log_error("Write to %s OUT endpoint failed with error: %s", usb_str_transfer_type(pipe->desc.transfer_type), str_error(rc));
			break;
		}

		gettimeofday(&final_time, NULL);
	}

	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Test on %s OUT endpoint completed in %ld ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	results->act_duration = in_duration;
	results->transfer_count = transfer_count;
	results->transfer_size = size;

	usb_pipe_free_buffer(pipe, buffer);

	return rc;
}

errno_t usbdiag_dev_test_in(ddf_fun_t *fun, const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	usb_pipe_t *pipe;

	switch (params->transfer_type) {
	case USB_TRANSFER_INTERRUPT:
		pipe = params->validate_data ? dev->data_intr_in : dev->burst_intr_in;
		break;
	case USB_TRANSFER_BULK:
		pipe = params->validate_data ? dev->data_bulk_in : dev->burst_bulk_in;
		break;
	case USB_TRANSFER_ISOCHRONOUS:
		pipe = params->validate_data ? dev->data_isoch_in : dev->burst_isoch_in;
		break;
	default:
		return ENOTSUP;
	}

	return test_in(pipe, params, results);
}

errno_t usbdiag_dev_test_out(ddf_fun_t *fun, const usbdiag_test_params_t *params, usbdiag_test_results_t *results)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	usb_pipe_t *pipe;

	switch (params->transfer_type) {
	case USB_TRANSFER_INTERRUPT:
		pipe = params->validate_data ? dev->data_intr_out : dev->burst_intr_out;
		break;
	case USB_TRANSFER_BULK:
		pipe = params->validate_data ? dev->data_bulk_out : dev->burst_bulk_out;
		break;
	case USB_TRANSFER_ISOCHRONOUS:
		pipe = params->validate_data ? dev->data_isoch_out : dev->burst_isoch_out;
		break;
	default:
		return ENOTSUP;
	}

	return test_out(pipe, params, results);
}

/**
 * @}
 */
