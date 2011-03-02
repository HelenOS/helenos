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
 * Initialization routines for USB mouse driver.
 */
#include "mouse.h"
#include <usb/debug.h>
#include <usb/classes/hid.h>
#include <usb/request.h>
#include <errno.h>

#if 0
/** Mouse polling endpoint description for boot protocol subclass. */
static usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};
#endif


int usb_mouse_create(ddf_dev_t *dev)
{
	usb_mouse_t *mouse = malloc(sizeof(usb_mouse_t));
	if (mouse == NULL) {
		return ENOMEM;
	}
	mouse->device = dev;

	int rc;

	/* Initialize the backing connection. */
	rc = usb_device_connection_initialize_from_device(&mouse->wire, dev);
	if (rc != EOK) {
		goto leave;
	}

	/* Initialize the default control pipe. */
	rc = usb_endpoint_pipe_initialize_default_control(&mouse->ctrl_pipe,
	    &mouse->wire);
	if (rc != EOK) {
		goto leave;
	}

	/* FIXME: initialize to the proper endpoint. */
	rc = usb_endpoint_pipe_initialize(&mouse->poll_pipe, &mouse->wire,
	    1, USB_TRANSFER_INTERRUPT, 8, USB_DIRECTION_IN);
	if (rc != EOK) {
		goto leave;
	}

	/* Create DDF function. */
	mouse->mouse_fun = ddf_fun_create(dev, fun_exposed, "mouse");
	if (mouse->mouse_fun == NULL) {
		rc = ENOMEM;
		goto leave;
	}
	rc = ddf_fun_bind(mouse->mouse_fun);
	if (rc != EOK) {
		goto leave;
	}

	/* Everything allright. */
	dev->driver_data = mouse;

	return EOK;

leave:
	free(mouse);

	return rc;
}

/**
 * @}
 */
