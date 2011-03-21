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
/**
 * Retreives HID Report descriptor from the device.
 *
 * This function first parses the HID descriptor from the Interface descriptor
 * to get the size of the Report descriptor and then requests the Report 
 * descriptor from the device.
 *
 * @param hid_dev HID device structure.
 * @param config_desc Full configuration descriptor (including all nested
 *                    descriptors).
 * @param config_desc_size Size of the full configuration descriptor (in bytes).
 * @param iface_desc Pointer to the interface descriptor inside the full
 *                   configuration descriptor (@a config_desc) for the interface
 *                   assigned with this device (@a hid_dev).
 *
 * @retval EOK if successful.
 * @retval ENOENT if no HID descriptor could be found.
 * @retval EINVAL if the HID descriptor  or HID report descriptor have different
 *                size than expected.
 * @retval ENOMEM if some allocation failed.
 * @return Other value inherited from function usb_request_get_descriptor().
 *
 * @sa usb_request_get_descriptor()
 */
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
	
	hid_dev->report_desc_size = length;
	
	usb_log_debug("Done.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/**
 * Retreives descriptors from the device, initializes pipes and stores 
 * important information from descriptors.
 *
 * Initializes the polling pipe described by the given endpoint description
 * (@a poll_ep_desc).
 * 
 * Information retreived from descriptors and stored in the HID device structure:
 *    - Assigned interface number (the interface controlled by this instance of
 *                                 the driver)
 *    - Polling interval (from the interface descriptor)
 *    - Report descriptor
 *
 * @param hid_dev HID device structure to be initialized.
 * @param poll_ep_desc Description of the polling (Interrupt In) endpoint
 *                     that has to be present in the device in order to
 *                     successfuly initialize the structure.
 *
 * @sa usb_pipe_initialize_from_configuration(), 
 *     usbhid_dev_get_report_descriptor()
 */
static int usbhid_dev_process_descriptors(usbhid_dev_t *hid_dev, 
    usb_endpoint_description_t *poll_ep_desc) 
{
	assert(hid_dev != NULL);
	
	usb_log_info("Processing descriptors...\n");
	
	int rc;

	uint8_t *descriptors = NULL;
	size_t descriptors_size;
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &hid_dev->ctrl_pipe, 0, (void **) &descriptors, &descriptors_size);
	if (rc != EOK) {
		usb_log_error("Failed to retrieve config descriptor: %s.\n",
		    str_error(rc));
		return rc;
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
	
	rc = usb_pipe_initialize_from_configuration(
	    endpoint_mapping, 1, descriptors, descriptors_size,
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
		return EREFUSED;  // probably not very good return value
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
	
	/*
	 * Save polling interval
	 */
	hid_dev->poll_interval = endpoint_mapping[0].descriptor->poll_interval;
	assert(hid_dev->poll_interval > 0);
	
	rc = usbhid_dev_get_report_descriptor(hid_dev,
	    descriptors, descriptors_size,
	    (uint8_t *)endpoint_mapping[0].interface);
	
	free(descriptors);
	
	if (rc != EOK) {
		usb_log_warning("Problem with getting Report descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	rc = usb_hid_parse_report_descriptor(hid_dev->parser, 
	    hid_dev->report_desc, hid_dev->report_desc_size);
	if (rc != EOK) {
		usb_log_warning("Problem parsing Report descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	usb_hid_descriptor_print(hid_dev->parser);
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/* API functions                                                              */
/*----------------------------------------------------------------------------*/
/**
 * Creates new uninitialized HID device structure.
 *
 * @return Pointer to the new HID device structure, or NULL if an error occured.
 */
usbhid_dev_t *usbhid_dev_new(void)
{
	usbhid_dev_t *dev = 
	    (usbhid_dev_t *)malloc(sizeof(usbhid_dev_t));

	if (dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}
	
	memset(dev, 0, sizeof(usbhid_dev_t));
	
	dev->parser = (usb_hid_report_parser_t *)(malloc(sizeof(
	    usb_hid_report_parser_t)));
	if (dev->parser == NULL) {
		usb_log_fatal("No memory!\n");
		free(dev);
		return NULL;
	}
	
	dev->initialized = 0;
	
	return dev;
}

/*----------------------------------------------------------------------------*/
/**
 * Properly destroys the HID device structure.
 *
 * @note Currently does not clean-up the used pipes, as there are no functions
 *       offering such functionality.
 * 
 * @param hid_dev Pointer to the structure to be destroyed.
 */
void usbhid_dev_free(usbhid_dev_t **hid_dev)
{
	if (hid_dev == NULL || *hid_dev == NULL) {
		return;
	}
	
	// free the report descriptor
	if ((*hid_dev)->report_desc != NULL) {
		free((*hid_dev)->report_desc);
	}
	// destroy the parser
	if ((*hid_dev)->parser != NULL) {
		usb_hid_free_report_parser((*hid_dev)->parser);
	}
	
	// TODO: cleanup pipes
	
	free(*hid_dev);
	*hid_dev = NULL;
}

/*----------------------------------------------------------------------------*/
/**
 * Initializes HID device structure.
 *
 * @param hid_dev HID device structure to be initialized.
 * @param dev DDF device representing the HID device.
 * @param poll_ep_desc Description of the polling (Interrupt In) endpoint
 *                     that has to be present in the device in order to
 *                     successfuly initialize the structure.
 *
 * @retval EOK if successful.
 * @retval EINVAL if some argument is missing.
 * @return Other value inherited from one of functions 
 *         usb_device_connection_initialize_from_device(),
 *         usb_pipe_initialize_default_control(),
 *         usb_pipe_start_session(), usb_pipe_end_session(),
 *         usbhid_dev_process_descriptors().
 *
 * @sa usbhid_dev_process_descriptors()
 */
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
	rc = usb_pipe_initialize_default_control(&hid_dev->ctrl_pipe,
	    &hid_dev->wire);
	if (rc != EOK) {
		usb_log_error("Failed to initialize default control pipe: %s."
		    "\n", str_error(rc));
		return rc;
	}
	rc = usb_pipe_probe_default_control(&hid_dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Probing default control pipe failed: %s.\n",
		    str_error(rc));
		return rc;
	}

	/*
	 * Initialize the report parser.
	 */
	rc = usb_hid_parser_init(hid_dev->parser);
	if (rc != EOK) {
		usb_log_error("Failed to initialize report parser.\n");
		return rc;
	}

	/*
	 * Get descriptors, parse descriptors and save endpoints.
	 */
	rc = usb_pipe_start_session(&hid_dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_error("Failed to start session on the control pipe: %s"
		    ".\n", str_error(rc));
		return rc;
	}
	
	rc = usbhid_dev_process_descriptors(hid_dev, poll_ep_desc);
	if (rc != EOK) {
		/* TODO: end session?? */
		usb_pipe_end_session(&hid_dev->ctrl_pipe);
		usb_log_error("Failed to process descriptors: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	rc = usb_pipe_end_session(&hid_dev->ctrl_pipe);
	if (rc != EOK) {
		usb_log_warning("Failed to start session on the control pipe: "
		    "%s.\n", str_error(rc));
		return rc;
	}
	
	hid_dev->initialized = 1;
	usb_log_info("HID device structure initialized.\n");
	
	return EOK;
}

/**
 * @}
 */
