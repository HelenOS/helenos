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

static int burst_in_test(usb_pipe_t *pipe, int cycles, size_t size, usbdiag_dur_t *duration)
{
	if (!pipe)
		return EBADMEM;

	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s IN burst test.", usb_str_transfer_type(pipe->desc.transfer_type));

	int rc = EOK;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	for (int i = 0; i < cycles; ++i) {
		// Read device's response.
		size_t remaining = size;
		size_t transferred;

		while (remaining > 0) {
			if ((rc = usb_pipe_read_dma(pipe, buffer + size - remaining, remaining, &transferred))) {
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
	}

	struct timeval final_time;
	gettimeofday(&final_time, NULL);
	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Burst test on %s IN endpoint completed in %lu ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	usb_pipe_free_buffer(pipe, buffer);
	if (duration)
		*duration = in_duration;

	return rc;
}

static int burst_out_test(usb_pipe_t *pipe, int cycles, size_t size, usbdiag_dur_t *duration)
{
	if (!pipe)
		return EBADMEM;

	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 42, size);

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s OUT burst test.", usb_str_transfer_type(pipe->desc.transfer_type));

	int rc = EOK;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	for (int i = 0; i < cycles; ++i) {
		// Write buffer to device.
		if ((rc = usb_pipe_write_dma(pipe, buffer, size))) {
			usb_log_error("Write to %s OUT endpoint failed with error: %s", usb_str_transfer_type(pipe->desc.transfer_type), str_error(rc));
			break;
		}
	}

	struct timeval final_time;
	gettimeofday(&final_time, NULL);
	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Burst test on %s OUT endpoint completed in %ld ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	usb_pipe_free_buffer(pipe, buffer);
	if (duration)
		*duration = in_duration;

	return rc;
}

static const uint32_t test_data_src = 0xDEADBEEF;

static int data_in_test(usb_pipe_t *pipe, int cycles, size_t size, usbdiag_dur_t *duration)
{
	if (!pipe)
		return EBADMEM;

	const uint32_t test_data = uint32_host2usb(test_data_src);

	if (size % sizeof(test_data))
		return EINVAL;

	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s IN data test.", usb_str_transfer_type(pipe->desc.transfer_type));

	int rc = EOK;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	for (int i = 0; i < cycles; ++i) {
		// Read device's response.
		size_t remaining = size;
		size_t transferred;

		while (remaining > 0) {
			if ((rc = usb_pipe_read_dma(pipe, buffer + size - remaining, remaining, &transferred))) {
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

		for (size_t i = 0; i < size; i += sizeof(test_data)) {
			if (*(uint32_t *)(buffer + i) != test_data) {
				usb_log_error("Read of %s IN endpoint returned "
				    "invald data at address %zu.",
				    usb_str_transfer_type(pipe->desc.transfer_type), i);
				rc = EINVAL;
				break;
			}
		}

		if (rc)
			break;
	}

	struct timeval final_time;
	gettimeofday(&final_time, NULL);
	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Data test on %s IN endpoint completed in %lu ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	usb_pipe_free_buffer(pipe, buffer);
	if (duration)
		*duration = in_duration;

	return rc;
}

static int data_out_test(usb_pipe_t *pipe, int cycles, size_t size, usbdiag_dur_t *duration)
{
	if (!pipe)
		return EBADMEM;

	const uint32_t test_data = uint32_host2usb(test_data_src);

	if (size % sizeof(test_data))
		return EINVAL;

	char *buffer = usb_pipe_alloc_buffer(pipe, size);
	if (!buffer)
		return ENOMEM;

	for (size_t i = 0; i < size; i += sizeof(test_data)) {
		memcpy(buffer + i, &test_data, sizeof(test_data));
	}

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing %s OUT data test.", usb_str_transfer_type(pipe->desc.transfer_type));

	int rc = EOK;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	for (int i = 0; i < cycles; ++i) {
		// Write buffer to device.
		if ((rc = usb_pipe_write_dma(pipe, buffer, size))) {
			usb_log_error("Write to %s OUT endpoint failed with error: %s", usb_str_transfer_type(pipe->desc.transfer_type), str_error(rc));
			break;
		}
	}

	struct timeval final_time;
	gettimeofday(&final_time, NULL);
	usbdiag_dur_t in_duration = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);

	usb_log_info("Data test on %s OUT endpoint completed in %ld ms.", usb_str_transfer_type(pipe->desc.transfer_type), in_duration);

	usb_pipe_free_buffer(pipe, buffer);
	if (duration)
		*duration = in_duration;

	return rc;
}

int usbdiag_burst_test_intr_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_in_test(dev->intr_in, cycles, size, duration);
}

int usbdiag_burst_test_intr_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_out_test(dev->intr_out, cycles, size, duration);
}

int usbdiag_burst_test_bulk_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_in_test(dev->bulk_in, cycles, size, duration);
}

int usbdiag_burst_test_bulk_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_out_test(dev->bulk_out, cycles, size, duration);
}

int usbdiag_burst_test_isoch_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_in_test(dev->isoch_in, cycles, size, duration);
}

int usbdiag_burst_test_isoch_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return burst_out_test(dev->isoch_out, cycles, size, duration);
}

int usbdiag_data_test_intr_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_in_test(dev->intr_in, cycles, size, duration);
}

int usbdiag_data_test_intr_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_out_test(dev->intr_out, cycles, size, duration);
}

int usbdiag_data_test_bulk_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_in_test(dev->bulk_in, cycles, size, duration);
}

int usbdiag_data_test_bulk_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_out_test(dev->bulk_out, cycles, size, duration);
}

int usbdiag_data_test_isoch_in(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_in_test(dev->isoch_in, cycles, size, duration);
}

int usbdiag_data_test_isoch_out(ddf_fun_t *fun, int cycles, size_t size, usbdiag_dur_t *duration)
{
	usbdiag_dev_t *dev = ddf_fun_to_usbdiag_dev(fun);
	if (!dev)
		return EBADMEM;

	return data_out_test(dev->isoch_out, cycles, size, duration);
}

/**
 * @}
 */
