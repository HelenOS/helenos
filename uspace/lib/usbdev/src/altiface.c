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
/** @file
 * Handling alternate interface settings.
 */
#include <usb/dev/driver.h>
#include <usb/dev/request.h>
#include <usb/debug.h>
#include <usb/dev/dp.h>
#include <errno.h>
#include <str_error.h>
#include <assert.h>

/** Count number of alternate settings of a interface.
 *
 * @param config_descr Full configuration descriptor.
 * @param config_descr_size Size of @p config_descr in bytes.
 * @param interface_no Interface number.
 * @return Number of alternate interfaces for @p interface_no interface.
 */
size_t usb_interface_count_alternates(uint8_t *config_descr,
    size_t config_descr_size, uint8_t interface_no)
{
	assert(config_descr != NULL);
	assert(config_descr_size > 0);

	const usb_dp_parser_t dp_parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	const usb_dp_parser_data_t dp_data = {
		.data = config_descr,
		.size = config_descr_size,
		.arg = NULL
	};

	size_t alternate_count = 0;

	const uint8_t *iface_ptr = usb_dp_get_nested_descriptor(&dp_parser,
	    &dp_data, config_descr);
	while (iface_ptr != NULL) {
		usb_standard_interface_descriptor_t *iface
		    = (usb_standard_interface_descriptor_t *) iface_ptr;
		if (iface->descriptor_type == USB_DESCTYPE_INTERFACE) {
			if (iface->interface_number == interface_no) {
				alternate_count++;
			}
		}
		iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser, &dp_data,
		    config_descr, iface_ptr);
	}

	return alternate_count;
}

/** Create alternate interface representation structure.
 *
 * @param[in] config_descr Configuration descriptor.
 * @param[in] config_descr_size Size of configuration descriptor.
 * @param[in] interface_number Interface number.
 * @param[out] alternates_ptr Where to store pointer to allocated structure.
 * @return Error code.
 */
int usb_alternate_interfaces_create(uint8_t *config_descr,
    size_t config_descr_size, int interface_number,
    usb_alternate_interfaces_t **alternates_ptr)
{
	assert(alternates_ptr != NULL);
	assert(config_descr != NULL);
	assert(config_descr_size > 0);

	if (interface_number < 0) {
		alternates_ptr = NULL;
		return EOK;
	}

	usb_alternate_interfaces_t *alternates
	    = malloc(sizeof(usb_alternate_interfaces_t));

	if (alternates == NULL) {
		return ENOMEM;
	}

	alternates->alternative_count
	    = usb_interface_count_alternates(config_descr, config_descr_size,
	    interface_number);

	if (alternates->alternative_count == 0) {
		free(alternates);
		return ENOENT;
	}

	alternates->alternatives = malloc(alternates->alternative_count
	    * sizeof(usb_alternate_interface_descriptors_t));
	if (alternates->alternatives == NULL) {
		free(alternates);
		return ENOMEM;
	}

	alternates->current = 0;

	usb_dp_parser_t dp_parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	usb_dp_parser_data_t dp_data = {
		.data = config_descr,
		.size = config_descr_size,
		.arg = NULL
	};

	usb_alternate_interface_descriptors_t *cur_alt_iface
	    = &alternates->alternatives[0];

	const uint8_t *iface_ptr = usb_dp_get_nested_descriptor(&dp_parser,
	    &dp_data, dp_data.data);
	while (iface_ptr != NULL) {
		usb_standard_interface_descriptor_t *iface
		    = (usb_standard_interface_descriptor_t *) iface_ptr;
		if ((iface->descriptor_type != USB_DESCTYPE_INTERFACE)
		    || (iface->interface_number != interface_number)) {
			iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser,
			    &dp_data,
			    dp_data.data, iface_ptr);
			continue;
		}

		cur_alt_iface->interface = iface;
		cur_alt_iface->nested_descriptors = iface_ptr + sizeof(*iface);

		/* Find next interface to count size of nested descriptors. */
		iface_ptr = usb_dp_get_sibling_descriptor(&dp_parser, &dp_data,
		    dp_data.data, iface_ptr);
		if (iface_ptr == NULL) {
			uint8_t *next = dp_data.data + dp_data.size;
			cur_alt_iface->nested_descriptors_size
			    = next - cur_alt_iface->nested_descriptors;
		} else {
			cur_alt_iface->nested_descriptors_size
			    = iface_ptr - cur_alt_iface->nested_descriptors;
		}

		cur_alt_iface++;
	}

	*alternates_ptr = alternates;

	return EOK;
}


/**
 * @}
 */
