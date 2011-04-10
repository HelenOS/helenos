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
#include "mouse/mousedev.h"

/*----------------------------------------------------------------------------*/

typedef struct usb_hid_callback_mapping {
	usb_hid_report_path_t *path;
	char *vendor_id;
	char *product_id;
} usb_hid_callback_mapping;

/*----------------------------------------------------------------------------*/

/* Array of endpoints expected on the device, NULL terminated. */
usb_endpoint_description_t *usb_hid_endpoints[USB_HID_POLL_EP_COUNT + 1] = {
	&usb_hid_kbd_poll_endpoint_description,
	&usb_hid_mouse_poll_endpoint_description,
	&usb_hid_generic_poll_endpoint_description,
	NULL
};

/*----------------------------------------------------------------------------*/

static int usb_hid_set_boot_kbd_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev->subdriver_count == 0);
	
	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	
	// set the init callback
	hid_dev->subdrivers[0].init = usb_kbd_init;
	
	// set the polling callback
	hid_dev->subdrivers[0].poll = usb_kbd_polling_callback;
	
	// set the polling ended callback
	hid_dev->subdrivers[0].poll_end = NULL;
	
	// set the deinit callback
	hid_dev->subdrivers[0].deinit = usb_kbd_deinit;
	
	// set subdriver count
	hid_dev->subdriver_count = 1;
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_set_boot_mouse_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev->subdriver_count == 0);
	
	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	
	// set the init callback
	hid_dev->subdrivers[0].init = usb_mouse_init;
	
	// set the polling callback
	hid_dev->subdrivers[0].poll = usb_mouse_polling_callback;
	
	// set the polling ended callback
	hid_dev->subdrivers[0].poll_end = NULL;
	
	// set the deinit callback
	hid_dev->subdrivers[0].deinit = usb_mouse_deinit;
	
	// set subdriver count
	hid_dev->subdriver_count = 1;
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_set_generic_hid_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev->subdriver_count == 0);
	
	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	
	// set the init callback
	hid_dev->subdrivers[0].init = NULL;
	
	// set the polling callback
	hid_dev->subdrivers[0].poll = usb_generic_hid_polling_callback;
	
	// set the polling ended callback
	hid_dev->subdrivers[0].poll_end = NULL;
	
	// set the deinit callback
	hid_dev->subdrivers[0].deinit = NULL;
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_find_subdrivers(usb_hid_dev_t *hid_dev)
{
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_check_pipes(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	// first try to find subdrivers that may want to handle this device
	int rc = usb_hid_find_subdrivers(hid_dev);
	
	if (dev->pipes[USB_HID_KBD_POLL_EP_NO].present) {
		usb_log_debug("Found keyboard endpoint.\n");
		
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_KBD_POLL_EP_NO;
		
		// if no subdrivers registered, use the boot kbd subdriver
		if (hid_dev->subdriver_count == 0) {
			rc = usb_hid_set_boot_kbd_subdriver(hid_dev);
		}
	} else if (dev->pipes[USB_HID_MOUSE_POLL_EP_NO].present) {
		usb_log_debug("Found mouse endpoint.\n");
		
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_MOUSE_POLL_EP_NO;
		//hid_dev->device_type = USB_HID_PROTOCOL_MOUSE;
		
		// if no subdrivers registered, use the boot kbd subdriver
		if (hid_dev->subdriver_count == 0) {
			rc = usb_hid_set_boot_mouse_subdriver(hid_dev);
		}		
	} else if (dev->pipes[USB_HID_GENERIC_POLL_EP_NO].present) {
		usb_log_debug("Found generic HID endpoint.\n");
		
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_GENERIC_POLL_EP_NO;
		
		if (hid_dev->subdriver_count == 0) {
			usb_log_warning("Found no subdriver for handling this"
			    " HID device. Setting generic HID subdriver.\n");
			usb_hid_set_generic_hid_subdriver(hid_dev);
			return EOK;
		}
	} else {
		usb_log_error("None of supported endpoints found - probably"
		    " not a supported device.\n");
		rc = ENOTSUP;
	}
	
	return rc;
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
	
	// TODO: remove the hack
	if (rc != EOK || hid_dev->poll_pipe_index == USB_HID_MOUSE_POLL_EP_NO) {
		usb_log_warning("Could not process report descriptor.\n");
		
		if (hid_dev->poll_pipe_index == USB_HID_KBD_POLL_EP_NO) {
			usb_log_warning("Falling back to boot protocol.\n");
			rc = usb_kbd_set_boot_protocol(hid_dev);
		} else if (hid_dev->poll_pipe_index 
		    == USB_HID_MOUSE_POLL_EP_NO) {
			usb_log_warning("Falling back to boot protocol.\n");
			rc = usb_mouse_set_boot_protocol(hid_dev);
		}
	}
	
	return rc;
}

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

int usb_hid_init(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	int rc, i;
	
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
	
	for (i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].init != NULL) {
			rc = hid_dev->subdrivers[i].init(hid_dev);
			if (rc != EOK) {
				usb_log_warning("Failed to initialize HID"
				    " subdriver structure.\n");
			}
		}
	}
	
	return rc;
}

/*----------------------------------------------------------------------------*/

bool usb_hid_polling_callback(usb_device_t *dev, uint8_t *buffer, 
    size_t buffer_size, void *arg)
{
	int i;
	
	if (dev == NULL || arg == NULL || buffer == NULL) {
		usb_log_error("Missing arguments to polling callback.\n");
		return false;
	}
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)arg;
	
	bool cont = false;
	
	// continue if at least one of the subdrivers want to continue
	for (i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].poll != NULL
		    && hid_dev->subdrivers[i].poll(hid_dev, buffer, 
		    buffer_size)) {
			cont = true;
		}
	}
	
	return cont;
}

/*----------------------------------------------------------------------------*/

void usb_hid_polling_ended_callback(usb_device_t *dev, bool reason, 
     void *arg)
{
	int i; 
	
	if (dev == NULL || arg == NULL) {
		return;
	}
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)arg;
	
	for (i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].poll_end != NULL) {
			hid_dev->subdrivers[i].poll_end(hid_dev, reason);
		}
	}
	
	usb_hid_free(&hid_dev);
}

/*----------------------------------------------------------------------------*/

const char *usb_hid_get_function_name(const usb_hid_dev_t *hid_dev)
{
	switch (hid_dev->poll_pipe_index) {
	case USB_HID_KBD_POLL_EP_NO:
		return HID_KBD_FUN_NAME;
		break;
	case USB_HID_MOUSE_POLL_EP_NO:
		return HID_MOUSE_FUN_NAME;
		break;
	default:
		return HID_GENERIC_FUN_NAME;
	}
}

/*----------------------------------------------------------------------------*/

const char *usb_hid_get_class_name(const usb_hid_dev_t *hid_dev)
{
	// this means that only boot protocol keyboards will be connected
	// to the console; there is probably no better way to do this
	
	switch (hid_dev->poll_pipe_index) {
	case USB_HID_KBD_POLL_EP_NO:
		return HID_KBD_CLASS_NAME;
		break;
	case USB_HID_MOUSE_POLL_EP_NO:
		return HID_MOUSE_CLASS_NAME;
		break;
	default:
		return HID_GENERIC_CLASS_NAME;
	}
}

/*----------------------------------------------------------------------------*/

void usb_hid_free(usb_hid_dev_t **hid_dev)
{
	int i;
	
	if (hid_dev == NULL || *hid_dev == NULL) {
		return;
	}
	
	for (i = 0; i < (*hid_dev)->subdriver_count; ++i) {
		if ((*hid_dev)->subdrivers[i].deinit != NULL) {
			(*hid_dev)->subdrivers[i].deinit(*hid_dev);
		}
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
