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

/** @addtogroup libusb
 * @{
 */
/**
 * @file
 * @brief USB descriptor parser (implementation).
 */
#include <stdio.h>
#include <str_error.h>
#include <errno.h>
#include <assert.h>
#include <bool.h>
#include <usb/dp.h>
#include <usb/descriptor.h>

#define NESTING(parentname, childname) \
	{ \
		.child = USB_DESCTYPE_##childname, \
		.parent = USB_DESCTYPE_##parentname, \
	}
#define LAST_NESTING { -1, -1 }

/** Nesting of standard USB descriptors. */
usb_dp_descriptor_nesting_t usb_dp_standard_descriptor_nesting[] = {
	NESTING(CONFIGURATION, INTERFACE),
	NESTING(INTERFACE, ENDPOINT),
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
static bool is_valid_descriptor_pointer(usb_dp_parser_data_t *data,
    uint8_t *ptr)
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
static uint8_t *get_next_descriptor(usb_dp_parser_data_t *data,
    uint8_t *current)
{
	assert(is_valid_descriptor_pointer(data, current));

	uint8_t current_length = *current;
	uint8_t *next = current + current_length;

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
static int get_descriptor_type(usb_dp_parser_data_t *data, uint8_t *start)
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
static bool is_nested_descriptor_type(usb_dp_parser_t *parser,
    int child, int parent)
{
	usb_dp_descriptor_nesting_t *nesting = parser->nesting;
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
static bool is_nested_descriptor(usb_dp_parser_t *parser,
    usb_dp_parser_data_t *data, uint8_t *child, uint8_t *parent)
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
uint8_t *usb_dp_get_nested_descriptor(usb_dp_parser_t *parser,
    usb_dp_parser_data_t *data, uint8_t *parent)
{
	if (!is_valid_descriptor_pointer(data, parent)) {
		return NULL;
	}

	uint8_t *next = get_next_descriptor(data, parent);
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
static uint8_t *skip_nested_descriptors(usb_dp_parser_t *parser,
    usb_dp_parser_data_t *data, uint8_t *parent)
{
	uint8_t *child = usb_dp_get_nested_descriptor(parser, data, parent);
	if (child == NULL) {
		return get_next_descriptor(data, parent);
	}
	uint8_t *next_child = skip_nested_descriptors(parser, data, child);
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
uint8_t *usb_dp_get_sibling_descriptor(usb_dp_parser_t *parser,
    usb_dp_parser_data_t *data, uint8_t *parent, uint8_t *sibling)
{
	if (!is_valid_descriptor_pointer(data, parent)
	    || !is_valid_descriptor_pointer(data, sibling)) {
		return NULL;
	}

	uint8_t *possible_sibling = skip_nested_descriptors(parser, data, sibling);
	if (possible_sibling == NULL) {
		return NULL;
	}

	int parent_type = get_descriptor_type(data, parent);
	int possible_sibling_type = get_descriptor_type(data, possible_sibling);
	if (is_nested_descriptor_type(parser, possible_sibling_type, parent_type)) {
		return possible_sibling;
	} else {
		return NULL;
	}
}


/** @}
 */
