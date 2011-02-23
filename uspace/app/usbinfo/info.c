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
#include "usbinfo.h"

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
	dump_match_ids(&match_id_list);

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
	 * Dump STRING languages.
	 */
	uint8_t *string_descriptor = NULL;
	size_t string_descriptor_size = 0;
	rc = usb_request_get_descriptor_alloc(&ctrl_pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_STRING, 0, 0,
	    (void **) &string_descriptor, &string_descriptor_size);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch language table: %s.\n",
		    str_error(rc));
		goto leave;
	}
	if ((string_descriptor_size <= 2)
	    || ((string_descriptor_size % 2) != 0)) {
		fprintf(stderr, NAME ": No supported languages.\n");
		goto leave;
	}

	size_t lang_count = (string_descriptor_size - 2) / 2;
	int *lang_codes = malloc(sizeof(int) * lang_count);
	size_t i;
	for (i = 0; i < lang_count; i++) {
		lang_codes[i] = (string_descriptor[2 + 2 * i + 1] << 8)
		    + string_descriptor[2 + 2 * i];
	}

	printf("String languages:");
	for (i = 0; i < lang_count; i++) {
		printf(" 0x%04x", lang_codes[i]);
	}
	printf(".\n");

	free(string_descriptor);
	free(lang_codes);

	/*
	 * Dump all strings in English (0x0409).
	 */
	uint16_t lang = 0x0409;
	/*
	 * Up to 3 string in device descriptor, 1 for configuration and
	 * one for each interface.
	 */
	size_t max_idx = 3 + 1 + config_descriptor.interface_count;
	size_t idx;
	for (idx = 1; idx <= max_idx; idx++) {
		uint8_t *string;
		size_t string_size;
		rc = usb_request_get_descriptor_alloc(&ctrl_pipe,
		    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_STRING,
		    idx, uint16_host2usb(lang),
		    (void **) &string, &string_size);
		if (rc == EINTR) {
			/* Invalid index, skip silently. */
			continue;
		}
		if (rc != EOK) {
			fprintf(stderr,
			    NAME ": failed to fetch string #%zu " \
			    "(lang 0x%04x): %s.\n",
			    idx, (int) lang, str_error(rc));
			continue;
		}
		/*
		printf("String #%zu (language 0x%04x):\n", idx, (int) lang);
		dump_buffer(NULL, 0,
		    string, string_size);
		*/
		if (string_size <= 2) {
			fprintf(stderr,
			    NAME ": no string for index #%zu.\n", idx);
			free(string);
			continue;
		}
		string_size -= 2;

		size_t string_char_count = string_size / 2;
		wchar_t *string_chars = malloc(sizeof(wchar_t) * (string_char_count + 1));
		assert(string_chars != NULL);
		for (i = 0; i < string_char_count; i++) {
			uint16_t uni_char = (string[2 + 2 * i + 1] << 8)
			    + string[2 + 2 * i];
			string_chars[i] = uni_char;
		}
		string_chars[string_char_count] = 0;
		free(string);

		char *str = wstr_to_astr(string_chars);
		assert(str != NULL);
		free(string_chars);

		printf("String #%zu (language 0x%04x): \"%s\"\n",
		    idx, (int) lang, str);
		free(str);
	}

	rc = EOK;

leave:
	/* Ignoring errors here. */
	usb_endpoint_pipe_end_session(&ctrl_pipe);

	return rc;
}

/** @}
 */
