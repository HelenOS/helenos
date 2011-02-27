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
 * Generic USB HID device structure and API.
 */

#include <assert.h>
#include <errno.h>
#include <str_error.h>

#include <ddf/driver.h>

#include <usb/dp.h>
#include <usb/debug.h>
#include <usb/request.h>
#include <usb/descriptor.h>
#include <usb/classes/hid.h>
#include <usb/pipes.h>

#include "hiddev.h"

/*----------------------------------------------------------------------------*/
/* Non-API functions                                                          */
/*----------------------------------------------------------------------------*/

static int usbhid_dev_get_report_descriptor(usbhid_dev_t *hid_dev, 
    uint8_t *config_desc, size_t config_desc_size, uint8_t *iface_desc)
{
	assert(hid_dev != NULL);
	assert(config_desc != NULL);
	assert(config_desc_size != 0);
	assert(iface_desc != NULL);
	
	usb_dp_parser_t parser =  {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	
	usb_dp_parser_data_t parser_data = {
		.data = config_desc,
		.size = config_desc_size,
		.arg = NULL
	};
	
	/*
	 * First nested descriptor of interface descriptor.
	 */
	uint8_t *d = 
	    usb_dp_get_nested_descriptor(&parser, &parser_data, iface_desc);
	
	/*
	 * Search through siblings until the HID descriptor is found.
	 */
	while (d != NULL && *(d + 1) != USB_DESCTYPE_HID) {
		d = usb_dp_get_sibling_descriptor(&parser, &parser_data, 
		    iface_desc, d);
	}
	
	if (d == NULL) {
		usb_log_fatal("No HID descriptor found!\n");
		return ENOENT;
	}
	
	if (*d != sizeof(usb_standard_hid_descriptor_t)) {
		usb_log_fatal("HID descriptor hass wrong size (%u, expected %u"
		    ")\n", *d, sizeof(usb_standard_hid_descriptor_t));
		return EINVAL;
	}
	
	usb_standard_hid_descriptor_t *hid_desc = 
	    (usb_standard_hid_descriptor_t *)d;
	
	uint16_t length =  hid_desc->report_desc_info.length;
	size_t actual_size = 0;

	/*
	 * Allocate space for the report descriptor.
	 */
	hid_dev->report_desc = (uint8_t *)malloc(length);
	if (hid_dev->report_desc == NULL) {
		usb_log_fatal("Failed to allocate space for Report descriptor."
		    "\n");
		return ENOMEM;
	}
	
	usb_log_debug("Getting Report descriptor, expected size: %u\n", length);
	
	/*
	 * Get the descriptor from the device.
	 */
	int rc = usb_request_get_descriptor(&hid_dev->ctrl_pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DESCTYPE_HID_REPORT, 0,
	    hid_dev->iface, hid_dev->report_desc, length, &actual_size);

	if (rc != EOK) {
		return rc;
	}

	if (actual_size != length) {
		free(hid_dev->report_desc);
		hid_dev->report_desc = NULL;
		usb_log_fatal("Report descriptor has wrong size (%u, expected "
		    "%u)\n", actual_size, length);
		return EINVAL;
	}
	
	usb_log_debug("Done.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usbhid_dev_process_descriptors(usbhid_dev_t *hid_dev, 
    usb_endpoint_description_t *poll_ep_desc) 
{
	assert(hid_dev != NULL);
	
	usb_log_info("Processing descriptors...\n");
	
	// get the first configuration descriptor
	usb_standard_configuration_descriptor_t config_desc;
	
	int rc;
	rc = usb_request_get_bare_configuration_descriptor(&hid_dev->ctrl_pipe,
	    0, &config_desc);
	
	if (rc != EOK) {
		usb_log_error("Failed to get bare config descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	// prepare space for all underlying descriptors
	uint8_t *descriptors = (uint8_t *)malloc(config_desc.total_length);
	if (descriptors == NULL) {
		usb_log_error("No memory!.\n");
		return ENOMEM;
	}
	
	size_t transferred = 0;
	// get full configuration descriptor
	rc = usb_request_get_full_configuration_descriptor(&hid_dev->ctrl_pipe,
	    0, descriptors, config_desc.total_length, &transferred);
	
	if (rc != EOK) {
		usb_log_error("Failed to get full config descriptor: %s.\n",
		    str_error(rc));
		free(descriptors);
		return rc;
	}
	
	if (transferred != config_desc.total_length) {
		usb_log_error("Configuration descriptor has wrong size (%u, "
		    "expected %u).\n", transferred, config_desc.total_length);
		free(descriptors);
		return ELIMIT;
	}
	
	/*
	 * Initialize the interrupt in endpoint.
	 */
	usb_endpoint_mapping_t endpoint_mapping[1] = {
		{
			.pipe = &hid_dev->poll_pipe,
			.description = poll_ep_desc,
			.interface_no =
			    usb_device_get_assigned_interface(hid_dev->device)
		}
	};
	
	rc = usb_endpoint_pipe_initialize_from_configuration(
	    endpoint_mapping, 1, descriptors, config_desc.total_length,
	    &hid_dev->wire);
	
	if (rc != EOK) {
		usb_log_error("Failed to initialize poll pipe: %s.\n",
		    str_error(rc));
		free(descriptors);
		return rc;
	}
	
	if (!endpoint_mapping[0].present) {
		usb_log_warning("Not accepting device.\n");
		free(descriptors);
		return EREFUSED;
	}
	
	usb_log_debug("Accepted device. Saving interface, and getting Report"
	    " descriptor.\n");
	
	/*
	 * Save assigned interface number.
	 */
	if (endpoint_mapping[0].interface_no < 0) {
		usb_log_error("Bad interface number.\n");
		free(descriptors);
		return EINVAL;
	}
	
	hid_dev->iface = endpoint_mapping[0].interface_no;
	
	assert(endpoint_mapping[0].interface != NULL);
	
	rc = usbhid_dev_get_report_descriptor(hid_dev, descriptors, transferred,
	    (uint8_t *)endpoint_mapping[0].interface);
	
	free(descriptors);
	
	if (rc != EOK) {
		usb_log_warning("Problem with parsing Report descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/* API functions                                                              */
/*----------------------------------------------------------------------------*/

usbhid_dev_t *usbhid_dev_new()
{
	usbhid_dev_t *dev = 
	    (usbhid_dev_t *)malloc(sizeof(usbhid_dev_t));

	if (dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}
	
	memset(dev, 0, sizeof(usbhid_dev_t));
	
	dev->initialized = 0;
	
	return dev;
}

/*----------------------------------------------------------------------------*/

int usbhid_dev_init(usbhid_dev_t *hid_dev, ddf_dev_t *dev, 
    usb_endpoint_description_t *poll_ep_desc)
{
	usb_log_info("Initializing HID device structure.\n");
	
	if (hid_dev == NULL) {
		usb_log_error("Failed to init HID device structure: no "
		    "structure given.\n");
		return EINVAL;
	}
	
	if (dev == NULL) {
		usb_log_error("Failed to init HID device structure: no device"
		    " given.\n");
		return EINVAL;
	}
	
	if (poll_ep_desc == NULL) {
		usb_log_error("No poll endpoint description given.\n");
		return EINVAL;
	}
	
	hid_dev->device = dev;
	
	int rc;

	/*
	 * Initialize the backing connection to the host controller.
	 */
	rc = usb_device_connection_initialize_from_device(&hid_dev->wire, dev);
	if (rc != EOK) {
		usb_log_error("Problem initializing connection to device: %s."
		    "\n", str_error(rc));
		return rc;
	}

	/*
	 * Initialize device pipes.
	 */
	rc = usb_endpoint_pipe_initialize_default_control(&hid_dev->ctrl_pipe,
	    &hid_dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed to initialize default control pipe: %s."
		    "\n", str_error(rc));
		return rc;
	}

	/*
	 * Get descriptors, parse descriptors and save endpoints.
	 */
	usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	
	rc = usbhid_dev_process_descriptors(hid_dev, poll_ep_desc);
	
	usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to process descriptors: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	hid_dev->initialized = 1;
	usb_log_info("HID device structure initialized.\n");
	
	return EOK;
}

/**
 * @}
 */
