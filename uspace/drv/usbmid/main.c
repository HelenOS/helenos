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

/** @addtogroup drvusbmid
 * @{
 */
/**
 * @file
 * Main routines of USB multi interface device driver.
 */
#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/request.h>
#include <usb/descriptor.h>
#include <usb/pipes.h>

#include "usbmid.h"

static int usbmid_add_device(ddf_dev_t *gen_dev)
{
	usbmid_device_t *dev = usbmid_device_create(gen_dev);
	if (dev == NULL) {
		return ENOMEM;
	}

	usb_log_info("Taking care of new MID: addr %d (HC %zu)\n",
	    dev->wire.address, dev->wire.hc_handle);

	int rc;

	rc = usb_endpoint_pipe_start_session(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to start session on control pipe: %s.\n",
		    str_error(rc));
		goto error_leave;
	}

	bool accept = usbmid_explore_device(dev);

	rc = usb_endpoint_pipe_end_session(&dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_warning("Failed to end session on control pipe: %s.\n",
		    str_error(rc));
	}

	if (!accept) {
		rc = ENOTSUP;
		goto error_leave;
	}

	gen_dev->driver_data = dev;

	return EOK;


error_leave:
	free(dev);
	return rc;
}

static driver_ops_t mid_driver_ops = {
	.add_device = usbmid_add_device,
};

static driver_t mid_driver = {
	.name = NAME,
	.driver_ops = &mid_driver_ops
};

int main(int argc, char *argv[])
{
	printf(NAME ": USB multi interface device driver.\n");

	usb_log_enable(USB_LOG_LEVEL_INFO, NAME);
	return ddf_driver_main(&mid_driver);
}

/**
 * @}
 */
