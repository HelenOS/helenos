/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2018 Ondrej Hlavaty, Michal Staruch
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
 * Non trivial initialization of endpoint pipes.
 *
 */
#include <usb/dev/pipes.h>
#include <usb/dev/dp.h>
#include <usb/dev/request.h>
#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/descriptor.h>

#include <assert.h>
#include <errno.h>

#define DEV_DESCR_MAX_PACKET_SIZE_OFFSET 7

#define NESTING(parentname, childname) \
	{ \
		.child = USB_DESCTYPE_##childname, \
		.parent = USB_DESCTYPE_##parentname, \
	}
#define LAST_NESTING { -1, -1 }

/** Nesting pairs of standard descriptors. */
static const usb_dp_descriptor_nesting_t descriptor_nesting[] = {
	NESTING(CONFIGURATION, INTERFACE),
	NESTING(INTERFACE, ENDPOINT),
	NESTING(INTERFACE, HUB),
	NESTING(INTERFACE, HID),
	NESTING(HID, HID_REPORT),
	NESTING(ENDPOINT, SSPEED_EP_COMPANION),
	LAST_NESTING
};

/** Tells whether given descriptor is of endpoint type.
 *
 * @param descriptor Descriptor in question.
 * @return Whether the given descriptor is endpoint descriptor.
 */
static inline bool is_endpoint_descriptor(const uint8_t *descriptor)
{
	return descriptor[1] == USB_DESCTYPE_ENDPOINT;
}

/** Tells whether given descriptor is of superspeed companion type.
 *
 * @param descriptor Descriptor in question.
 * @return Whether the given descriptor is superspeed companion descriptor.
 */
static inline bool is_superspeed_companion_descriptor(const uint8_t *descriptor)
{
	return descriptor[1] == USB_DESCTYPE_SSPEED_EP_COMPANION;
}

/** Tells whether found endpoint corresponds to endpoint described by user.
 *
 * @param wanted Endpoint description as entered by driver author.
 * @param found Endpoint description obtained from endpoint descriptor.
 * @return Whether the @p found descriptor fits the @p wanted descriptor.
 */
static bool endpoint_fits_description(const usb_endpoint_description_t *wanted,
    const usb_endpoint_description_t *found)
{
#define _SAME(fieldname) ((wanted->fieldname) == (found->fieldname))

	if (!_SAME(direction)) {
		return false;
	}

	if (!_SAME(transfer_type)) {
		return false;
	}

	if ((wanted->interface_class >= 0) && !_SAME(interface_class)) {
		return false;
	}

	if ((wanted->interface_subclass >= 0) && !_SAME(interface_subclass)) {
		return false;
	}

	if ((wanted->interface_protocol >= 0) && !_SAME(interface_protocol)) {
		return false;
	}

#undef _SAME

	return true;
}

/** Find endpoint mapping for a found endpoint.
 *
 * @param mapping Endpoint mapping list.
 * @param mapping_count Number of endpoint mappings in @p mapping.
 * @param found_endpoint Description of found endpoint.
 * @param interface_number Number of currently processed interface.
 * @return Endpoint mapping corresponding to @p found_endpoint.
 * @retval NULL No corresponding endpoint found.
 */
static usb_endpoint_mapping_t *find_endpoint_mapping(
    usb_endpoint_mapping_t *mapping, size_t mapping_count,
    const usb_endpoint_description_t *found_endpoint,
    int interface_number, int interface_setting)
{
	while (mapping_count > 0) {
		bool interface_number_fits = (mapping->interface_no < 0) ||
		    (mapping->interface_no == interface_number);

		bool interface_setting_fits = (mapping->interface_setting < 0) ||
		    (mapping->interface_setting == interface_setting);

		bool endpoint_descriptions_fits = endpoint_fits_description(
		    mapping->description, found_endpoint);

		if (interface_number_fits &&
		    interface_setting_fits &&
		    endpoint_descriptions_fits &&
		    !mapping->present) {
			return mapping;
		}

		mapping++;
		mapping_count--;
	}

	return NULL;
}

/** Process endpoint descriptor.
 *
 * @param mapping Endpoint mapping list.
 * @param mapping_count Number of endpoint mappings in @p mapping.
 * @param interface Interface descriptor under which belongs the @p endpoint.
 * @param endpoint Endpoint descriptor.
 * @param companion Superspeed companion descriptor.
 * @return Error code.
 */
static errno_t process_endpoint(
    usb_endpoint_mapping_t *mapping, size_t mapping_count,
    usb_standard_interface_descriptor_t *interface,
    usb_standard_endpoint_descriptor_t *endpoint_desc,
    usb_superspeed_endpoint_companion_descriptor_t *companion_desc,
    usb_dev_session_t *bus_session)
{

	/*
	 * Get endpoint characteristics.
	 */
	const usb_endpoint_description_t description = {
		.transfer_type = USB_ED_GET_TRANSFER_TYPE(*endpoint_desc),
		.direction = USB_ED_GET_DIR(*endpoint_desc),

		/* Get interface characteristics. */
		.interface_class = interface->interface_class,
		.interface_subclass = interface->interface_subclass,
		.interface_protocol = interface->interface_protocol,
	};

	/*
	 * Find the most fitting mapping and initialize the pipe.
	 */
	usb_endpoint_mapping_t *ep_mapping = find_endpoint_mapping(mapping,
	    mapping_count, &description,
	    interface->interface_number, interface->alternate_setting);
	if (ep_mapping == NULL) {
		return ENOENT;
	}

	if (ep_mapping->present) {
		return EEXIST;
	}

	errno_t err = usb_pipe_initialize(&ep_mapping->pipe, bus_session);
	if (err)
		return err;

	ep_mapping->present = true;
	ep_mapping->descriptor = endpoint_desc;
	ep_mapping->companion_descriptor = companion_desc;
	ep_mapping->interface = interface;

	return EOK;
}

/** Process whole USB interface.
 *
 * @param mapping Endpoint mapping list.
 * @param mapping_count Number of endpoint mappings in @p mapping.
 * @param parser Descriptor parser.
 * @param parser_data Descriptor parser data.
 * @param interface_descriptor Interface descriptor.
 * @return Error code.
 */
static errno_t process_interface(
    usb_endpoint_mapping_t *mapping, size_t mapping_count,
    const usb_dp_parser_t *parser, const usb_dp_parser_data_t *parser_data,
    const uint8_t *interface_descriptor, usb_dev_session_t *bus_session)
{
	const uint8_t *descriptor = usb_dp_get_nested_descriptor(parser,
	    parser_data, interface_descriptor);

	if (descriptor == NULL) {
		return ENOENT;
	}

	do {
		if (is_endpoint_descriptor(descriptor)) {
			/* Check if companion descriptor is present too, it should immediatelly follow. */
			const uint8_t *companion_desc = usb_dp_get_nested_descriptor(parser,
			    parser_data, descriptor);
			if (companion_desc && !is_superspeed_companion_descriptor(companion_desc)) {
				/* Not what we wanted, don't pass it further. */
				companion_desc = NULL;
			}

			(void) process_endpoint(mapping, mapping_count,
			    (usb_standard_interface_descriptor_t *)
			    interface_descriptor,
			    (usb_standard_endpoint_descriptor_t *)
			    descriptor,
			    (usb_superspeed_endpoint_companion_descriptor_t *)
			    companion_desc,
			    bus_session);
		}

		descriptor = usb_dp_get_sibling_descriptor(parser, parser_data,
		    interface_descriptor, descriptor);
	} while (descriptor != NULL);

	return EOK;
}

/** Initialize endpoint pipes from configuration descriptor.
 *
 * The mapping array is expected to conform to following rules:
 * - @c pipe must be uninitialized pipe
 * - @c description must point to prepared endpoint description
 * - @c descriptor does not need to be initialized (will be overwritten)
 * - @c interface does not need to be initialized (will be overwritten)
 * - @c present does not need to be initialized (will be overwritten)
 *
 * After processing the configuration descriptor, the mapping is updated
 * in the following fashion:
 * - @c present will be set to @c true when the endpoint was found in the
 *   configuration
 * - @c descriptor will point inside the configuration descriptor to endpoint
 *   corresponding to given description (or NULL for not found descriptor)
 * - @c interface will point inside the configuration descriptor to interface
 *   descriptor the endpoint @c descriptor belongs to (or NULL for not found
 *   descriptor)
 * - @c pipe will be initialized when found, otherwise left untouched
 * - @c description will be untouched under all circumstances
 *
 * @param mapping Endpoint mapping list.
 * @param mapping_count Number of endpoint mappings in @p mapping.
 * @param configuration_descriptor Full configuration descriptor (is expected
 *	to be in USB endianness: i.e. as-is after being retrieved from
 *	the device).
 * @param configuration_descriptor_size Size of @p configuration_descriptor
 *	in bytes.
 * @param connection Connection backing the endpoint pipes.
 * @return Error code.
 */
errno_t usb_pipe_initialize_from_configuration(
    usb_endpoint_mapping_t *mapping, size_t mapping_count,
    const uint8_t *config_descriptor, size_t config_descriptor_size,
    usb_dev_session_t *bus_session)
{
	if (config_descriptor == NULL)
		return EBADMEM;

	if (config_descriptor_size <
	    sizeof(usb_standard_configuration_descriptor_t)) {
		return ERANGE;
	}

	/* Go through the mapping and set all endpoints to not present. */
	for (size_t i = 0; i < mapping_count; i++) {
		mapping[i].present = false;
		mapping[i].descriptor = NULL;
		mapping[i].interface = NULL;
	}

	/* Prepare the descriptor parser. */
	const usb_dp_parser_t dp_parser = {
		.nesting = descriptor_nesting
	};
	const usb_dp_parser_data_t dp_data = {
		.data = config_descriptor,
		.size = config_descriptor_size,
	};

	/*
	 * Iterate through all interfaces.
	 */
	const uint8_t *interface = usb_dp_get_nested_descriptor(&dp_parser,
	    &dp_data, config_descriptor);
	if (interface == NULL) {
		return ENOENT;
	}
	do {
		(void) process_interface(mapping, mapping_count,
		    &dp_parser, &dp_data, interface, bus_session);
		interface = usb_dp_get_sibling_descriptor(&dp_parser, &dp_data,
		    config_descriptor, interface);
	} while (interface != NULL);

	return EOK;
}

/**
 * @}
 */
