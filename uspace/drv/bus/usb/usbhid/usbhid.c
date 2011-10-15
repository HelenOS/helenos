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
#include <usb/hid/hid.h>
#include <usb/hid/hidparser.h>
#include <usb/hid/hidreport.h>
#include <usb/hid/request.h>
#include <errno.h>
#include <str_error.h>

#include "usbhid.h"

#include "kbd/kbddev.h"
#include "generic/hiddev.h"
#include "mouse/mousedev.h"
#include "subdrivers.h"

/*----------------------------------------------------------------------------*/

/* Array of endpoints expected on the device, NULL terminated. */
usb_endpoint_description_t *usb_hid_endpoints[] = {
	&usb_hid_kbd_poll_endpoint_description,
	&usb_hid_mouse_poll_endpoint_description,
	&usb_hid_generic_poll_endpoint_description,
	NULL
};

static const int USB_HID_MAX_SUBDRIVERS = 10;

/*----------------------------------------------------------------------------*/

static int usb_hid_set_boot_kbd_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL && hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}

	assert(hid_dev->subdriver_count >= 0);

	// set the init callback
	hid_dev->subdrivers[hid_dev->subdriver_count].init = usb_kbd_init;

	// set the polling callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll = 
	    usb_kbd_polling_callback;

	// set the polling ended callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll_end = NULL;

	// set the deinit callback
	hid_dev->subdrivers[hid_dev->subdriver_count].deinit = usb_kbd_deinit;

	// set subdriver count
	++hid_dev->subdriver_count;

	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_set_boot_mouse_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL && hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}

	assert(hid_dev->subdriver_count >= 0);

	// set the init callback
	hid_dev->subdrivers[hid_dev->subdriver_count].init = usb_mouse_init;

	// set the polling callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll = 
	    usb_mouse_polling_callback;

	// set the polling ended callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll_end = NULL;

	// set the deinit callback
	hid_dev->subdrivers[hid_dev->subdriver_count].deinit = usb_mouse_deinit;

	// set subdriver count
	++hid_dev->subdriver_count;

	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_set_generic_hid_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL && hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc(
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}

	assert(hid_dev->subdriver_count >= 0);

	// set the init callback
	hid_dev->subdrivers[hid_dev->subdriver_count].init =
	    usb_generic_hid_init;

	// set the polling callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll = 
	    usb_generic_hid_polling_callback;

	// set the polling ended callback
	hid_dev->subdrivers[hid_dev->subdriver_count].poll_end = NULL;

	// set the deinit callback
	hid_dev->subdrivers[hid_dev->subdriver_count].deinit =
	    usb_generic_hid_deinit;

	// set subdriver count
	++hid_dev->subdriver_count;

	return EOK;
}

/*----------------------------------------------------------------------------*/

static bool usb_hid_ids_match(usb_hid_dev_t *hid_dev, 
    const usb_hid_subdriver_mapping_t *mapping)
{
	assert(hid_dev != NULL);
	assert(hid_dev->usb_dev != NULL);

	return (hid_dev->usb_dev->descriptors.device.vendor_id 
	    == mapping->vendor_id
	    && hid_dev->usb_dev->descriptors.device.product_id 
	    == mapping->product_id);
}

/*----------------------------------------------------------------------------*/

static bool usb_hid_path_matches(usb_hid_dev_t *hid_dev, 
    const usb_hid_subdriver_mapping_t *mapping)
{
	assert(hid_dev != NULL);
	assert(mapping != NULL);

	usb_hid_report_path_t *usage_path = usb_hid_report_path();
	if (usage_path == NULL) {
		usb_log_debug("Failed to create usage path.\n");
		return false;
	}
	int i = 0;
	while (mapping->usage_path[i].usage != 0 
	    || mapping->usage_path[i].usage_page != 0) {
		if (usb_hid_report_path_append_item(usage_path, 
		    mapping->usage_path[i].usage_page, 
		    mapping->usage_path[i].usage) != EOK) {
			usb_log_debug("Failed to append to usage path.\n");
			usb_hid_report_path_free(usage_path);
			return false;
		}
		++i;
	}

	assert(hid_dev->report != NULL);

	usb_log_debug("Compare flags: %d\n", mapping->compare);

	bool matches = false;
	uint8_t report_id = mapping->report_id;

	do {
		usb_log_debug("Trying report id %u\n", report_id);
		
		if (report_id != 0) {
			usb_hid_report_path_set_report_id(usage_path,
				report_id);
		}

		usb_hid_report_field_t *field = usb_hid_report_get_sibling(
		    hid_dev->report,
		    NULL, usage_path, mapping->compare, 
		    USB_HID_REPORT_TYPE_INPUT);
		
		usb_log_debug("Field: %p\n", field);

		if (field != NULL) {
			matches = true;
			break;
		}
		
		report_id = usb_hid_get_next_report_id(
		    hid_dev->report, report_id,
		    USB_HID_REPORT_TYPE_INPUT);
	} while (!matches && report_id != 0);

	usb_hid_report_path_free(usage_path);

	return matches;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_save_subdrivers(usb_hid_dev_t *hid_dev, 
    const usb_hid_subdriver_t **subdrivers, int count)
{
	int i;

	if (count <= 0) {
		hid_dev->subdriver_count = 0;
		hid_dev->subdrivers = NULL;
		return EOK;
	}

	// add one generic HID subdriver per device

	hid_dev->subdrivers = (usb_hid_subdriver_t *)malloc((count + 1) * 
	    sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}

	for (i = 0; i < count; ++i) {
		hid_dev->subdrivers[i].init = subdrivers[i]->init;
		hid_dev->subdrivers[i].deinit = subdrivers[i]->deinit;
		hid_dev->subdrivers[i].poll = subdrivers[i]->poll;
		hid_dev->subdrivers[i].poll_end = subdrivers[i]->poll_end;
	}

	hid_dev->subdrivers[count].init = usb_generic_hid_init;
	hid_dev->subdrivers[count].poll = usb_generic_hid_polling_callback;
	hid_dev->subdrivers[count].deinit = usb_generic_hid_deinit;
	hid_dev->subdrivers[count].poll_end = NULL;

	hid_dev->subdriver_count = count + 1;

	return EOK;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_find_subdrivers(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);

	const usb_hid_subdriver_t *subdrivers[USB_HID_MAX_SUBDRIVERS];

	int i = 0, count = 0;
	const usb_hid_subdriver_mapping_t *mapping = &usb_hid_subdrivers[i];

	bool ids_matched;
	bool matched;

	while (count < USB_HID_MAX_SUBDRIVERS &&
	    (mapping->usage_path != NULL
	    || mapping->vendor_id >= 0 || mapping->product_id >= 0)) {
		// check the vendor & product ID
		if (mapping->vendor_id >= 0 && mapping->product_id < 0) {
			usb_log_warning("Missing Product ID for Vendor ID %d\n",
			    mapping->vendor_id);
			return EINVAL;
		}
		if (mapping->product_id >= 0 && mapping->vendor_id < 0) {
			usb_log_warning("Missing Vendor ID for Product ID %d\n",
			    mapping->product_id);
			return EINVAL;
		}
		
		ids_matched = false;
		matched = false;
		
		if (mapping->vendor_id >= 0) {
			assert(mapping->product_id >= 0);
			usb_log_debug("Comparing device against vendor ID %u"
			    " and product ID %u.\n", mapping->vendor_id,
			    mapping->product_id);
			if (usb_hid_ids_match(hid_dev, mapping)) {
				usb_log_debug("IDs matched.\n");
				ids_matched = true;
			}
		}
		
		if (mapping->usage_path != NULL) {
			usb_log_debug("Comparing device against usage path.\n");
			if (usb_hid_path_matches(hid_dev, mapping)) {
				// does not matter if IDs were matched
				matched = true;
			}
		} else {
			// matched only if IDs were matched and there is no path
			matched = ids_matched;
		}
		
		if (matched) {
			usb_log_debug("Subdriver matched.\n");
			subdrivers[count++] = &mapping->subdriver;
		}
		
		mapping = &usb_hid_subdrivers[++i];
	}

	// we have all subdrivers determined, save them into the hid device
	return usb_hid_save_subdrivers(hid_dev, subdrivers, count);
}

/*----------------------------------------------------------------------------*/

static int usb_hid_check_pipes(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	assert(hid_dev != NULL && dev != NULL);

	int rc = EOK;

	if (dev->pipes[USB_HID_KBD_POLL_EP_NO].present) {
		usb_log_debug("Found keyboard endpoint.\n");
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_KBD_POLL_EP_NO;
	} else if (dev->pipes[USB_HID_MOUSE_POLL_EP_NO].present) {
		usb_log_debug("Found mouse endpoint.\n");
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_MOUSE_POLL_EP_NO;
	} else if (dev->pipes[USB_HID_GENERIC_POLL_EP_NO].present) {
		usb_log_debug("Found generic HID endpoint.\n");
		// save the pipe index
		hid_dev->poll_pipe_index = USB_HID_GENERIC_POLL_EP_NO;
	} else {
		usb_log_error("None of supported endpoints found - probably"
		    " not a supported device.\n");
		rc = ENOTSUP;
	}

	return rc;
}

/*----------------------------------------------------------------------------*/

static int usb_hid_init_report(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL && hid_dev->report != NULL);

	uint8_t report_id = 0;
	size_t size;

	size_t max_size = 0;

	do {
		usb_log_debug("Getting size of the report.\n");
		size = usb_hid_report_byte_size(hid_dev->report, report_id, 
		    USB_HID_REPORT_TYPE_INPUT);
		usb_log_debug("Report ID: %u, size: %zu\n", report_id, size);
		max_size = (size > max_size) ? size : max_size;
		usb_log_debug("Getting next report ID\n");
		report_id = usb_hid_get_next_report_id(hid_dev->report, 
		    report_id, USB_HID_REPORT_TYPE_INPUT);
	} while (report_id != 0);

	usb_log_debug("Max size of input report: %zu\n", max_size);

	hid_dev->max_input_report_size = max_size;
	assert(hid_dev->input_report == NULL);

	hid_dev->input_report = malloc(max_size);
	if (hid_dev->input_report == NULL) {
		return ENOMEM;
	}
	memset(hid_dev->input_report, 0, max_size);

	return EOK;
}

/*----------------------------------------------------------------------------*/

usb_hid_dev_t *usb_hid_new(void)
{
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)calloc(1,
	    sizeof(usb_hid_dev_t));

	if (hid_dev == NULL) {
		usb_log_error("No memory!\n");
		return NULL;
	}

	hid_dev->report = (usb_hid_report_t *)(malloc(sizeof(
	    usb_hid_report_t)));
	if (hid_dev->report == NULL) {
		usb_log_error("No memory!\n");
		free(hid_dev);
		return NULL;
	}

	hid_dev->poll_pipe_index = -1;

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

	usb_hid_report_init(hid_dev->report);

	/* The USB device should already be initialized, save it in structure */
	hid_dev->usb_dev = dev;

	rc = usb_hid_check_pipes(hid_dev, dev);
	if (rc != EOK) {
		return rc;
	}
		
	/* Get the report descriptor and parse it. */
	rc = usb_hid_process_report_descriptor(hid_dev->usb_dev, 
	    hid_dev->report, &hid_dev->report_desc, &hid_dev->report_desc_size);

	bool fallback = false;

	if (rc == EOK) {
		// try to find subdrivers that may want to handle this device
		rc = usb_hid_find_subdrivers(hid_dev);
		if (rc != EOK || hid_dev->subdriver_count == 0) {
			// try to fall back to the boot protocol if available
			usb_log_info("No subdrivers found to handle this"
			    " device.\n");
			fallback = true;
			assert(hid_dev->subdrivers == NULL);
			assert(hid_dev->subdriver_count == 0);
		}
	} else {
		usb_log_error("Failed to parse Report descriptor.\n");
		// try to fall back to the boot protocol if available
		fallback = true;
	}

	if (fallback) {
		// fall back to boot protocol
		switch (hid_dev->poll_pipe_index) {
		case USB_HID_KBD_POLL_EP_NO:
			usb_log_info("Falling back to kbd boot protocol.\n");
			rc = usb_kbd_set_boot_protocol(hid_dev);
			if (rc == EOK) {
				rc = usb_hid_set_boot_kbd_subdriver(hid_dev);
			}
			break;
		case USB_HID_MOUSE_POLL_EP_NO:
			usb_log_info("Falling back to mouse boot protocol.\n");
			rc = usb_mouse_set_boot_protocol(hid_dev);
			if (rc == EOK) {
				rc = usb_hid_set_boot_mouse_subdriver(hid_dev);
			}
			break;
		default:
			assert(hid_dev->poll_pipe_index 
			    == USB_HID_GENERIC_POLL_EP_NO);
			
			usb_log_info("Falling back to generic HID driver.\n");
			rc = usb_hid_set_generic_hid_subdriver(hid_dev);
		}
	}

	if (rc != EOK) {
		usb_log_error("No subdriver for handling this device could be"
		    " initialized: %s.\n", str_error(rc));
		usb_log_debug("Subdriver count: %d\n", 
		    hid_dev->subdriver_count);
		
	} else {
		bool ok = false;
		
		usb_log_debug("Subdriver count: %d\n", 
		    hid_dev->subdriver_count);
		
		for (i = 0; i < hid_dev->subdriver_count; ++i) {
			if (hid_dev->subdrivers[i].init != NULL) {
				usb_log_debug("Initializing subdriver %d.\n",i);
				rc = hid_dev->subdrivers[i].init(hid_dev,
				    &hid_dev->subdrivers[i].data);
				if (rc != EOK) {
					usb_log_warning("Failed to initialize"
					    " HID subdriver structure.\n");
				} else {
					// at least one subdriver initialized
					ok = true;
				}
			} else {
				ok = true;
			}
		}
		
		rc = (ok) ? EOK : -1;	// what error to report
	}


	if (rc == EOK) {
		// save max input report size and allocate space for the report
		rc = usb_hid_init_report(hid_dev);
		if (rc != EOK) {
			usb_log_error("Failed to initialize input report buffer"
			    ".\n");
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

	assert(hid_dev->input_report != NULL);
	usb_log_debug("New data [%zu/%zu]: %s\n", buffer_size,
	    hid_dev->max_input_report_size,
	    usb_debug_str_buffer(buffer, buffer_size, 0));

	if (hid_dev->max_input_report_size >= buffer_size) {
		/*! @todo This should probably be atomic. */
		memcpy(hid_dev->input_report, buffer, buffer_size);
		hid_dev->input_report_size = buffer_size;
		usb_hid_new_report(hid_dev);
	}

	// parse the input report

	int rc = usb_hid_parse_report(hid_dev->report, buffer, buffer_size, 
	    &hid_dev->report_id);

	if (rc != EOK) {
		usb_log_warning("Error in usb_hid_parse_report():"
		    "%s\n", str_error(rc));
	}	

	bool cont = false;

	// continue if at least one of the subdrivers want to continue
	for (i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].poll != NULL
		    && hid_dev->subdrivers[i].poll(hid_dev, 
		        hid_dev->subdrivers[i].data)) {
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
			hid_dev->subdrivers[i].poll_end(hid_dev,
			    hid_dev->subdrivers[i].data, reason);
		}
	}

	hid_dev->running = false;
}

/*----------------------------------------------------------------------------*/

void usb_hid_new_report(usb_hid_dev_t *hid_dev)
{
	++hid_dev->report_nr;
}

/*----------------------------------------------------------------------------*/

int usb_hid_report_number(usb_hid_dev_t *hid_dev)
{
	return hid_dev->report_nr;
}

/*----------------------------------------------------------------------------*/

void usb_hid_destroy(usb_hid_dev_t *hid_dev)
{
	int i;

	if (hid_dev == NULL) {
		return;
	}

	usb_log_debug("Subdrivers: %p, subdriver count: %d\n", 
	    hid_dev->subdrivers, hid_dev->subdriver_count);

	assert(hid_dev->subdrivers != NULL 
	    || hid_dev->subdriver_count == 0);

	for (i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].deinit != NULL) {
			hid_dev->subdrivers[i].deinit(hid_dev,
			    hid_dev->subdrivers[i].data);
		}
	}

	/* Free allocated structures */
	free(hid_dev->subdrivers);
	free(hid_dev->report_desc);

	/* Destroy the parser */
	if (hid_dev->report != NULL) {
		usb_hid_free_report(hid_dev->report);
	}

}

/**
 * @}
 */
