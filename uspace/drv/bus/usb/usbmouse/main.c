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
#include <usb/dev/poll.h>
#include <errno.h>
#include <str_error.h>

#define NAME  "usbmouse"

/** Callback when new mouse device is attached and recognised by DDF.
 *
 * @param dev Representation of a generic DDF device.
 *
 * @return Error code.
 *
 */
static int usbmouse_device_add(usb_device_t *dev)
{
	int rc = usb_mouse_create(dev);
	if (rc != EOK) {
		usb_log_error("Failed to initialize device driver: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	usb_log_debug("Polling pipe at endpoint %d.\n",
	    dev->pipes[0].pipe->endpoint_no);
	
	rc = usb_device_auto_poll(dev, 0, usb_mouse_polling_callback,
	    dev->pipes[0].pipe->max_packet_size,
	    usb_mouse_polling_ended_callback, dev->driver_data);
	
	if (rc != EOK) {
		usb_log_error("Failed to start polling fibril: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	usb_log_info("controlling new mouse (handle %" PRIun ").\n",
	    dev->ddf_dev->handle);
	
	return EOK;
}

/** USB mouse driver ops. */
static usb_driver_ops_t mouse_driver_ops = {
	.device_add = usbmouse_device_add,
};

static usb_endpoint_description_t *endpoints[] = {
	&poll_endpoint_description,
	NULL
};

/** USB mouse driver. */
static usb_driver_t mouse_driver = {
	.name = NAME,
	.ops = &mouse_driver_ops,
	.endpoints = endpoints
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);
	return usb_driver_main(&mouse_driver);
}

/**
 * @}
 */
