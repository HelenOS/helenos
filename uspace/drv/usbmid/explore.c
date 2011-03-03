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
 * Exploration of available interfaces in the USB device.
 */
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <usb/classes/classes.h>
#include <usb/request.h>
#include <usb/dp.h>
#include "usbmid.h"

/** Find starting indexes of all interface descriptors in a configuration.
 *
 * @param config_descriptor Full configuration descriptor.
 * @param config_descriptor_size Size of @p config_descriptor in bytes.
 * @param interface_positions Array where to store indexes of interfaces.
 * @param interface_count Size of @p interface_positions array.
 * @return Number of found interfaces.
 * @retval (size_t)-1 Error occured.
 */
static size_t find_interface_descriptors(uint8_t *config_descriptor,
    size_t config_descriptor_size,
    size_t *interface_positions, size_t interface_count)
{
	if (interface_count == 0) {
		return (size_t) -1;
	}

	usb_dp_parser_data_t data = {
		.data = config_descriptor,
		.size = config_descriptor_size,
		.arg = NULL
	};

	usb_dp_parser_t parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};

	uint8_t *interface = usb_dp_get_nested_descriptor(&parser, &data,
	    data.data);
	if (interface == NULL) {
		return (size_t) -1;
	}
	if (interface[1] != USB_DESCTYPE_INTERFACE) {
		return (size_t) -1;
	}

	size_t found_interfaces = 0;
	interface_positions[found_interfaces] = interface - config_descriptor;
	found_interfaces++;

	while (interface != NULL) {
		interface = usb_dp_get_sibling_descriptor(&parser, &data,
		    data.data, interface);
		if ((interface != NULL)
		    && (found_interfaces < interface_count)
		    && (interface[1] == USB_DESCTYPE_INTERFACE)) {
			interface_positions[found_interfaces]
			    = interface - config_descriptor;
			found_interfaces++;
		}
	}

	return found_interfaces;
}

/** Explore MID device.
 *
 * We expect that @p dev is initialized and session on control pipe is
 * started.
 *
 * @param dev Device to be explored.
 * @return Whether to accept this device from devman.
 */
bool usbmid_explore_device(usbmid_device_t *dev)
{
	usb_standard_device_descriptor_t device_descriptor;
	int rc = usb_request_get_device_descriptor(&dev->ctrl_pipe,
	    &device_descriptor);
	if (rc != EOK) {
		usb_log_error("Getting device descriptor failed: %s.\n",
		    str_error(rc));
		return false;
	}

	if (device_descriptor.device_class != USB_CLASS_USE_INTERFACE) {
		usb_log_warning(
		    "Device class: %d (%s), but expected class 0.\n",
		    device_descriptor.device_class,
		    usb_str_class(device_descriptor.device_class));
		usb_log_error("Not multi interface device, refusing.\n");
		return false;
	}

	size_t config_descriptor_size;
	uint8_t *config_descriptor_raw = NULL;
	rc = usb_request_get_full_configuration_descriptor_alloc(
	    &dev->ctrl_pipe, 0,
	    (void **) &config_descriptor_raw, &config_descriptor_size);
	if (rc != EOK) {
		usb_log_error("Failed getting full config descriptor: %s.\n",
		    str_error(rc));
		return false;
	}

	usb_standard_configuration_descriptor_t *config_descriptor
	    = (usb_standard_configuration_descriptor_t *) config_descriptor_raw;

	size_t *interface_descriptors
	    = malloc(sizeof(size_t) * config_descriptor->interface_count);
	if (interface_descriptors == NULL) {
		usb_log_error("Out of memory (wanted %zuB).\n",
		    sizeof(size_t) * config_descriptor->interface_count);
		free(config_descriptor_raw);
		return false;
	}
	size_t interface_descriptors_count
	    = find_interface_descriptors(
	    config_descriptor_raw, config_descriptor_size,
	    interface_descriptors, config_descriptor->interface_count);

	if (interface_descriptors_count == (size_t) -1) {
		usb_log_error("Problem parsing configuration descriptor.\n");
		free(config_descriptor_raw);
		free(interface_descriptors);
		return false;
	}

	ddf_fun_t *ctl_fun = ddf_fun_create(dev->dev, fun_exposed, "ctl");
	if (ctl_fun == NULL) {
		usb_log_error("Failed to create control function.\n");
		free(config_descriptor_raw);
		free(interface_descriptors);
		return false;
	}
	rc = ddf_fun_bind(ctl_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind control function: %s.\n",
		    str_error(rc));
		free(config_descriptor_raw);
		free(interface_descriptors);
		return false;
	}

	size_t i;
	for (i = 0; i < interface_descriptors_count; i++) {
		usb_standard_interface_descriptor_t *interface
		    = (usb_standard_interface_descriptor_t *)
		    (config_descriptor_raw + interface_descriptors[i]);
		usb_log_debug2("Interface descriptor at index %zu (type %d).\n",
		    interface_descriptors[i], (int) interface->descriptor_type);
		usb_log_info("Creating child for interface %d (%s).\n",
		    (int) interface->interface_number,
		    usb_str_class(interface->interface_class));
		rc = usbmid_spawn_interface_child(dev, &device_descriptor,
		    interface);
		if (rc != EOK) {
			usb_log_error("Failed to create interface child: %s.\n",
			    str_error(rc));
		}
	}

	free(config_descriptor_raw);

	return true;
}

/**
 * @}
 */
