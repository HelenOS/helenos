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

/** @addtogroup usbinfo
 * @{
 */
/**
 * @file
 * USB querying.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>

#include "usbinfo.h"
#include <usb/dev/dp.h>

#define INDENT "  "
#define BYTES_PER_LINE 12

const char *get_indent(size_t level)
{
	static const char *indents[] = {
		INDENT,
		INDENT INDENT,
		INDENT INDENT INDENT,
		INDENT INDENT INDENT INDENT,
		INDENT INDENT INDENT INDENT INDENT,
		INDENT INDENT INDENT INDENT INDENT INDENT,
	};
	static size_t indents_count = sizeof(indents) / sizeof(indents[0]);
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
		if (((i > 0) && (((i + 1) % BYTES_PER_LINE) == 0)) ||
		    (i + 1 == length)) {
			printf("\n");
			if (i + 1 < length) {
				printf("%s", get_indent(indent));
			}
		} else {
			printf("  ");
		}
	}
}

void dump_usb_descriptor(uint8_t *descriptor, size_t size)
{
	printf("Device descriptor:\n");
	usb_dump_standard_descriptor(stdout, get_indent(0), "\n",
	    descriptor, size);
}

void dump_match_ids(match_id_list_t *matches, const char *line_prefix)
{
	list_foreach(matches->ids, link, match_id_t, match) {
		printf("%s%3d %s\n", line_prefix, match->score, match->id);
	}
}

static void dump_tree_descriptor(const uint8_t *descriptor, size_t depth)
{
	if (descriptor == NULL) {
		return;
	}
	int type = descriptor[1];
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
	usb_dump_standard_descriptor(stdout, get_indent(depth), "\n",
	    descriptor, descriptor[0]);
}

static void dump_tree_internal(
    usb_dp_parser_t *parser, usb_dp_parser_data_t *data,
    const uint8_t *root, size_t depth)
{
	if (root == NULL) {
		return;
	}
	dump_tree_descriptor(root, depth);
	const uint8_t *child = usb_dp_get_nested_descriptor(parser, data, root);
	do {
		dump_tree_internal(parser, data, child, depth + 1);
		child = usb_dp_get_sibling_descriptor(parser, data, root, child);
	} while (child != NULL);
}

static void dump_tree(usb_dp_parser_t *parser, usb_dp_parser_data_t *data)
{
	const uint8_t *ptr = data->data;
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
