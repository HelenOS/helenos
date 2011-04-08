/*
 * Copyright (c) 2011 Lubos Slovak
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB HID driver API.
 */

#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/hid.h>
#include <usb/classes/hidparser.h>
#include <usb/classes/hidreport.h>
#include <usb/classes/hidreq.h>
#include <errno.h>

#include "usbhid.h"

#include "kbd/kbddev.h"
#include "generic/hiddev.h"

/*----------------------------------------------------------------------------*/

/** Mouse polling endpoint description for boot protocol class. */
static usb_endpoint_description_t ush_hid_mouse_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};

/* Array of endpoints expected on the device, NULL terminated. */
usb_endpoint_description_t 
    *usb_hid_endpoints[USB_HID_POLL_EP_COUNT + 1] = {
	&ush_hid_kbd_poll_endpoint_description,
	&ush_hid_mouse_poll_endpoint_description,
	&usb_hid_generic_poll_endpoint_description,
	NULL
};

static const char *HID_MOUSE_FUN_NAME = "mouse";
static const char *HID_MOUSE_CLASS_NAME = "mouse";

/*----------------------------------------------------------------------------*/

usb_hid_dev_t *usb_hid_new(void)
{
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)calloc(1,
	    sizeof(usb_hid_dev_t));
	
	if (hid_dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}
	
	hid_dev->parser = (usb_hid_report_parser_t *)(malloc(sizeof(
	    usb_hid_report_parser_t)));
	if (hid_dev->parser == NULL) {
		usb_log_fatal("No memory!\n");
		free(hid_dev);
		return NULL;
	}
	
	return hid_dev;
}

/*----------------------------------------------------------------------------*/

static bool usb_dummy_polling_callback(usb_device_t *dev, uint8_t *buffer,
     size_t buffer_size, void *arg)
{
	usb_log_debug("Dummy polling callback.\n");
	return false;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_check_pipes(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	if (dev->pipes[USB_HID_KBD_POLL_EP_NO].present) {
		usb_log_debug("Found keyboard endpoint.\n");
		
		// save the pipe index and device type
		hid_dev->poll_pipe_index = USB_HID_KBD_POLL_EP_NO;
		hid_dev->device_type = USB_HID_PROTOCOL_KEYBOARD;
		
		// set the polling callback
		hid_dev->poll_callback = usb_kbd_polling_callback;

	} else if (dev->pipes[USB_HID_MOUSE_POLL_EP_NO].present) {
		usb_log_debug("Found mouse endpoint.\n");
		
		// save the pipe index and device type
		hid_dev->poll_pipe_index = USB_HID_MOUSE_POLL_EP_NO;
		hid_dev->device_type = USB_HID_PROTOCOL_MOUSE;
		
		// set the polling callback
		hid_dev->poll_callback = usb_dummy_polling_callback;
		
	} else if (dev->pipes[USB_HID_GENERIC_POLL_EP_NO].present) {
		usb_log_debug("Found generic HID endpoint.\n");
		
		// save the pipe index and device type
		hid_dev->poll_pipe_index = USB_HID_GENERIC_POLL_EP_NO;
		hid_dev->device_type = USB_HID_PROTOCOL_NONE;
		
		// set the polling callback
		hid_dev->poll_callback = usb_hid_polling_callback;
		
	} else {
		usb_log_warning("None of supported endpoints found - probably"
		    " not a supported device.\n");
		return ENOTSUP;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_init_parser(usb_hid_dev_t *hid_dev)
{
	/* Initialize the report parser. */
	int rc = usb_hid_parser_init(hid_dev->parser);
	if (rc != EOK) {
		usb_log_error("Failed to initialize report parser.\n");
		return rc;
	}
	
	/* Get the report descriptor and parse it. */
	rc = usb_hid_process_report_descriptor(hid_dev->usb_dev, 
	    hid_dev->parser);
	
	if (rc != EOK) {
		usb_log_warning("Could not process report descriptor.\n");
		
		if (hid_dev->device_type == USB_HID_PROTOCOL_KEYBOARD) {
			usb_log_warning("Falling back to boot protocol.\n");
			
			rc = usb_kbd_set_boot_protocol(hid_dev);
			
		} else if (hid_dev->device_type == USB_HID_PROTOCOL_MOUSE) {
			usb_log_warning("No boot protocol for mouse yet.\n");
			rc = ENOTSUP;
		}
	}
	
	return rc;
}

/*----------------------------------------------------------------------------*/

int usb_hid_init(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	int rc;
	
	usb_log_debug("Initializing HID structure...\n");
	
	if (hid_dev == NULL) {
		usb_log_error("Failed to init HID structure: no structure given"
		    ".\n");
		return EINVAL;
	}
	
	if (dev == NULL) {
		usb_log_error("Failed to init HID structure: no USB device"
		    " given.\n");
		return EINVAL;
	}
	
	/* The USB device should already be initialized, save it in structure */
	hid_dev->usb_dev = dev;
	
	rc = usb_hid_check_pipes(hid_dev, dev);
	if (rc != EOK) {
		return rc;
	}
	
	rc = usb_hid_init_parser(hid_dev);
	if (rc != EOK) {
		usb_log_error("Failed to initialize HID parser.\n");
		return rc;
	}
	
	switch (hid_dev->device_type) {
	case USB_HID_PROTOCOL_KEYBOARD:
		// initialize the keyboard structure
		rc = usb_kbd_init(hid_dev);
		if (rc != EOK) {
			usb_log_warning("Failed to initialize KBD structure."
			    "\n");
		}
		break;
	case USB_HID_PROTOCOL_MOUSE:
		break;
	default:
//		usbhid_req_set_idle(&hid_dev->usb_dev->ctrl_pipe, 
//		    hid_dev->usb_dev->interface_no, 0);
		break;
	}
	
	return rc;
}

/*----------------------------------------------------------------------------*/

void usb_hid_polling_ended_callback(usb_device_t *dev, bool reason, 
     void *arg)
{
	if (dev == NULL || arg == NULL) {
		return;
	}
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)arg;
	
	usb_hid_free(&hid_dev);
}

/*----------------------------------------------------------------------------*/

const char *usb_hid_get_function_name(usb_hid_iface_protocol_t device_type)
{
	switch (device_type) {
	case USB_HID_PROTOCOL_KEYBOARD:
		return HID_KBD_FUN_NAME;
		break;
	case USB_HID_PROTOCOL_MOUSE:
		return HID_MOUSE_FUN_NAME;
		break;
	default:
		return HID_GENERIC_FUN_NAME;
	}
}

/*----------------------------------------------------------------------------*/

const char *usb_hid_get_class_name(usb_hid_iface_protocol_t device_type)
{
	switch (device_type) {
	case USB_HID_PROTOCOL_KEYBOARD:
		return HID_KBD_CLASS_NAME;
		break;
	case USB_HID_PROTOCOL_MOUSE:
		return HID_MOUSE_CLASS_NAME;
		break;
	default:
		return HID_GENERIC_CLASS_NAME;
	}
}

/*----------------------------------------------------------------------------*/

void usb_hid_free(usb_hid_dev_t **hid_dev)
{
	if (hid_dev == NULL || *hid_dev == NULL) {
		return;
	}
	
	switch ((*hid_dev)->device_type) {
	case USB_HID_PROTOCOL_KEYBOARD:
		usb_kbd_deinit(*hid_dev);
		break;
	case USB_HID_PROTOCOL_MOUSE:
		break;
	default:
		break;
	}

	// destroy the parser
	if ((*hid_dev)->parser != NULL) {
		usb_hid_free_report_parser((*hid_dev)->parser);
	}

	if ((*hid_dev)->report_desc != NULL) {
		free((*hid_dev)->report_desc);
	}

	free(*hid_dev);
	*hid_dev = NULL;
}

/**
 * @}
 */
