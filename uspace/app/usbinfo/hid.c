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
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <usb/hid/hidparser.h>
#include <errno.h>
#include "usbinfo.h"

#define BYTES_PER_LINE 20

typedef enum {
	HID_DUMP_RAW,
	HID_DUMP_USAGES
} hid_dump_type_t;

typedef struct {
	usb_device_t *usb_dev;
	hid_dump_type_t dump_type;
	usb_standard_interface_descriptor_t *last_iface;
} descriptor_walk_context_t;

static bool is_descriptor_kind(const uint8_t *d, usb_descriptor_type_t t)
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

/** Dumps HID report in raw format.
 *
 * @param iface_no USB interface the report belongs to.
 * @param report Report descriptor.
 * @param size Size of the @p report in bytes.
 */
static void dump_hid_report_raw(int iface_no, uint8_t *report, size_t size)
{
	printf("%sHID report descriptor for interface %d", get_indent(0),
	    iface_no);
	for (size_t i = 0; i < size; i++) {
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
}

/** Dumps usages in HID report.
 *
 * @param iface_no USB interface the report belongs to.
 * @param report Parsed report descriptor.
 */
static void dump_hid_report_usages(int iface_no, usb_hid_report_t *report)
{
	printf("%sParsed HID report descriptor for interface %d\n",
	    get_indent(0), iface_no);
	list_foreach(report->reports, reports_link,
	    usb_hid_report_description_t, description) {
		printf("%sReport %d (type %d)\n", get_indent(1),
		    (int) description->report_id,
		    (int) description->type);
		list_foreach(description->report_items, ritems_link,
		    usb_hid_report_field_t, field) {
			printf("%sUsage page = 0x%04x    Usage = 0x%04x\n",
			    get_indent(2),
			    (int) field->usage_page, (int) field->usage);
		}
	}
}

/** Retrieves HID report from given USB device and dumps it.
 *
 * @param dump_type In which format to dump the report.
 * @param ctrl_pipe Default control pipe to the device.
 * @param iface_no Interface number.
 * @param report_size Size of the report descriptor.
 */
static void retrieve_and_dump_hid_report(hid_dump_type_t dump_type,
    usb_pipe_t *ctrl_pipe, uint8_t iface_no, size_t report_size)
{
	assert(report_size > 0);

	uint8_t *raw_report = malloc(report_size);
	if (raw_report == NULL) {
		usb_log_warning(
		    "Failed to allocate %zuB, skipping interface %d.\n",
		    report_size, (int) iface_no);
		return;
	}

	size_t actual_report_size;
	errno_t rc = usb_request_get_descriptor(ctrl_pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DESCTYPE_HID_REPORT, 0, iface_no,
	    raw_report, report_size, &actual_report_size);
	if (rc != EOK) {
		usb_log_error("Failed to retrieve HID report descriptor: %s.",
		    str_error(rc));
		free(raw_report);
		return;
	}

	usb_hid_report_t report;
	rc = usb_hid_parse_report_descriptor(&report, raw_report, report_size);
	if (rc != EOK) {
		usb_log_error("Failed to part report descriptor: %s.",
		    str_error(rc));
	}

	switch (dump_type) {
	case HID_DUMP_RAW:
		dump_hid_report_raw(iface_no, raw_report, report_size);
		break;
	case HID_DUMP_USAGES:
		dump_hid_report_usages(iface_no, &report);
		break;
	default:
		assert(false && "unreachable code apparently reached");
	}

	free(raw_report);
	usb_hid_report_deinit(&report);
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
static void descriptor_walk_callback(const uint8_t *raw_descriptor,
    size_t depth, void *arg)
{
	descriptor_walk_context_t *context = (descriptor_walk_context_t *) arg;

	if (is_descriptor_kind(raw_descriptor, USB_DESCTYPE_INTERFACE)) {
		context->last_iface =
		    (usb_standard_interface_descriptor_t *) raw_descriptor;
		return;
	}

	if (!is_descriptor_kind(raw_descriptor, USB_DESCTYPE_HID)) {
		return;
	}

	if (context->last_iface == NULL) {
		return;
	}

	usb_standard_hid_descriptor_t *hid_descr =
	    (usb_standard_hid_descriptor_t *) raw_descriptor;

	if (hid_descr->report_desc_info.type != USB_DESCTYPE_HID_REPORT) {
		return;
	}

	size_t report_size = hid_descr->report_desc_info.length;

	if (report_size == 0) {
		return;
	}

	retrieve_and_dump_hid_report(context->dump_type,
	    usb_device_get_default_pipe(context->usb_dev),
	    context->last_iface->interface_number, report_size);
}

void dump_hidreport_raw(usb_device_t *usb_dev)
{
	descriptor_walk_context_t context = {
		.usb_dev = usb_dev,
		.dump_type = HID_DUMP_RAW,
		.last_iface = NULL
	};

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    descriptor_walk_callback, &context);
}

void dump_hidreport_usages(usb_device_t *usb_dev)
{
	descriptor_walk_context_t context = {
		.usb_dev = usb_dev,
		.dump_type = HID_DUMP_USAGES,
		.last_iface = NULL
	};

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    descriptor_walk_callback, &context);
}

/** @}
 */
