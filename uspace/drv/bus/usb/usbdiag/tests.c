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
#include "tests.h"

#define NAME "usbdiag"


int usb_diag_stress_bulk_out(usb_diag_dev_t *dev, int cycles, size_t size)
{
	if (!dev)
		return EBADMEM;

	char *buffer = (char *) malloc(size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 42, size);

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing bulk out stress test on device %s.", ddf_fun_get_name(dev->fun));
	int rc = EOK;
	for (int i = 0; i < cycles; ++i) {
		// Write buffer to device.
		if ((rc = usb_pipe_write(dev->bulk_out, buffer, size))) {
			usb_log_error("Bulk OUT write failed. %s\n", str_error(rc));
			break;
		}
	}

	free(buffer);
	return rc;
}

int usb_diag_stress_bulk_in(usb_diag_dev_t *dev, int cycles, size_t size)
{
	if (!dev)
		return EBADMEM;

	char *buffer = (char *) malloc(size);
	if (!buffer)
		return ENOMEM;

	// TODO: Are we sure that no other test is running on this endpoint?

	usb_log_info("Performing bulk in stress test on device %s.", ddf_fun_get_name(dev->fun));
	int rc = EOK;
	for (int i = 0; i < cycles; ++i) {
		// Read device's response.
		size_t remaining = size;
		size_t transferred;

		while (remaining > 0) {
			if ((rc = usb_pipe_read(dev->bulk_in, buffer + size - remaining, remaining, &transferred))) {
				usb_log_error("Bulk IN read failed. %s\n", str_error(rc));
				break;
			}

			if (transferred > remaining) {
				usb_log_error("Bulk IN read more than expected.\n");
				rc = EINVAL;
				break;
			}

			remaining -= transferred;
		}

		if (rc)
			break;
	}

	free(buffer);
	return rc;
}

/**
 * @}
 */
