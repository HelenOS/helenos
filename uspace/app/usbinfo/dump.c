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
#include <usb/classes/classes.h>

#include "usbinfo.h"
#include <usb/dp.h>

#define INDENT "  "
#define PRINTLINE(indent, fmt, ...) printf("%s - " fmt, get_indent(indent), __VA_ARGS__)
#define BYTES_PER_LINE 12

#define BCD_INT(a) (((unsigned int)(a)) / 256)
#define BCD_FRAC(a) (((unsigned int)(a)) % 256)

#define BCD_FMT "%x.%x"
#define BCD_ARGS(a) BCD_INT((a)), BCD_FRAC((a))

static void dump_descriptor_by_type(size_t, uint8_t *, size_t);

typedef struct {
	int descriptor;
	void (*dump)(size_t indent, uint8_t *descriptor, size_t length);
} descriptor_dump_t;

static void dump_descriptor_device(size_t, uint8_t *, size_t);
static void dump_descriptor_configuration(size_t, uint8_t *, size_t);
static void dump_descriptor_interface(size_t, uint8_t *, size_t);
static void dump_descriptor_string(size_t, uint8_t *, size_t);
static void dump_descriptor_endpoint(size_t, uint8_t *, size_t);
static void dump_descriptor_hid(size_t, uint8_t *, size_t);
static void dump_descriptor_hub(size_t, uint8_t *, size_t);
static void dump_descriptor_generic(size_t, uint8_t *, size_t);

static descriptor_dump_t descriptor_dumpers[] = {
	{ USB_DESCTYPE_DEVICE, dump_descriptor_device },
	{ USB_DESCTYPE_CONFIGURATION, dump_descriptor_configuration },
	{ USB_DESCTYPE_STRING, dump_descriptor_string },
	{ USB_DESCTYPE_INTERFACE, dump_descriptor_interface },
	{ USB_DESCTYPE_ENDPOINT, dump_descriptor_endpoint },
	{ USB_DESCTYPE_HID, dump_descriptor_hid },
	{ USB_DESCTYPE_HUB, dump_descriptor_hub },
	{ -1, dump_descriptor_generic },
	{ -1, NULL }
};

static const char *get_indent(size_t level)
{
	static const char *indents[] = {
		INDENT,
		INDENT INDENT,
		INDENT INDENT INDENT,
		INDENT INDENT INDENT INDENT,
		INDENT INDENT INDENT INDENT INDENT
	};
	static size_t indents_count = sizeof(indents)/sizeof(indents[0]);
	if (level >= indents_count) {
		return indents[indents_count - 1];
	}
	return indents[level];
}

void dump_buffer(const char *msg, size_t indent,
    const uint8_t *buffer, size_t length)
{
	if (msg != NULL) {
		printf("%s\n", msg);
	}

	size_t i;
	if (length > 0) {
		printf("%s", get_indent(indent));
	}
	for (i = 0; i < length; i++) {
		printf("0x%02X", buffer[i]);
		if (((i > 0) && (((i+1) % BYTES_PER_LINE) == 0))
		    || (i + 1 == length)) {
			printf("\n");
			if (i + 1 < length) {
				printf("%s", get_indent(indent));
			}
		} else {
			printf("  ");
		}
	}
}

void dump_descriptor_by_type(size_t indent, uint8_t *d, size_t length)
{
	if (length < 2) {
		return;
	}
	int type = d[1];
	
	descriptor_dump_t *dumper = descriptor_dumpers;
	while (dumper->dump != NULL) {
		if ((dumper->descriptor == type) || (dumper->descriptor < 0)) {
			dumper->dump(indent, d, length);
			return;
		}
		dumper++;
	}			
}

void dump_usb_descriptor(uint8_t *descriptor, size_t size)
{
	dump_descriptor_by_type(0, descriptor, size);
}

void dump_descriptor_device(size_t indent, uint8_t *descr, size_t size)
{
	usb_standard_device_descriptor_t *d
	    = (usb_standard_device_descriptor_t *) descr;
	if (size != sizeof(*d)) {
		return;
	}
	
	PRINTLINE(indent, "bLength = %d\n", d->length);
	PRINTLINE(indent, "bDescriptorType = 0x%02x\n", d->descriptor_type);
	PRINTLINE(indent, "bcdUSB = %d (" BCD_FMT ")\n", d->usb_spec_version,
	    BCD_ARGS(d->usb_spec_version));
	PRINTLINE(indent, "bDeviceClass = 0x%02x\n", d->device_class);
	PRINTLINE(indent, "bDeviceSubClass = 0x%02x\n", d->device_subclass);
	PRINTLINE(indent, "bDeviceProtocol = 0x%02x\n", d->device_protocol);
	PRINTLINE(indent, "bMaxPacketSize0 = %d\n", d->max_packet_size);
	PRINTLINE(indent, "idVendor = %d\n", d->vendor_id);
	PRINTLINE(indent, "idProduct = %d\n", d->product_id);
	PRINTLINE(indent, "bcdDevice = %d\n", d->device_version);
	PRINTLINE(indent, "iManufacturer = %d\n", d->str_manufacturer);
	PRINTLINE(indent, "iProduct = %d\n", d->str_product);
	PRINTLINE(indent, "iSerialNumber = %d\n", d->str_serial_number);
	PRINTLINE(indent, "bNumConfigurations = %d\n", d->configuration_count);
}

void dump_descriptor_configuration(size_t indent, uint8_t *descr, size_t size)
{
	usb_standard_configuration_descriptor_t *d
	    = (usb_standard_configuration_descriptor_t *) descr;
	if (size != sizeof(*d)) {
		return;
	}
	
	bool self_powered = d->attributes & 64;
	bool remote_wakeup = d->attributes & 32;
	
	PRINTLINE(indent, "bLength = %d\n", d->length);
	PRINTLINE(indent, "bDescriptorType = 0x%02x\n", d->descriptor_type);
	PRINTLINE(indent, "wTotalLength = %d\n", d->total_length);
	PRINTLINE(indent, "bNumInterfaces = %d\n", d->interface_count);
	PRINTLINE(indent, "bConfigurationValue = %d\n", d->configuration_number);
	PRINTLINE(indent, "iConfiguration = %d\n", d->str_configuration);
	PRINTLINE(indent, "bmAttributes = %d [%s%s%s]\n", d->attributes,
	    self_powered ? "self-powered" : "",
	    (self_powered & remote_wakeup) ? ", " : "",
	    remote_wakeup ? "remote-wakeup" : "");
	PRINTLINE(indent, "MaxPower = %d (%dmA)\n", d->max_power,
	    2 * d->max_power);
}

void dump_descriptor_interface(size_t indent, uint8_t *descr, size_t size)
{
	usb_standard_interface_descriptor_t *d
	    = (usb_standard_interface_descriptor_t *) descr;
	if (size != sizeof(*d)) {
		return;
	}
	
	PRINTLINE(indent, "bLength = %d\n", d->length);
	PRINTLINE(indent, "bDescriptorType = 0x%02x\n", d->descriptor_type);
	PRINTLINE(indent, "bInterfaceNumber = %d\n", d->interface_number);
	PRINTLINE(indent, "bAlternateSetting = %d\n", d->alternate_setting);
	PRINTLINE(indent, "bNumEndpoints = %d\n", d->endpoint_count);
	PRINTLINE(indent, "bInterfaceClass = %s\n", d->interface_class == 0
	    ? "reserved (0)" : usb_str_class(d->interface_class));
	PRINTLINE(indent, "bInterfaceSubClass = %d\n", d->interface_subclass);
	PRINTLINE(indent, "bInterfaceProtocol = %d\n", d->interface_protocol);
	PRINTLINE(indent, "iInterface = %d\n", d->str_interface);
}

void dump_descriptor_string(size_t indent, uint8_t *descr, size_t size)
{
	dump_descriptor_generic(indent, descr, size);
}

void dump_descriptor_endpoint(size_t indent, uint8_t *descr, size_t size)
{
	usb_standard_endpoint_descriptor_t *d
	   = (usb_standard_endpoint_descriptor_t *) descr;
	if (size != sizeof(*d)) {
		return;
	}
	
	int endpoint = d->endpoint_address & 15;
	usb_direction_t direction = d->endpoint_address & 128
	    ? USB_DIRECTION_IN : USB_DIRECTION_OUT;
	
	PRINTLINE(indent, "bLength = %d\n", d->length);
	PRINTLINE(indent, "bDescriptorType = 0x%02X\n", d->descriptor_type);
	PRINTLINE(indent, "bEndpointAddress = 0x%02X [%d, %s]\n",
	    d->endpoint_address, endpoint,
	    direction == USB_DIRECTION_IN ? "in" : "out");
	PRINTLINE(indent, "bmAttributes = %d\n", d->attributes);
	PRINTLINE(indent, "wMaxPacketSize = %d\n", d->max_packet_size);
	PRINTLINE(indent, "bInterval = %dms\n", d->poll_interval);
}

void dump_descriptor_hid(size_t indent, uint8_t *descr, size_t size)
{
	dump_descriptor_generic(indent, descr, size);
}

void dump_descriptor_hub(size_t indent, uint8_t *descr, size_t size)
{
	dump_descriptor_generic(indent, descr, size);
}

void dump_descriptor_generic(size_t indent, uint8_t *descr, size_t size)
{
	dump_buffer(NULL, indent, descr, size);
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

static void dump_tree_descriptor(uint8_t *descriptor, size_t depth)
{
	if (descriptor == NULL) {
		return;
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
	printf("%s%s (0x%02X):\n", get_indent(depth), name, type);
	dump_descriptor_by_type(depth, descriptor, descriptor[0]);
	
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
	dump_tree_internal(parser, data, ptr, 0);
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
