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

/** @addtogroup drvusbmouse
 * @{
 */
/**
 * @file
 * Main routines of USB boot protocol mouse driver.
 */
#include "mouse.h"
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>

static int usbmouse_add_device(ddf_dev_t *dev)
{
	int rc = usb_mouse_create(dev);
	if (rc != EOK) {
		usb_log_error("Failed to initialize device driver: %s.\n",
		    str_error(rc));
		return rc;
	}

	fid_t poll_fibril = fibril_create(usb_mouse_polling_fibril, dev);
	if (poll_fibril == 0) {
		usb_log_error("Failed to initialize polling fibril.\n");
		/* FIXME: free allocated resources. */
		return ENOMEM;
	}

	fibril_add_ready(poll_fibril);

	usb_log_info("controlling new mouse (handle %llu).\n",
	    dev->handle);

	return EOK;
}

static driver_ops_t mouse_driver_ops = {
	.add_device = usbmouse_add_device,
};

static driver_t mouse_driver = {
	.name = NAME,
	.driver_ops = &mouse_driver_ops
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEBUG2, NAME);

	return ddf_driver_main(&mouse_driver);
}

/**
 * @}
 */
