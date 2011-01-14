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

/** @addtogroup usb
 * @{
 */
/**
 * @file
 * @brief USB querying.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>

#include <usb/usb.h>
#include <usb/descriptor.h>

#include "usbinfo.h"
#include <usb/dp.h>

#define INDENT "  "
#define BYTES_PER_LINE 12

#define BCD_INT(a) (((unsigned int)(a)) / 256)
#define BCD_FRAC(a) (((unsigned int)(a)) % 256)

#define BCD_FMT "%x.%x"
#define BCD_ARGS(a) BCD_INT((a)), BCD_FRAC((a))

void dump_buffer(const char *msg, const uint8_t *buffer, size_t length)
{
	printf("%s\n", msg);

	size_t i;
	for (i = 0; i < length; i++) {
		printf("  0x%02X", buffer[i]);
		if (((i > 0) && (((i+1) % BYTES_PER_LINE) == 0))
		    || (i + 1 == length)) {
			printf("\n");
		}
	}
}

void dump_match_ids(match_id_list_t *matches)
{
	printf("Match ids:\n");
	link_t *link;
	for (link = matches->ids.next;
	    link != &matches->ids;
	    link = link->next) {
		match_id_t *match = list_get_instance(link, match_id_t, link);

		printf(INDENT "%d %s\n", match->score, match->id);
	}
}

void dump_standard_device_descriptor(usb_standard_device_descriptor_t *d)
{
	printf("Standard device descriptor:\n");

	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "bcdUSB = %d (" BCD_FMT ")\n", d->usb_spec_version,
	    BCD_ARGS(d->usb_spec_version));
	printf(INDENT "bDeviceClass = 0x%02x\n", d->device_class);
	printf(INDENT "bDeviceSubClass = 0x%02x\n", d->device_subclass);
	printf(INDENT "bDeviceProtocol = 0x%02x\n", d->device_protocol);
	printf(INDENT "bMaxPacketSize0 = %d\n", d->max_packet_size);
	printf(INDENT "idVendor = %d\n", d->vendor_id);
	printf(INDENT "idProduct = %d\n", d->product_id);
	printf(INDENT "bcdDevice = %d\n", d->device_version);
	printf(INDENT "iManufacturer = %d\n", d->str_manufacturer);
	printf(INDENT "iProduct = %d\n", d->str_product);
	printf(INDENT "iSerialNumber = %d\n", d->str_serial_number);
	printf(INDENT "bNumConfigurations = %d\n", d->configuration_count);
}

void dump_standard_configuration_descriptor(
    int index, usb_standard_configuration_descriptor_t *d)
{
	bool self_powered = d->attributes & 64;
	bool remote_wakeup = d->attributes & 32;
	
	printf("Standard configuration descriptor #%d\n", index);
	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "wTotalLength = %d\n", d->total_length);
	printf(INDENT "bNumInterfaces = %d\n", d->interface_count);
	printf(INDENT "bConfigurationValue = %d\n", d->configuration_number);
	printf(INDENT "iConfiguration = %d\n", d->str_configuration);
	printf(INDENT "bmAttributes = %d [%s%s%s]\n", d->attributes,
	    self_powered ? "self-powered" : "",
	    (self_powered & remote_wakeup) ? ", " : "",
	    remote_wakeup ? "remote-wakeup" : "");
	printf(INDENT "MaxPower = %d (%dmA)\n", d->max_power,
	    2 * d->max_power);
	// printf(INDENT " = %d\n", d->);
}

static void dump_tree_descriptor(uint8_t *descriptor, size_t depth)
{
	if (descriptor == NULL) {
		return;
	}
	while (depth > 0) {
		printf("  ");
		depth--;
	}
	int type = (int) *(descriptor + 1);
	const char *name = "unknown";
	switch (type) {
#define _TYPE(descriptor_type) \
		case USB_DESCTYPE_##descriptor_type: name = #descriptor_type; break
		_TYPE(DEVICE);
		_TYPE(CONFIGURATION);
		_TYPE(STRING);
		_TYPE(INTERFACE);
		_TYPE(ENDPOINT);
		_TYPE(HID);
		_TYPE(HID_REPORT);
		_TYPE(HID_PHYSICAL);
		_TYPE(HUB);
#undef _TYPE
	}
	printf("0x%02x (%s)\n", type, name);
}

static void dump_tree_internal(usb_dp_parser_t *parser, usb_dp_parser_data_t *data,
    uint8_t *root, size_t depth)
{
	if (root == NULL) {
		return;
	}
	dump_tree_descriptor(root, depth);
	uint8_t *child = usb_dp_get_nested_descriptor(parser, data, root);
	do {
		dump_tree_internal(parser, data, child, depth + 1);
		child = usb_dp_get_sibling_descriptor(parser, data, root, child);
	} while (child != NULL);
}

static void dump_tree(usb_dp_parser_t *parser, usb_dp_parser_data_t *data)
{
	uint8_t *ptr = data->data;
	printf("Descriptor tree:\n");
	dump_tree_internal(parser, data, ptr, 1);
}

#define NESTING(parentname, childname) \
	{ \
		.child = USB_DESCTYPE_##childname, \
		.parent = USB_DESCTYPE_##parentname, \
	}
#define LAST_NESTING { -1, -1 }

static usb_dp_descriptor_nesting_t descriptor_nesting[] = {
	NESTING(CONFIGURATION, INTERFACE),
	NESTING(INTERFACE, ENDPOINT),
	NESTING(INTERFACE, HUB),
	NESTING(INTERFACE, HID),
	NESTING(HID, HID_REPORT),
	LAST_NESTING
};

static usb_dp_parser_t parser = {
	.nesting = descriptor_nesting
};

void dump_descriptor_tree(uint8_t *descriptors, size_t length)
{
	usb_dp_parser_data_t data = {
		.data = descriptors,
		.size = length,
		.arg = NULL
	};

	dump_tree(&parser, &data);
}

/** @}
 */
