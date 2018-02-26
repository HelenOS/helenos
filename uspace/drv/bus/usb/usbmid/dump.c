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

/** @addtogroup drvusbmid
 * @{
 */
/**
 * @file
 * Dumping and debugging functions.
 */
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <usb/dev/pipes.h>
#include <usb/dev/dp.h>
#include <usb/classes/classes.h>
#include "usbmid.h"

/** Dump found descriptor.
 *
 * @param data Descriptor data.
 * @param depth Nesting depth.
 */
static void dump_tree_descriptor(const uint8_t *data, size_t depth)
{
	if (data == NULL) {
		return;
	}
	const int type = data[1];
	if (type == USB_DESCTYPE_INTERFACE) {
		usb_standard_interface_descriptor_t *descriptor =
		    (usb_standard_interface_descriptor_t *) data;
		usb_log_info("Found interface: %s (0x%02x/0x%02x/0x%02x).",
		    usb_str_class(descriptor->interface_class),
		    (int) descriptor->interface_class,
		    (int) descriptor->interface_subclass,
		    (int) descriptor->interface_protocol);
	}
}

/** Dump tree of descriptors.
 *
 * @param parser Descriptor parser.
 * @param data Descriptor parser data.
 * @param root Pointer to current root.
 * @param depth Nesting depth.
 */
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

/** Dump descriptor tree.
 *
 * @param parser Descriptor parser.
 * @param data Descriptor parser data.
 */
static void dump_tree(usb_dp_parser_t *parser, usb_dp_parser_data_t *data)
{
	const uint8_t *ptr = data->data;
	dump_tree_internal(parser, data, ptr, 0);
}

/** Dump given descriptors.
 *
 * @param descriptors Descriptors buffer (typically full config descriptor).
 * @param length Size of @p descriptors buffer in bytes.
 */
void usbmid_dump_descriptors(uint8_t *descriptors, size_t length)
{
	usb_dp_parser_data_t data = {
		.data = descriptors,
		.size = length,
		.arg = NULL
	};

	usb_dp_parser_t parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};

	dump_tree(&parser, &data);
}

/**
 * @}
 */
