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

/** @addtogroup libusbdev
 * @{
 */
/**
 * @file
 * USB descriptor parser (implementation).
 *
 * The descriptor parser is a generic parser for structure, where individual
 * items are stored in single buffer and each item begins with length followed
 * by type. These types are organized into tree hierarchy.
 *
 * The parser is able of only two actions: find first child and find next
 * sibling.
 */
#include <usb/dev/dp.h>
#include <usb/descriptor.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NESTING(parentname, childname) \
	{ \
		.child = USB_DESCTYPE_##childname, \
		.parent = USB_DESCTYPE_##parentname, \
	}
#define LAST_NESTING { -1, -1 }

/** Nesting of standard USB descriptors. */
const usb_dp_descriptor_nesting_t usb_dp_standard_descriptor_nesting[] = {
	NESTING(CONFIGURATION, INTERFACE),
	NESTING(INTERFACE, ENDPOINT),
	NESTING(ENDPOINT, SSPEED_EP_COMPANION),
	NESTING(INTERFACE, HUB),
	NESTING(INTERFACE, HID),
	NESTING(HID, HID_REPORT),
	LAST_NESTING
};

#undef NESTING
#undef LAST_NESTING

/** Tells whether pointer points inside descriptor data.
 *
 * @param data Parser data.
 * @param ptr Pointer to be verified.
 * @return Whether @p ptr points inside <code>data->data</code> field.
 */
static bool is_valid_descriptor_pointer(const usb_dp_parser_data_t *data,
    const uint8_t *ptr)
{
	if (ptr == NULL) {
		return false;
	}

	if (ptr < data->data) {
		return false;
	}

	if ((size_t)(ptr - data->data) >= data->size) {
		return false;
	}

	return true;
}

/** Get next descriptor regardless of the nesting.
 *
 * @param data Parser data.
 * @param current Pointer to current descriptor.
 * @return Pointer to start of next descriptor.
 * @retval NULL Invalid input or no next descriptor.
 */
static const uint8_t *get_next_descriptor(const usb_dp_parser_data_t *data,
    const uint8_t *current)
{
	assert(is_valid_descriptor_pointer(data, current));

	const uint8_t current_length = *current;
	const uint8_t *next = current + current_length;

	if (!is_valid_descriptor_pointer(data, next)) {
		return NULL;
	}

	return next;
}

/** Get descriptor type.
 *
 * @see usb_descriptor_type_t
 *
 * @param data Parser data.
 * @param start Pointer to start of the descriptor.
 * @return Descriptor type.
 * @retval -1 Invalid input.
 */
static int get_descriptor_type(const usb_dp_parser_data_t *data,
    const uint8_t *start)
{
	if (start == NULL) {
		return -1;
	}

	start++;
	if (!is_valid_descriptor_pointer(data, start)) {
		return -1;
	} else {
		return (int) (*start);
	}
}

/** Tells whether descriptors could be nested.
 *
 * @param parser Parser.
 * @param child Child descriptor type.
 * @param parent Parent descriptor type.
 * @return Whether @p child could be child of @p parent.
 */
static bool is_nested_descriptor_type(const usb_dp_parser_t *parser,
    int child, int parent)
{
	const usb_dp_descriptor_nesting_t *nesting = parser->nesting;
	while ((nesting->child > 0) && (nesting->parent > 0)) {
		if ((nesting->child == child) && (nesting->parent == parent)) {
			return true;
		}
		nesting++;
	}
	return false;
}

/** Tells whether descriptors could be nested.
 *
 * @param parser Parser.
 * @param data Parser data.
 * @param child Pointer to child descriptor.
 * @param parent Pointer to parent descriptor.
 * @return Whether @p child could be child of @p parent.
 */
static bool is_nested_descriptor(const usb_dp_parser_t *parser,
    const usb_dp_parser_data_t *data, const uint8_t *child, const uint8_t *parent)
{
	return is_nested_descriptor_type(parser,
	    get_descriptor_type(data, child),
	    get_descriptor_type(data, parent));
}

/** Find first nested descriptor of given parent.
 *
 * @param parser Parser.
 * @param data Parser data.
 * @param parent Pointer to the beginning of parent descriptor.
 * @return Pointer to the beginning of the first nested (child) descriptor.
 * @retval NULL No child descriptor found.
 * @retval NULL Invalid input.
 */
const uint8_t *usb_dp_get_nested_descriptor(const usb_dp_parser_t *parser,
    const usb_dp_parser_data_t *data, const uint8_t *parent)
{
	if (!is_valid_descriptor_pointer(data, parent)) {
		return NULL;
	}

	const uint8_t *next = get_next_descriptor(data, parent);
	if (next == NULL) {
		return NULL;
	}

	if (is_nested_descriptor(parser, data, next, parent)) {
		return next;
	} else {
		return NULL;
	}
}

/** Skip all nested descriptors.
 *
 * @param parser Parser.
 * @param data Parser data.
 * @param parent Pointer to the beginning of parent descriptor.
 * @return Pointer to first non-child descriptor.
 * @retval NULL No next descriptor.
 * @retval NULL Invalid input.
 */
static const uint8_t *skip_nested_descriptors(const usb_dp_parser_t *parser,
    const usb_dp_parser_data_t *data, const uint8_t *parent)
{
	const uint8_t *child =
	    usb_dp_get_nested_descriptor(parser, data, parent);
	if (child == NULL) {
		return get_next_descriptor(data, parent);
	}
	const uint8_t *next_child =
	    skip_nested_descriptors(parser, data, child);
	while (is_nested_descriptor(parser, data, next_child, parent)) {
		next_child = skip_nested_descriptors(parser, data, next_child);
	}

	return next_child;
}

/** Get sibling descriptor.
 *
 * @param parser Parser.
 * @param data Parser data.
 * @param parent Pointer to common parent descriptor.
 * @param sibling Left sibling.
 * @return Pointer to first right sibling of @p sibling.
 * @retval NULL No sibling exist.
 * @retval NULL Invalid input.
 */
const uint8_t *usb_dp_get_sibling_descriptor(
    const usb_dp_parser_t *parser, const usb_dp_parser_data_t *data,
    const uint8_t *parent, const uint8_t *sibling)
{
	if (!is_valid_descriptor_pointer(data, parent)
	    || !is_valid_descriptor_pointer(data, sibling)) {
		return NULL;
	}

	const uint8_t *possible_sibling =
	    skip_nested_descriptors(parser, data, sibling);
	if (possible_sibling == NULL) {
		return NULL;
	}

	int parent_type = get_descriptor_type(data, parent);
	int possible_sibling_type = get_descriptor_type(data, possible_sibling);
	if (is_nested_descriptor_type(parser,
		    possible_sibling_type, parent_type)) {
		return possible_sibling;
	} else {
		return NULL;
	}
}

/** Browser of the descriptor tree.
 *
 * @see usb_dp_walk_simple
 *
 * @param parser Descriptor parser.
 * @param data Data for descriptor parser.
 * @param root Pointer to current root of the tree.
 * @param depth Current nesting depth.
 * @param callback Callback for each found descriptor.
 * @param arg Custom (user) argument.
 */
static void usb_dp_browse_simple_internal(const usb_dp_parser_t *parser,
    const usb_dp_parser_data_t *data, const uint8_t *root, size_t depth,
    void (*callback)(const uint8_t *, size_t, void *), void *arg)
{
	if (root == NULL) {
		return;
	}
	callback(root, depth, arg);
	const uint8_t *child = usb_dp_get_nested_descriptor(parser, data, root);
	do {
		usb_dp_browse_simple_internal(parser, data, child, depth + 1,
		    callback, arg);
		child = usb_dp_get_sibling_descriptor(parser, data,
		    root, child);
	} while (child != NULL);
}

/** Browse flatten descriptor tree.
 *
 * The callback is called with following arguments: pointer to the start
 * of the descriptor (somewhere inside @p descriptors), depth of the nesting
 * (starting from 0 for the first descriptor) and the custom argument.
 * Note that the size of the descriptor is not passed because it can
 * be read from the first byte of the descriptor.
 *
 * @param descriptors Descriptor data.
 * @param descriptors_size Size of descriptor data (in bytes).
 * @param descriptor_nesting Possible descriptor nesting.
 * @param callback Callback for each found descriptor.
 * @param arg Custom (user) argument.
 */
void usb_dp_walk_simple(const uint8_t *descriptors, size_t descriptors_size,
    const usb_dp_descriptor_nesting_t *descriptor_nesting,
    walk_callback_t callback, void *arg)
{
	if ((descriptors == NULL) || (descriptors_size == 0)
	    || (descriptor_nesting == NULL) || (callback == NULL)) {
		return;
	}

	const usb_dp_parser_data_t data = {
		.data = descriptors,
		.size = descriptors_size,
		.arg = NULL
	};

	const usb_dp_parser_t parser = {
		.nesting = descriptor_nesting
	};

	usb_dp_browse_simple_internal(&parser, &data, descriptors,
	    0, callback, arg);
}

/** @}
 */
