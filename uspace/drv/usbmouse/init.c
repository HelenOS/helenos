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
#include <usb/classes/classes.h>
#include <usb/classes/hid.h>
#include <usb/request.h>
#include <errno.h>

/** Mouse polling endpoint description for boot protocol subclass. */
static usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};

/** Initialize poll pipe.
 *
 * Expects that session is already started on control pipe zero.
 *
 * @param mouse Mouse device.
 * @param my_interface Interface number.
 * @return Error code.
 */
static int intialize_poll_pipe(usb_mouse_t *mouse, int my_interface)
{
	assert(usb_endpoint_pipe_is_session_started(&mouse->ctrl_pipe));

	int rc;

	void *config_descriptor;
	size_t config_descriptor_size;

	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &mouse->ctrl_pipe, 0, &config_descriptor, &config_descriptor_size);
	if (rc != EOK) {
		return rc;
	}

	usb_endpoint_mapping_t endpoint_mapping[1] = {
		{
			.pipe = &mouse->poll_pipe,
			.description = &poll_endpoint_description,
			.interface_no = my_interface
		}
	};

	rc = usb_endpoint_pipe_initialize_from_configuration(endpoint_mapping,
	    1, config_descriptor, config_descriptor_size, &mouse->wire);
	if (rc != EOK) {
		return rc;
	}

	if (!endpoint_mapping[0].present) {
		return ENOENT;
	}

	mouse->poll_interval_us = 1000 * endpoint_mapping[0].descriptor->poll_interval;

	usb_log_debug("prepared polling endpoint %d (interval %zu).\n",
	    mouse->poll_pipe.endpoint_no, mouse->poll_interval_us);

	return EOK;
}


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

	rc = usb_endpoint_pipe_start_session(&mouse->ctrl_pipe);
	if (rc != EOK) {
		goto leave;
	}

	rc = intialize_poll_pipe(mouse, usb_device_get_assigned_interface(dev));

	/* We can ignore error here. */
	usb_endpoint_pipe_end_session(&mouse->ctrl_pipe);

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
