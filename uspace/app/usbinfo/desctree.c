/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbinfo
 * @{
 */
/**
 * @file
 * Descriptor tree dump.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>

#include "usbinfo.h"
#include <usb/dev/dp.h>

static void browse_descriptor_tree_internal(usb_dp_parser_t *parser,
    usb_dp_parser_data_t *data, const uint8_t *root, size_t depth,
    dump_descriptor_in_tree_t callback, void *arg)
{
	if (root == NULL) {
		return;
	}
	callback(root, depth, arg);
	const uint8_t *child = usb_dp_get_nested_descriptor(parser, data, root);
	do {
		browse_descriptor_tree_internal(parser, data, child, depth + 1,
		    callback, arg);
		child = usb_dp_get_sibling_descriptor(parser, data,
		    root, child);
	} while (child != NULL);
}

void browse_descriptor_tree(uint8_t *descriptors, size_t descriptors_size,
    usb_dp_descriptor_nesting_t *descriptor_nesting,
    dump_descriptor_in_tree_t callback, size_t initial_depth, void *arg)
{
	usb_dp_parser_data_t data = {
		.data = descriptors,
		.size = descriptors_size,
		.arg = NULL
	};

	usb_dp_parser_t parser = {
		.nesting = descriptor_nesting
	};

	browse_descriptor_tree_internal(&parser, &data, descriptors,
	    initial_depth, callback, arg);
}

/** @}
 */
