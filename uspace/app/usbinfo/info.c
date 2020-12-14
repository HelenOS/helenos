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
 * Dumping of generic device properties.
 */
#include <stdio.h>
#include <str_error.h>
#include <errno.h>
#include <usb/debug.h>
#include <usb/dev/pipes.h>
#include <usb/dev/recognise.h>
#include <usb/dev/request.h>
#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include "usbinfo.h"

void dump_short_device_identification(usb_device_t *usb_dev)
{
	printf("%sDevice 0x%04x by vendor 0x%04x\n", get_indent(0),
	    usb_device_descriptors(usb_dev)->device.product_id,
	    usb_device_descriptors(usb_dev)->device.vendor_id);
}

static void dump_match_ids_from_interface(
    const uint8_t *descriptor, size_t depth, void *arg)
{
	if (depth != 1) {
		return;
	}
	size_t descr_size = descriptor[0];
	if (descr_size < sizeof(usb_standard_interface_descriptor_t)) {
		return;
	}
	int descr_type = descriptor[1];
	if (descr_type != USB_DESCTYPE_INTERFACE) {
		return;
	}

	usb_device_t *usb_dev = arg;
	assert(usb_dev);

	usb_standard_interface_descriptor_t *iface =
	    (usb_standard_interface_descriptor_t *) descriptor;

	printf("%sInterface #%d match ids (%s, 0x%02x, 0x%02x)\n",
	    get_indent(0),
	    (int) iface->interface_number,
	    usb_str_class(iface->interface_class),
	    (int) iface->interface_subclass,
	    (int) iface->interface_protocol);

	match_id_list_t matches;
	init_match_ids(&matches);
	usb_device_create_match_ids_from_interface(
	    &usb_device_descriptors(usb_dev)->device, iface, &matches);
	dump_match_ids(&matches, get_indent(1));
	clean_match_ids(&matches);
}

void dump_device_match_ids(usb_device_t *usb_dev)
{
	match_id_list_t matches;
	init_match_ids(&matches);
	usb_device_create_match_ids_from_device_descriptor(
	    &usb_device_descriptors(usb_dev)->device, &matches);
	printf("%sDevice match ids (0x%04x by 0x%04x, %s)\n", get_indent(0),
	    usb_device_descriptors(usb_dev)->device.product_id,
	    usb_device_descriptors(usb_dev)->device.vendor_id,
	    usb_str_class(usb_device_descriptors(usb_dev)->device.device_class));
	dump_match_ids(&matches, get_indent(1));
	clean_match_ids(&matches);

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    dump_match_ids_from_interface, usb_dev);
}

static void dump_descriptor_tree_brief_device(const char *prefix,
    usb_standard_device_descriptor_t *descriptor)
{
	printf("%sDevice (0x%04x by 0x%04x, %s, %zu configurations)\n", prefix,
	    (int) descriptor->product_id,
	    (int) descriptor->vendor_id,
	    usb_str_class(descriptor->device_class),
	    (size_t) descriptor->configuration_count);
}

static void dump_descriptor_tree_brief_configuration(const char *prefix,
    usb_standard_configuration_descriptor_t *descriptor)
{
	printf("%sConfiguration #%d (%zu interfaces, total %zuB)\n", prefix,
	    (int) descriptor->configuration_number,
	    (size_t) descriptor->interface_count,
	    (size_t) descriptor->total_length);
}

static void dump_descriptor_tree_brief_interface(const char *prefix,
    usb_standard_interface_descriptor_t *descriptor)
{
	printf("%sInterface #%d (%s, 0x%02x, 0x%02x), alternate %d\n", prefix,
	    (int) descriptor->interface_number,
	    usb_str_class(descriptor->interface_class),
	    (int) descriptor->interface_subclass,
	    (int) descriptor->interface_protocol,
	    (int) descriptor->alternate_setting);
}

static void dump_descriptor_tree_brief_endpoint(const char *prefix,
    usb_standard_endpoint_descriptor_t *descriptor)
{
	usb_endpoint_t endpoint_no = descriptor->endpoint_address & 0xF;
	usb_transfer_type_t transfer = descriptor->attributes & 0x3;
	usb_direction_t direction = descriptor->endpoint_address & 0x80 ?
	    USB_DIRECTION_IN : USB_DIRECTION_OUT;
	printf("%sEndpoint #%d (%s %s, %zu)\n", prefix,
	    endpoint_no, usb_str_transfer_type(transfer),
	    direction == USB_DIRECTION_IN ? "in" : "out",
	    (size_t) descriptor->max_packet_size);
}

static void dump_descriptor_tree_brief_superspeed_endpoint_companion(const char *prefix,
    usb_superspeed_endpoint_companion_descriptor_t *descriptor)
{
	printf("%sSuperspeed endpoint companion\n", prefix);
}

static void dump_descriptor_tree_brief_hid(const char *prefix,
    usb_standard_hid_descriptor_t *descriptor)
{
	printf("%sHID (country %d, %d descriptors)\n", prefix,
	    (int) descriptor->country_code,
	    (int) descriptor->class_desc_count);
}

static void dump_descriptor_tree_brief_hub(const char *prefix,
    usb_hub_descriptor_header_t *descriptor)
{
	printf("%shub (%d ports)\n", prefix,
	    (int) descriptor->port_count);
}

static void dump_descriptor_tree_callback(
    const uint8_t *descriptor, size_t depth, void *arg)
{
	const char *indent = get_indent(depth + 1);

	int descr_type = -1;
	size_t descr_size = descriptor[0];
	if (descr_size > 0) {
		descr_type = descriptor[1];
	}

	switch (descr_type) {

#define _BRANCH(type_enum, descriptor_type, callback) \
	case type_enum: \
		if (descr_size >= sizeof(descriptor_type)) { \
			callback(indent, (descriptor_type *) descriptor); \
			if (arg != NULL) { \
				usb_dump_standard_descriptor(stdout, \
				    get_indent(depth +2), "\n", \
				    descriptor, descr_size); \
			} \
		} else { \
			descr_type = -1; \
		} \
		break;

		_BRANCH(USB_DESCTYPE_DEVICE,
		    usb_standard_device_descriptor_t,
		    dump_descriptor_tree_brief_device);
		_BRANCH(USB_DESCTYPE_CONFIGURATION,
		    usb_standard_configuration_descriptor_t,
		    dump_descriptor_tree_brief_configuration);
		_BRANCH(USB_DESCTYPE_INTERFACE,
		    usb_standard_interface_descriptor_t,
		    dump_descriptor_tree_brief_interface);
		_BRANCH(USB_DESCTYPE_ENDPOINT,
		    usb_standard_endpoint_descriptor_t,
		    dump_descriptor_tree_brief_endpoint);
		_BRANCH(USB_DESCTYPE_SSPEED_EP_COMPANION,
		    usb_superspeed_endpoint_companion_descriptor_t,
		    dump_descriptor_tree_brief_superspeed_endpoint_companion);
		_BRANCH(USB_DESCTYPE_HID,
		    usb_standard_hid_descriptor_t,
		    dump_descriptor_tree_brief_hid);
		/*
		 * Probably useless, hub descriptor shall not be part of
		 * configuration descriptor.
		 */
		_BRANCH(USB_DESCTYPE_HUB,
		    usb_hub_descriptor_header_t,
		    dump_descriptor_tree_brief_hub);

	default:
		break;
	}

	if (descr_type == -1) {
		printf("%sInvalid descriptor.\n", indent);
	}
}

void dump_descriptor_tree_brief(usb_device_t *usb_dev)
{
	dump_descriptor_tree_callback(
	    (const uint8_t *) &usb_device_descriptors(usb_dev)->device,
	    (size_t) -1, NULL);

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    dump_descriptor_tree_callback, NULL);
}

void dump_descriptor_tree_full(usb_device_t *usb_dev)
{
	dump_descriptor_tree_callback(
	    (const uint8_t *) &usb_device_descriptors(usb_dev)->device,
	    (size_t) -1, usb_dev);

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    dump_descriptor_tree_callback, usb_dev);
}

static void find_string_indexes_callback(
    const uint8_t *descriptor, size_t depth, void *arg)
{
	size_t descriptor_length = descriptor[0];
	if (descriptor_length <= 1) {
		return;
	}

#define SET_STRING_INDEX(descr, mask, descr_type, descr_struct, descr_item) \
	do { \
		if ((descr)[1] == (descr_type)) { \
			descr_struct *__type_descr = (descr_struct *) (descr); \
			size_t __str_index = __type_descr->descr_item; \
			if ((__str_index > 0) && (__str_index < 64)) { \
				mask = (mask) | (1 << __str_index); \
			} \
		} \
	} while (0)

	uint64_t *mask = arg;

#define SET_STR(descr_type, descr_struct, descr_item) \
	SET_STRING_INDEX(descriptor, (*mask), descr_type, descr_struct, descr_item)

	SET_STR(USB_DESCTYPE_DEVICE, usb_standard_device_descriptor_t,
	    str_manufacturer);
	SET_STR(USB_DESCTYPE_DEVICE, usb_standard_device_descriptor_t,
	    str_product);
	SET_STR(USB_DESCTYPE_DEVICE, usb_standard_device_descriptor_t,
	    str_serial_number);
	SET_STR(USB_DESCTYPE_CONFIGURATION, usb_standard_configuration_descriptor_t,
	    str_configuration);
	SET_STR(USB_DESCTYPE_INTERFACE, usb_standard_interface_descriptor_t,
	    str_interface);
}

void dump_strings(usb_device_t *usb_dev)
{
	/* Find used indexes. Devices with more than 64 strings are very rare. */
	uint64_t str_mask = 0;
	find_string_indexes_callback(
	    (const uint8_t *) &usb_device_descriptors(usb_dev)->device, 0,
	    &str_mask);

	usb_dp_walk_simple(
	    usb_device_descriptors(usb_dev)->full_config,
	    usb_device_descriptors(usb_dev)->full_config_size,
	    usb_dp_standard_descriptor_nesting,
	    find_string_indexes_callback, &str_mask);

	if (str_mask == 0) {
		printf("Device does not support string descriptors.\n");
		return;
	}

	/* Get supported languages. */
	l18_win_locales_t *langs;
	size_t langs_count;
	errno_t rc = usb_request_get_supported_languages(
	    usb_device_get_default_pipe(usb_dev), &langs, &langs_count);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to get list of supported languages: %s.\n",
		    str_error(rc));
		return;
	}

	printf("%sString languages (%zu):", get_indent(0), langs_count);
	size_t i;
	for (i = 0; i < langs_count; i++) {
		printf(" 0x%04x", (int) langs[i]);
	}
	printf(".\n");

	/* Get all strings and dump them. */
	for (i = 0; i < langs_count; i++) {
		l18_win_locales_t lang = langs[i];

		printf("%sStrings in %s:\n", get_indent(0),
		    str_l18_win_locale(lang));

		size_t idx;
		for (idx = 1; idx < 64; idx++) {
			if ((str_mask & ((uint64_t)1 << idx)) == 0) {
				continue;
			}
			char *string = NULL;
			rc = usb_request_get_string(
			    usb_device_get_default_pipe(usb_dev), idx, lang,
			    &string);
			if ((rc != EOK) && (rc != EEMPTY)) {
				printf("%sWarn: failed to retrieve string #%zu: %s.\n",
				    get_indent(1), idx, str_error(rc));
				continue;
			}
			printf("%sString #%zu: \"%s\"\n", get_indent(1),
			    idx, rc == EOK ? string : "");
			if (string != NULL) {
				free(string);
			}
		}
	}
}

void dump_status(usb_device_t *usb_dev)
{
	errno_t rc;
	uint16_t status = 0;

	/* Device status first. */
	rc = usb_request_get_status(usb_device_get_default_pipe(usb_dev),
	    USB_REQUEST_RECIPIENT_DEVICE, 0, &status);
	if (rc != EOK) {
		printf("%sFailed to get device status: %s.\n",
		    get_indent(0), str_error(rc));
	} else {
		printf("%sDevice status 0x%04x: power=%s, remote-wakeup=%s.\n",
		    get_indent(0), status,
		    status & USB_DEVICE_STATUS_SELF_POWERED ? "self" : "bus",
		    status & USB_DEVICE_STATUS_REMOTE_WAKEUP ? "yes" : "no");
	}

	/* Interface is not interesting, skipping ;-). */

	/* Control endpoint zero. */
	status = 0;
	rc = usb_request_get_status(usb_device_get_default_pipe(usb_dev),
	    USB_REQUEST_RECIPIENT_ENDPOINT, 0, &status);
	if (rc != EOK) {
		printf("%sFailed to get control endpoint status: %s.\n",
		    get_indent(0), str_error(rc));
	} else {
		printf("%sControl endpoint zero status %04X: halted=%s.\n",
		    get_indent(0), status,
		    status & USB_ENDPOINT_STATUS_HALTED ? "yes" : "no");
	}
}

/** @}
 */
