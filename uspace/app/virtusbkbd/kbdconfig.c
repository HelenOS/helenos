/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usbvirtkbd
 * @{
 */
/**
 * @file
 * @brief Keyboard configuration.
 */
#include "kbdconfig.h"
#include "keys.h"
#include <usb/usb.h>
#include <usb/classes/hid.h>
#include <usb/classes/hidut.h>
#include <usb/classes/classes.h>

/** Standard device descriptor. */
usb_standard_device_descriptor_t std_device_descriptor = {
	.length = sizeof(usb_standard_device_descriptor_t),
	.descriptor_type = USB_DESCTYPE_DEVICE,
	.usb_spec_version = 0x110,
	.device_class = USB_CLASS_USE_INTERFACE,
	.device_subclass = 0,
	.device_protocol = 0,
	.max_packet_size = 64,
	.configuration_count = 1
};

/** Standard interface descriptor. */
usb_standard_interface_descriptor_t std_interface_descriptor = {
	.length = sizeof(usb_standard_interface_descriptor_t),
	.descriptor_type = USB_DESCTYPE_INTERFACE,
	.interface_number = 0,
	.alternate_setting = 0,
	.endpoint_count = 1,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.str_interface = 0
};

/** USB keyboard report descriptor.
 * Copied from USB HID 1.11 (section E.6).
 */
report_descriptor_data_t report_descriptor = {
	STD_USAGE_PAGE(USB_HIDUT_PAGE_GENERIC_DESKTOP),
	USAGE1(USB_HIDUT_USAGE_GENERIC_DESKTOP_KEYBOARD),
	START_COLLECTION(COLLECTION_APPLICATION),
		STD_USAGE_PAGE(USB_HIDUT_PAGE_KEYBOARD),
		USAGE_MINIMUM1(224),
		USAGE_MAXIMUM1(231),
		LOGICAL_MINIMUM1(0),
		LOGICAL_MAXIMUM1(1),
		REPORT_SIZE1(1),
		REPORT_COUNT1(8),
		/* Modifiers */
		INPUT(IOF_DATA | IOF_VARIABLE | IOF_ABSOLUTE),
		REPORT_COUNT1(1),
		REPORT_SIZE1(8),
		/* Reserved */
		INPUT(IOF_CONSTANT),
		REPORT_COUNT1(5),
		REPORT_SIZE1(1),
		STD_USAGE_PAGE(USB_HIDUT_PAGE_LED),
		USAGE_MINIMUM1(1),
		USAGE_MAXIMUM1(5),
		/* LED states */
		OUTPUT(IOF_DATA | IOF_VARIABLE | IOF_ABSOLUTE),
		REPORT_COUNT1(1),
		REPORT_SIZE1(3),
		/* LED states padding */
		OUTPUT(IOF_CONSTANT),
		REPORT_COUNT1(KB_MAX_KEYS_AT_ONCE),
		REPORT_SIZE1(8),
		LOGICAL_MINIMUM1(0),
		LOGICAL_MAXIMUM1(101),
		STD_USAGE_PAGE(USB_HIDUT_PAGE_KEYBOARD),
		USAGE_MINIMUM1(0),
		USAGE_MAXIMUM1(101),
		/* Key array */
		INPUT(IOF_DATA | IOF_ARRAY),
	END_COLLECTION()
};
size_t report_descriptor_size = sizeof(report_descriptor);

/** HID descriptor. */
hid_descriptor_t hid_descriptor = {
	.length = sizeof(hid_descriptor_t),
	.type = 0x21, // HID descriptor
	.hid_spec_release = 0x101,
	.country_code = 0,
	.descriptor_count = 1,
	.descriptor1_type = 0x22, // Report descriptor
	.descriptor1_length = sizeof(report_descriptor)
};

/** Endpoint descriptor. */
usb_standard_endpoint_descriptor_t endpoint_descriptor = {
	.length = sizeof(usb_standard_endpoint_descriptor_t),
	.descriptor_type = USB_DESCTYPE_ENDPOINT,
	.endpoint_address = 1 | 128,
	.attributes = USB_TRANSFER_INTERRUPT,
	.max_packet_size = 8,
	.poll_interval = 10
};

/** Standard configuration descriptor. */
usb_standard_configuration_descriptor_t std_configuration_descriptor = {
	.length = sizeof(usb_standard_configuration_descriptor_t),
	.descriptor_type = USB_DESCTYPE_CONFIGURATION,
	.total_length = 
		sizeof(usb_standard_configuration_descriptor_t)
		+ sizeof(std_interface_descriptor)
		+ sizeof(hid_descriptor)
		+ sizeof(endpoint_descriptor)
		,
	.interface_count = 1,
	.configuration_number = 1,
	.str_configuration = 0,
	.attributes = 128, /* denotes bus-powered device */
	.max_power = 50
};





/** @}
 */
