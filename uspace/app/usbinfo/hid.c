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

/** @addtogroup usbinfo
 * @{
 */
/**
 * @file
 * Dumping of HID-related properties.
 */
#include <stdio.h>
#include <str_error.h>
#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <errno.h>
#include "usbinfo.h"

#define BYTES_PER_LINE 20

typedef struct {
	usbinfo_device_t *dev;
	usb_standard_interface_descriptor_t *last_iface;
} descriptor_walk_context_t;

static bool is_descriptor_kind(uint8_t *d, usb_descriptor_type_t t)
{
	if (d == NULL) {
		return false;
	}
	uint8_t size = d[0];
	if (size <= 1) {
		return false;
	}
	uint8_t type = d[1];
	return type == t;
}

/** Retrieves HID report from given USB device and dumps it.
 *
 * @param ctrl_pipe Default control pipe to the device.
 * @param iface_no Interface number.
 * @param report_size Size of the report descriptor.
 */
static void retrieve_and_dump_hid_report(usb_pipe_t *ctrl_pipe,
    uint8_t iface_no, size_t report_size)
{
	assert(report_size > 0);

	uint8_t *report = malloc(report_size);
	if (report == NULL) {
		usb_log_warning(
		    "Failed to allocate %zuB, skipping interface %d.\n",
		    report_size, (int) iface_no);
		return;
	}

	size_t actual_report_size;
	int rc = usb_request_get_descriptor(ctrl_pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DESCTYPE_HID_REPORT, 0, iface_no,
	    report, report_size, &actual_report_size);
	if (rc != EOK) {
		usb_log_error("Failed to retrieve HID report descriptor: %s.\n",
		    str_error(rc));
		free(report);
		return;
	}

	printf("%sHID report descriptor for interface %d", get_indent(0),
	    (int) iface_no);
	for (size_t i = 0; i < report_size; i++) {
		size_t line_idx = i % BYTES_PER_LINE;
		if (line_idx == 0) {
			printf("\n%s", get_indent(1));
		}
		printf("%02X", (int) report[i]);
		if (line_idx + 1 < BYTES_PER_LINE) {
			printf(" ");
		}
	}
	printf("\n");

	free(report);
}

/** Callback for walking descriptor tree.
 * This callback remembers current interface and dumps HID report after
 * encountering HID descriptor.
 * It dumps only the first report and it expects it to be a normal
 * report, not a physical one.
 *
 * @param raw_descriptor Descriptor as a byte array.
 * @param depth Descriptor tree depth (currently ignored).
 * @param arg Custom argument, passed as descriptor_walk_context_t.
 */
static void descriptor_walk_callback(uint8_t *raw_descriptor,
    size_t depth, void *arg)
{
	descriptor_walk_context_t *context = (descriptor_walk_context_t *) arg;

	if (is_descriptor_kind(raw_descriptor, USB_DESCTYPE_INTERFACE)) {
		context->last_iface
		    = (usb_standard_interface_descriptor_t *) raw_descriptor;
		return;
	}

	if (!is_descriptor_kind(raw_descriptor, USB_DESCTYPE_HID)) {
		return;
	}

	if (context->last_iface == NULL) {
		return;
	}

	usb_standard_hid_descriptor_t *hid_descr
	    = (usb_standard_hid_descriptor_t *) raw_descriptor;

	if (hid_descr->report_desc_info.type != USB_DESCTYPE_HID_REPORT) {
		return;
	}

	size_t report_size = hid_descr->report_desc_info.length;

	if (report_size == 0) {
		return;
	}

	retrieve_and_dump_hid_report(&context->dev->ctrl_pipe,
	    context->last_iface->interface_number, report_size);
}


void dump_hidreport(usbinfo_device_t *dev)
{
	descriptor_walk_context_t context = {
		.dev = dev,
		.last_iface = NULL
	};

	usb_dp_walk_simple(dev->full_configuration_descriptor,
	    dev->full_configuration_descriptor_size,
	    usb_dp_standard_descriptor_nesting,
	    descriptor_walk_callback, &context);
}

/** @}
 */
