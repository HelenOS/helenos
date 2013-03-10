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
