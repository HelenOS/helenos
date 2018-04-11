/*
 * Copyright (c) 2011 Lubos Slovak
 * Copyright (c) 2018 Petr Manek, Ondrej Hlavaty
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
#include <macros.h>
#include <str_error.h>

#include "usbhid.h"

#include "kbd/kbddev.h"
#include "generic/hiddev.h"
#include "mouse/mousedev.h"
#include "subdrivers.h"

/* Array of endpoints expected on the device, NULL terminated. */
const usb_endpoint_description_t *usb_hid_endpoints[] = {
	&usb_hid_kbd_poll_endpoint_description,
	&usb_hid_mouse_poll_endpoint_description,
	&usb_hid_generic_poll_endpoint_description,
	NULL
};

static errno_t usb_hid_set_boot_kbd_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);
	assert(hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = malloc(sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	hid_dev->subdriver_count = 1;
	// TODO 0 should be keyboard, but find a better way
	hid_dev->subdrivers[0] = usb_hid_subdrivers[0].subdriver;

	return EOK;
}

static errno_t usb_hid_set_boot_mouse_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);
	assert(hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = malloc(sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	hid_dev->subdriver_count = 1;
	// TODO 2 should be mouse, but find a better way
	hid_dev->subdrivers[0] = usb_hid_subdrivers[2].subdriver;

	return EOK;
}

static errno_t usb_hid_set_generic_hid_subdriver(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);
	assert(hid_dev->subdriver_count == 0);

	hid_dev->subdrivers = malloc(sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}
	hid_dev->subdriver_count = 1;

	/* Set generic hid subdriver routines */
	hid_dev->subdrivers[0].init = usb_generic_hid_init;
	hid_dev->subdrivers[0].poll = usb_generic_hid_polling_callback;
	hid_dev->subdrivers[0].poll_end = NULL;
	hid_dev->subdrivers[0].deinit = usb_generic_hid_deinit;

	return EOK;
}

static bool usb_hid_ids_match(const usb_hid_dev_t *hid_dev,
    const usb_hid_subdriver_mapping_t *mapping)
{
	assert(hid_dev);
	assert(hid_dev->usb_dev);
	assert(mapping);
	const usb_standard_device_descriptor_t *d =
	    &usb_device_descriptors(hid_dev->usb_dev)->device;

	return (d->vendor_id == mapping->vendor_id) &&
	    (d->product_id == mapping->product_id);
}

static bool usb_hid_path_matches(usb_hid_dev_t *hid_dev,
    const usb_hid_subdriver_mapping_t *mapping)
{
	assert(hid_dev != NULL);
	assert(mapping != NULL);

	usb_hid_report_path_t *usage_path = usb_hid_report_path();
	if (usage_path == NULL) {
		usb_log_debug("Failed to create usage path.");
		return false;
	}

	for (int i = 0; mapping->usage_path[i].usage != 0 ||
	    mapping->usage_path[i].usage_page != 0; ++i) {
		if (usb_hid_report_path_append_item(usage_path,
		    mapping->usage_path[i].usage_page,
		    mapping->usage_path[i].usage) != EOK) {
			usb_log_debug("Failed to append to usage path.");
			usb_hid_report_path_free(usage_path);
			return false;
		}
	}

	usb_log_debug("Compare flags: %d", mapping->compare);

	bool matches = false;
	uint8_t report_id = mapping->report_id;

	do {
		usb_log_debug("Trying report id %u", report_id);
		if (report_id != 0) {
			usb_hid_report_path_set_report_id(usage_path,
			    report_id);
		}

		const usb_hid_report_field_t *field =
		    usb_hid_report_get_sibling(
		    &hid_dev->report, NULL, usage_path, mapping->compare,
		    USB_HID_REPORT_TYPE_INPUT);

		usb_log_debug("Field: %p", field);

		if (field != NULL) {
			matches = true;
			break;
		}

		report_id = usb_hid_get_next_report_id(
		    &hid_dev->report, report_id, USB_HID_REPORT_TYPE_INPUT);
	} while (!matches && report_id != 0);

	usb_hid_report_path_free(usage_path);

	return matches;
}

static errno_t usb_hid_save_subdrivers(usb_hid_dev_t *hid_dev,
    const usb_hid_subdriver_t **subdrivers, unsigned count)
{
	assert(hid_dev);
	assert(subdrivers);

	if (count == 0) {
		hid_dev->subdriver_count = 0;
		hid_dev->subdrivers = NULL;
		return EOK;
	}

	/* +1 for generic hid subdriver */
	hid_dev->subdrivers = calloc((count + 1), sizeof(usb_hid_subdriver_t));
	if (hid_dev->subdrivers == NULL) {
		return ENOMEM;
	}

	for (unsigned i = 0; i < count; ++i) {
		hid_dev->subdrivers[i] = *subdrivers[i];
	}

	/* Add one generic HID subdriver per device */
	hid_dev->subdrivers[count].init = usb_generic_hid_init;
	hid_dev->subdrivers[count].poll = usb_generic_hid_polling_callback;
	hid_dev->subdrivers[count].deinit = usb_generic_hid_deinit;
	hid_dev->subdrivers[count].poll_end = NULL;

	hid_dev->subdriver_count = count + 1;

	return EOK;
}

static errno_t usb_hid_find_subdrivers(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);

	const usb_hid_subdriver_t *subdrivers[USB_HID_MAX_SUBDRIVERS];
	unsigned count = 0;

	for (unsigned i = 0; i < USB_HID_MAX_SUBDRIVERS; ++i) {
		const usb_hid_subdriver_mapping_t *mapping =
		    &usb_hid_subdrivers[i];
		/* Check the vendor & product ID. */
		if (mapping->vendor_id >= 0 && mapping->product_id < 0) {
			usb_log_warning("Mapping[%d]: Missing Product ID for "
			    "Vendor ID %d\n", i, mapping->vendor_id);
		}
		if (mapping->product_id >= 0 && mapping->vendor_id < 0) {
			usb_log_warning("Mapping[%d]: Missing Vendor ID for "
			    "Product ID %d\n", i, mapping->product_id);
		}

		bool matched = false;

		/* Check ID match. */
		if (mapping->vendor_id >= 0 && mapping->product_id >= 0) {
			usb_log_debug("Comparing device against vendor ID %u"
			    " and product ID %u.\n", mapping->vendor_id,
			    mapping->product_id);
			if (usb_hid_ids_match(hid_dev, mapping)) {
				usb_log_debug("IDs matched.");
				matched = true;
			}
		}

		/* Check usage match. */
		if (mapping->usage_path != NULL) {
			usb_log_debug("Comparing device against usage path.");
			if (usb_hid_path_matches(hid_dev, mapping)) {
				/* Does not matter if IDs were matched. */
				matched = true;
			}
		}

		if (matched) {
			usb_log_debug("Subdriver matched.");
			subdrivers[count++] = &mapping->subdriver;
		}
	}

	/* We have all subdrivers determined, save them into the hid device */
	return usb_hid_save_subdrivers(hid_dev, subdrivers, count);
}

static errno_t usb_hid_check_pipes(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	assert(hid_dev);
	assert(dev);

	static const struct {
		const usb_endpoint_description_t *desc;
		const char *description;
	} endpoints[] = {
		{ &usb_hid_kbd_poll_endpoint_description, "Keyboard endpoint" },
		{ &usb_hid_mouse_poll_endpoint_description, "Mouse endpoint" },
		{ &usb_hid_generic_poll_endpoint_description, "Generic HID endpoint" },
	};

	for (unsigned i = 0; i < ARRAY_SIZE(endpoints); ++i) {
		usb_endpoint_mapping_t *epm =
		    usb_device_get_mapped_ep_desc(dev, endpoints[i].desc);
		if (epm && epm->present) {
			usb_log_debug("Found: %s.", endpoints[i].description);
			hid_dev->poll_pipe_mapping = epm;
			return EOK;
		}
	}
	return ENOTSUP;
}

static errno_t usb_hid_init_report(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev != NULL);

	uint8_t report_id = 0;
	size_t max_size = 0;

	do {
		usb_log_debug("Getting size of the report.");
		const size_t size =
		    usb_hid_report_byte_size(&hid_dev->report, report_id,
		    USB_HID_REPORT_TYPE_INPUT);
		usb_log_debug("Report ID: %u, size: %zu", report_id, size);
		max_size = (size > max_size) ? size : max_size;
		usb_log_debug("Getting next report ID");
		report_id = usb_hid_get_next_report_id(&hid_dev->report,
		    report_id, USB_HID_REPORT_TYPE_INPUT);
	} while (report_id != 0);

	usb_log_debug("Max size of input report: %zu", max_size);

	assert(hid_dev->input_report == NULL);

	hid_dev->input_report = calloc(1, max_size);
	if (hid_dev->input_report == NULL) {
		return ENOMEM;
	}
	hid_dev->max_input_report_size = max_size;

	return EOK;
}

static bool usb_hid_polling_callback(usb_device_t *dev, uint8_t *buffer,
    size_t buffer_size, void *arg)
{
	if (dev == NULL || arg == NULL || buffer == NULL) {
		usb_log_error("Missing arguments to polling callback.");
		return false;
	}
	usb_hid_dev_t *hid_dev = arg;

	assert(hid_dev->input_report != NULL);

	usb_log_debug("New data [%zu/%zu]: %s", buffer_size,
	    hid_dev->max_input_report_size,
	    usb_debug_str_buffer(buffer, buffer_size, 0));

	if (hid_dev->max_input_report_size >= buffer_size) {
		/*! @todo This should probably be atomic. */
		memcpy(hid_dev->input_report, buffer, buffer_size);
		hid_dev->input_report_size = buffer_size;
		usb_hid_new_report(hid_dev);
	}

	/* Parse the input report */
	const errno_t rc = usb_hid_parse_report(
	    &hid_dev->report, buffer, buffer_size, &hid_dev->report_id);
	if (rc != EOK) {
		usb_log_warning("Failure in usb_hid_parse_report():"
		    "%s\n", str_error(rc));
	}

	bool cont = false;
	/* Continue if at least one of the subdrivers want to continue */
	for (unsigned i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].poll != NULL) {
			cont = cont || hid_dev->subdrivers[i].poll(
			    hid_dev, hid_dev->subdrivers[i].data);
		}
	}

	return cont;
}

static bool usb_hid_polling_error_callback(usb_device_t *dev, errno_t err_code, void *arg)
{
	assert(dev);
	assert(arg);
	usb_hid_dev_t *hid_dev = arg;

	usb_log_error("Device %s polling error: %s", usb_device_get_name(dev),
	    str_error(err_code));

	/* Continue polling until the device is about to be removed. */
	return hid_dev->running;
}

static void usb_hid_polling_ended_callback(usb_device_t *dev, bool reason, void *arg)
{
	assert(dev);
	assert(arg);

	usb_hid_dev_t *hid_dev = arg;

	for (unsigned i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].poll_end != NULL) {
			hid_dev->subdrivers[i].poll_end(
			    hid_dev, hid_dev->subdrivers[i].data, reason);
		}
	}

	hid_dev->running = false;
}

/*
 * This functions initializes required structures from the device's descriptors
 * and starts new fibril for polling the keyboard for events and another one for
 * handling auto-repeat of keys.
 *
 * During initialization, the keyboard is switched into boot protocol, the idle
 * rate is set to 0 (infinity), resulting in the keyboard only reporting event
 * when a key is pressed or released. Finally, the LED lights are turned on
 * according to the default setup of lock keys.
 *
 * @note By default, the keyboards is initialized with Num Lock turned on and
 *       other locks turned off.
 *
 * @param hid_dev Device to initialize, non-NULL.
 * @param dev USB device, non-NULL.
 * @return Error code.
 */
errno_t usb_hid_init(usb_hid_dev_t *hid_dev, usb_device_t *dev)
{
	assert(hid_dev);
	assert(dev);

	usb_log_debug("Initializing HID structure...");

	usb_hid_report_init(&hid_dev->report);

	/* The USB device should already be initialized, save it in structure */
	hid_dev->usb_dev = dev;
	hid_dev->poll_pipe_mapping = NULL;

	errno_t rc = usb_hid_check_pipes(hid_dev, dev);
	if (rc != EOK) {
		return rc;
	}

	/* Get the report descriptor and parse it. */
	rc = usb_hid_process_report_descriptor(
	    hid_dev->usb_dev, &hid_dev->report, &hid_dev->report_desc,
	    &hid_dev->report_desc_size);

	/* If report parsing went well, find subdrivers. */
	if (rc == EOK) {
		usb_hid_find_subdrivers(hid_dev);
	} else {
		usb_log_error("Failed to parse report descriptor: fallback.");
		hid_dev->subdrivers = NULL;
		hid_dev->subdriver_count = 0;
	}

	usb_log_debug("Subdriver count(before trying boot protocol): %d",
	    hid_dev->subdriver_count);

	/* No subdrivers, fall back to the boot protocol if available. */
	if (hid_dev->subdriver_count == 0) {
		assert(hid_dev->subdrivers == NULL);
		usb_log_info("No subdrivers found to handle device, trying "
		    "boot protocol.\n");

		switch (hid_dev->poll_pipe_mapping->interface->interface_protocol) {
		case USB_HID_PROTOCOL_KEYBOARD:
			usb_log_info("Falling back to kbd boot protocol.");
			rc = usb_kbd_set_boot_protocol(hid_dev);
			if (rc == EOK) {
				usb_hid_set_boot_kbd_subdriver(hid_dev);
			}
			break;
		case USB_HID_PROTOCOL_MOUSE:
			usb_log_info("Falling back to mouse boot protocol.");
			rc = usb_mouse_set_boot_protocol(hid_dev);
			if (rc == EOK) {
				usb_hid_set_boot_mouse_subdriver(hid_dev);
			}
			break;
		default:
			usb_log_info("Falling back to generic HID driver.");
			usb_hid_set_generic_hid_subdriver(hid_dev);
		}
	}

	usb_log_debug("Subdriver count(after trying boot protocol): %d",
	    hid_dev->subdriver_count);

	/* Still no subdrivers? */
	if (hid_dev->subdriver_count == 0) {
		assert(hid_dev->subdrivers == NULL);
		usb_log_error(
		    "No subdriver for handling this device could be found.\n");
		return ENOTSUP;
	}

	/* Initialize subdrivers */
	bool ok = false;
	for (unsigned i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].init != NULL) {
			usb_log_debug("Initializing subdriver %d.", i);
			const errno_t pret = hid_dev->subdrivers[i].init(hid_dev,
			    &hid_dev->subdrivers[i].data);
			if (pret != EOK) {
				usb_log_warning("Failed to initialize"
				    " HID subdriver structure: %s.\n",
				    str_error(pret));
				rc = pret;
			} else {
				/* At least one subdriver initialized. */
				ok = true;
			}
		} else {
			/* Does not need initialization. */
			ok = true;
		}
	}

	if (ok) {
		/* Save max input report size and
		 * allocate space for the report */
		rc = usb_hid_init_report(hid_dev);
		if (rc != EOK) {
			usb_log_error("Failed to initialize input report buffer: %s", str_error(rc));
			// FIXME: What happens now?
		}

		usb_polling_t *polling = &hid_dev->polling;
		if ((rc = usb_polling_init(polling))) {
			usb_log_error("Failed to initialize polling: %s", str_error(rc));
			// FIXME: What happens now?
		}

		polling->device = hid_dev->usb_dev;
		polling->ep_mapping = hid_dev->poll_pipe_mapping;
		polling->request_size = hid_dev->poll_pipe_mapping->pipe.desc.max_transfer_size;
		polling->buffer = malloc(polling->request_size);
		polling->on_data = usb_hid_polling_callback;
		polling->on_polling_end = usb_hid_polling_ended_callback;
		polling->on_error = usb_hid_polling_error_callback;
		polling->arg = hid_dev;
	}

	return rc;
}

void usb_hid_new_report(usb_hid_dev_t *hid_dev)
{
	++hid_dev->report_nr;
}

int usb_hid_report_number(const usb_hid_dev_t *hid_dev)
{
	return hid_dev->report_nr;
}

void usb_hid_deinit(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev);
	assert(hid_dev->subdrivers != NULL || hid_dev->subdriver_count == 0);

	free(hid_dev->polling.buffer);
	usb_polling_fini(&hid_dev->polling);

	usb_log_debug("Subdrivers: %p, subdriver count: %d",
	    hid_dev->subdrivers, hid_dev->subdriver_count);

	for (unsigned i = 0; i < hid_dev->subdriver_count; ++i) {
		if (hid_dev->subdrivers[i].deinit != NULL) {
			hid_dev->subdrivers[i].deinit(hid_dev,
			    hid_dev->subdrivers[i].data);
		}
	}

	/* Free allocated structures */
	free(hid_dev->subdrivers);
	free(hid_dev->report_desc);

	/* Destroy the parser */
	usb_hid_report_deinit(&hid_dev->report);
}

/**
 * @}
 */
