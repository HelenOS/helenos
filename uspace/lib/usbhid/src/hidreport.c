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

/** @addtogroup libusbhid
 * @{
 */
/**
 * @file
 * USB HID keyboard device structure and API.
 */

#include <assert.h>
#include <errno.h>
#include <str_error.h>

#include <usb/debug.h>
#include <usb/hid/hidparser.h>
#include <usb/dev/dp.h>
#include <usb/dev/driver.h>
#include <usb/dev/pipes.h>
#include <usb/hid/hid.h>
#include <usb/descriptor.h>
#include <usb/dev/request.h>

#include <usb/hid/hidreport.h>

static errno_t usb_hid_get_report_descriptor(usb_device_t *dev,
    uint8_t **report_desc, size_t *size)
{
	assert(report_desc != NULL);
	assert(size != NULL);
	
	usb_dp_parser_t parser =  {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	
	usb_dp_parser_data_t parser_data = {
		.data = usb_device_descriptors(dev)->full_config,
		.size = usb_device_descriptors(dev)->full_config_size,
		.arg = NULL
	};
	
	/*
	 * First nested descriptor of the configuration descriptor.
	 */
	const uint8_t *d =
	    usb_dp_get_nested_descriptor(&parser, &parser_data,
	        usb_device_descriptors(dev)->full_config);
	
	/*
	 * Find the interface descriptor corresponding to our interface number.
	 */
	int i = 0;
	while (d != NULL && i < usb_device_get_iface_number(dev)) {
		d = usb_dp_get_sibling_descriptor(&parser, &parser_data,
		    usb_device_descriptors(dev)->full_config, d);
		++i;
	}
	
	if (d == NULL) {
		usb_log_error("The %d. interface descriptor not found!\n",
		    usb_device_get_iface_number(dev));
		return ENOENT;
	}
	
	/*
	 * First nested descriptor of the interface descriptor.
	 */
	const uint8_t *iface_desc = d;
	d = usb_dp_get_nested_descriptor(&parser, &parser_data, iface_desc);
	
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
		usb_log_error("HID descriptor has wrong size (%u, expected %zu"
		    ")\n", *d, sizeof(usb_standard_hid_descriptor_t));
		return EINVAL;
	}
	
	usb_standard_hid_descriptor_t *hid_desc =
	    (usb_standard_hid_descriptor_t *)d;
	
	uint16_t length = uint16_usb2host(hid_desc->report_desc_info.length);
	size_t actual_size = 0;

	/*
	 * Allocate space for the report descriptor.
	 */
	*report_desc = (uint8_t *)malloc(length);
	if (*report_desc == NULL) {
		usb_log_error("Failed to allocate space for Report descriptor."
		    "\n");
		return ENOMEM;
	}
	
	usb_log_debug("Getting Report descriptor, expected size: %u\n", length);
	
	/*
	 * Get the descriptor from the device.
	 */
	errno_t rc = usb_request_get_descriptor(usb_device_get_default_pipe(dev),
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DESCTYPE_HID_REPORT, 0, usb_device_get_iface_number(dev),
	    *report_desc, length, &actual_size);

	if (rc != EOK) {
		free(*report_desc);
		*report_desc = NULL;
		return rc;
	}

	if (actual_size != length) {
		free(*report_desc);
		*report_desc = NULL;
		usb_log_error("Report descriptor has wrong size (%zu, expected "
		    "%u)\n", actual_size, length);
		return EINVAL;
	}
	
	*size = length;
	
	usb_log_debug("Done.\n");
	
	return EOK;
}



errno_t usb_hid_process_report_descriptor(usb_device_t *dev, 
    usb_hid_report_t *report, uint8_t **report_desc, size_t *report_size)
{
	if (dev == NULL || report == NULL) {
		usb_log_error("Failed to process Report descriptor: wrong "
		    "parameters given.\n");
		return EINVAL;
	}
	
//	uint8_t *report_desc = NULL;
//	size_t report_size;
	
	errno_t rc = usb_hid_get_report_descriptor(dev, report_desc, report_size);
	
	if (rc != EOK) {
		usb_log_error("Problem with getting Report descriptor: %s.\n",
		    str_error(rc));
		if (*report_desc != NULL) {
			free(*report_desc);
			*report_desc = NULL;
		}
		return rc;
	}
	
	assert(*report_desc != NULL);
	
	rc = usb_hid_parse_report_descriptor(report, *report_desc, *report_size);
	if (rc != EOK) {
		usb_log_error("Problem parsing Report descriptor: %s.\n",
		    str_error(rc));
		free(*report_desc);
		*report_desc = NULL;
		return rc;
	}
	
	usb_hid_descriptor_print(report);
	
	return EOK;
}

/**
 * @}
 */
