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
#include <usb/pipes.h>
#include <usb/recognise.h>
#include <usb/request.h>
#include <usb/classes/classes.h>
#include "usbinfo.h"

void dump_short_device_identification(usbinfo_device_t *dev)
{
	printf("%sDevice 0x%04x by vendor 0x%04x\n", get_indent(0),
	    (int) dev->device_descriptor.product_id,
	    (int) dev->device_descriptor.vendor_id);
}

static void dump_match_ids_from_interface(uint8_t *descriptor, size_t depth,
    void *arg)
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

	usbinfo_device_t *dev = (usbinfo_device_t *) arg;

	usb_standard_interface_descriptor_t *iface
	    = (usb_standard_interface_descriptor_t *) descriptor;

	printf("%sInterface #%d match ids (%s, 0x%02x, 0x%02x)\n",
	    get_indent(0),
	    (int) iface->interface_number,
	    usb_str_class(iface->interface_class),
	    (int) iface->interface_subclass,
	    (int) iface->interface_protocol);

	match_id_list_t matches;
	init_match_ids(&matches);
	usb_device_create_match_ids_from_interface(&dev->device_descriptor,
	    iface, &matches);
	dump_match_ids(&matches, get_indent(1));
	clean_match_ids(&matches);
}

void dump_device_match_ids(usbinfo_device_t *dev)
{
	match_id_list_t matches;
	init_match_ids(&matches);
	usb_device_create_match_ids_from_device_descriptor(
	    &dev->device_descriptor, &matches);
	printf("%sDevice match ids (0x%04x by 0x%04x, %s)\n", get_indent(0),
	    (int) dev->device_descriptor.product_id,
	    (int) dev->device_descriptor.vendor_id,
	    usb_str_class(dev->device_descriptor.device_class));
	dump_match_ids(&matches, get_indent(1));
	clean_match_ids(&matches);

	usb_dp_walk_simple(dev->full_configuration_descriptor,
	    dev->full_configuration_descriptor_size,
	    usb_dp_standard_descriptor_nesting,
	    dump_match_ids_from_interface,
	    dev);
}

static void dump_descriptor_tree_brief_device(const char *prefix,
    usb_standard_device_descriptor_t *descriptor)
{
	printf("%sDevice (0x%04x by 0x%04x, %s)\n", prefix,
	    (int) descriptor->product_id,
	    (int) descriptor->vendor_id,
	    usb_str_class(descriptor->device_class));
}

static void dump_descriptor_tree_brief_configuration(const char *prefix,
    usb_standard_configuration_descriptor_t *descriptor)
{
	printf("%sConfiguration #%d\n", prefix,
	    (int) descriptor->configuration_number);
}

static void dump_descriptor_tree_brief_interface(const char *prefix,
    usb_standard_interface_descriptor_t *descriptor)
{
	printf("%sInterface #%d (%s, 0x%02x, 0x%02x)\n", prefix,
	    (int) descriptor->interface_number,
	    usb_str_class(descriptor->interface_class),
	    (int) descriptor->interface_subclass,
	    (int) descriptor->interface_protocol);
}

static void dump_descriptor_tree_brief_endpoint(const char *prefix,
    usb_standard_endpoint_descriptor_t *descriptor)
{
	usb_endpoint_t endpoint_no = descriptor->endpoint_address & 0xF;
	usb_transfer_type_t transfer = descriptor->attributes & 0x3;
	usb_direction_t direction = descriptor->endpoint_address & 0x80
	    ? USB_DIRECTION_IN : USB_DIRECTION_OUT;
	printf("%sEndpoint #%d (%s %s, %zu)\n", prefix,
	    endpoint_no, usb_str_transfer_type(transfer),
	    direction == USB_DIRECTION_IN ? "in" : "out",
	    (size_t) descriptor->max_packet_size);
}


static void dump_descriptor_tree_brief_callback(uint8_t *descriptor,
    size_t depth, void *arg)
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

		default:
			break;
	}

	if (descr_type == -1) {
		printf("%sInvalid descriptor.\n", indent);
	}
}

void dump_descriptor_tree_brief(usbinfo_device_t *dev)
{
	dump_descriptor_tree_brief_callback((uint8_t *)&dev->device_descriptor,
	    (size_t) -1, NULL);
	usb_dp_walk_simple(dev->full_configuration_descriptor,
	    dev->full_configuration_descriptor_size,
	    usb_dp_standard_descriptor_nesting,
	    dump_descriptor_tree_brief_callback,
	    NULL);
}

int dump_device(devman_handle_t hc_handle, usb_address_t address)
{
	int rc;
	usb_device_connection_t wire;
	usb_endpoint_pipe_t ctrl_pipe;

	/*
	 * Initialize pipes.
	 */
	rc = usb_device_connection_initialize(&wire, hc_handle, address);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to create connection to the device: %s.\n",
		    str_error(rc));
		goto leave;
	}
	rc = usb_endpoint_pipe_initialize_default_control(&ctrl_pipe, &wire);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to create default control pipe: %s.\n",
		    str_error(rc));
		goto leave;
	}
	rc = usb_endpoint_pipe_probe_default_control(&ctrl_pipe);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": probing default control pipe failed: %s.\n",
		    str_error(rc));
		goto leave;
	}
	rc = usb_endpoint_pipe_start_session(&ctrl_pipe);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to start session on control pipe: %s.\n",
		    str_error(rc));
		goto leave;
	}

	/*
	 * Dump information about possible match ids.
	 */
	match_id_list_t match_id_list;
	init_match_ids(&match_id_list);
	rc = usb_device_create_match_ids(&ctrl_pipe, &match_id_list);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch match ids of the device: %s.\n",
		    str_error(rc));
		goto leave;
	}
	dump_match_ids(&match_id_list, get_indent(0));

	/*
	 * Get device descriptor and dump it.
	 */
	usb_standard_device_descriptor_t device_descriptor;
	rc = usb_request_get_device_descriptor(&ctrl_pipe, &device_descriptor);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch standard device descriptor: %s.\n",
		    str_error(rc));
		goto leave;
	}
	dump_usb_descriptor((uint8_t *)&device_descriptor, sizeof(device_descriptor));

	rc = EOK;
	goto leave;

	/*
	 * Get first configuration descriptor and dump it.
	 */
	usb_standard_configuration_descriptor_t config_descriptor;
	int config_index = 0;
	rc = usb_request_get_bare_configuration_descriptor(&ctrl_pipe,
	    config_index, &config_descriptor);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch standard configuration descriptor: %s.\n",
		    str_error(rc));
		goto leave;
	}
	//dump_standard_configuration_descriptor(config_index, &config_descriptor);

	void *full_config_descriptor = malloc(config_descriptor.total_length);
	rc = usb_request_get_full_configuration_descriptor(&ctrl_pipe,
	    config_index,
	    full_config_descriptor, config_descriptor.total_length, NULL);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch full configuration descriptor: %s.\n",
		    str_error(rc));
		goto leave;
	}

	dump_descriptor_tree(full_config_descriptor,
	    config_descriptor.total_length);

	/*
	 * Get supported languages of STRING descriptors.
	 */
	l18_win_locales_t *langs;
	size_t langs_count;
	rc = usb_request_get_supported_languages(&ctrl_pipe,
	    &langs, &langs_count);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to get list of supported languages: %s.\n",
		    str_error(rc));
		goto skip_strings;
	}

	printf("String languages (%zu):", langs_count);
	size_t i;
	for (i = 0; i < langs_count; i++) {
		printf(" 0x%04x", (int) langs[i]);
	}
	printf(".\n");

	/*
	 * Dump all strings in all available langages;
	 */
	for (i = 0; i < langs_count; i++) {
		l18_win_locales_t lang = langs[i];

		printf("%sStrings for language 0x%04x:\n", get_indent(0),
		    (int) lang);

		/*
		 * Try all indexes - we will see what pops-up ;-).
		 * However, to speed things up, we will stop after
		 * encountering several broken (or nonexistent ones)
		 * descriptors in line.
		 */
		size_t idx;
		size_t failed_count = 0;
		for (idx = 1; idx < 0xFF; idx++) {
			char *string;
			rc = usb_request_get_string(&ctrl_pipe, idx, lang,
			    &string);
			if (rc != EOK) {
				failed_count++;
				if (failed_count > 3) {
					break;
				}
				continue;
			}
			printf("%sString #%zu: \"%s\"\n", get_indent(1),
			    idx, string);
			free(string);
			failed_count = 0; /* Reset failed counter. */
		}
	}


skip_strings:

	rc = EOK;

leave:
	/* Ignoring errors here. */
	usb_endpoint_pipe_end_session(&ctrl_pipe);

	return rc;
}

/** @}
 */
