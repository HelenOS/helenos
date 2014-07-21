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
#include <ddf/driver.h>
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <usb/dev/dp.h>
#include <usb/ddfiface.h>
#include "usbmid.h"

/** Operations of the device itself. */
static ddf_dev_ops_t mid_device_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface_hub_impl
};

/** Tell whether given interface is already in the list.
 *
 * @param list List of usbmid_interface_t members to be searched.
 * @param interface_no Interface number caller is looking for.
 * @return Interface @p interface_no is already present in the list.
 */
static bool interface_in_list(const list_t *list, int interface_no)
{
	list_foreach(*list, link, usbmid_interface_t, iface) {
		if (iface->interface_no == interface_no) {
			return true;
		}
	}

	return false;
}

/** Create list of interfaces from configuration descriptor.
 *
 * @param config_descriptor Configuration descriptor.
 * @param config_descriptor_size Size of configuration descriptor in bytes.
 * @param list List where to add the interfaces.
 * @return EOK on success, ENOMEM if out of memory.
 */
static int create_interfaces(usb_mid_t *mid, const uint8_t *config_descriptor,
    size_t config_descriptor_size)
{
	const usb_dp_parser_data_t data = {
		.data = config_descriptor,
		.size = config_descriptor_size,
		.arg = NULL
	};

	static const usb_dp_parser_t parser = {
		.nesting = usb_dp_standard_descriptor_nesting
	};

	const uint8_t *interface_ptr =
	    usb_dp_get_nested_descriptor(&parser, &data, config_descriptor);

	/* Walk all descriptors nested in the current configuration decriptor;
	 * i.e. all interface descriptors. */
	for (;interface_ptr != NULL;
	    interface_ptr = usb_dp_get_sibling_descriptor(
	        &parser, &data, config_descriptor, interface_ptr))
	{
		/* The second byte is DESCTYPE byte in all desriptors. */
		if (interface_ptr[1] != USB_DESCTYPE_INTERFACE)
			continue;

		const usb_standard_interface_descriptor_t *interface
		    = (usb_standard_interface_descriptor_t *) interface_ptr;

		/* Skip alternate interfaces. */
		if (interface_in_list(&mid->interface_list,
		    interface->interface_number)) {
			/* TODO: add the alternatives and create match ids
			 * for them. */
			continue;
		}

		/* Create the function */
		ddf_fun_t *fun = ddf_fun_create(mid->dev, fun_inner, NULL);
		if (fun == NULL)
			goto error;

		usbmid_interface_t *iface = ddf_fun_data_alloc(fun,
		    sizeof(usbmid_interface_t));
		if (iface == NULL)
			goto error;

		link_initialize(&iface->link);
		iface->fun = fun;
		iface->interface_no = interface->interface_number;
		iface->interface = interface;

		list_append(&iface->link, &mid->interface_list);
	}

	return EOK;
error:
	while (!list_empty(&mid->interface_list)) {
		link_t *link = list_first(&mid->interface_list);
		usbmid_interface_t *iface = list_get_instance(link,
		    usbmid_interface_t, link);

		ddf_fun_destroy(iface->fun);
	}

	return ENOMEM;
}

/** Explore MID device.
 *
 * We expect that @p dev is initialized and session on control pipe is
 * started.
 *
 * @param dev Device to be explored.
 * @return Whether to accept this device from devman.
 */
bool usbmid_explore_device(usb_device_t *dev)
{
	int rc;

	unsigned dev_class = dev->descriptors.device.device_class;
	if (dev_class != USB_CLASS_USE_INTERFACE) {
		usb_log_warning(
		    "Device class: %u (%s), but expected class %u.\n",
		    dev_class, usb_str_class(dev_class),
		    USB_CLASS_USE_INTERFACE);
		usb_log_error("Not multi interface device, refusing.\n");
		return false;
	}

	/* Shortcuts to save on typing ;-). */
	const void *config_descriptor_raw = dev->descriptors.configuration;
	size_t config_descriptor_size = dev->descriptors.configuration_size;
	const usb_standard_configuration_descriptor_t *config_descriptor =
	    config_descriptor_raw;

	/* Select the first configuration */
	rc = usb_request_set_configuration(&dev->ctrl_pipe,
	    config_descriptor->configuration_number);
	if (rc != EOK) {
		usb_log_error("Failed to set device configuration: %s.\n",
		    str_error(rc));
		return false;
	}

	/* Create driver soft-state. */
	usb_mid_t *usb_mid = usb_device_data_alloc(dev, sizeof(usb_mid_t));
	if (!usb_mid) {
		usb_log_error("Failed to create USB MID structure.\n");
		return false;
	}

	usb_mid->dev = dev->ddf_dev;

	/* Create control function. */
	usb_mid->ctl_fun = ddf_fun_create(dev->ddf_dev, fun_exposed, "ctl");
	if (usb_mid->ctl_fun == NULL) {
		usb_log_error("Failed to create control function.\n");
		return false;
	}
	ddf_fun_set_ops(usb_mid->ctl_fun, &mid_device_ops);

	/* Bind control function. */
	rc = ddf_fun_bind(usb_mid->ctl_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind control function: %s.\n",
		    str_error(rc));
		ddf_fun_destroy(usb_mid->ctl_fun);
		return false;
	}

	/* Create interface children. */
	list_initialize(&usb_mid->interface_list);
	create_interfaces(usb_mid, config_descriptor_raw, config_descriptor_size);

	/* Start child function for every interface. */
	list_foreach(usb_mid->interface_list, link, usbmid_interface_t, iface) {
		usb_log_info("Creating child for interface %d (%s).\n",
		    iface->interface_no,
		    usb_str_class(iface->interface->interface_class));

		rc = usbmid_spawn_interface_child(dev, iface,
		    &dev->descriptors.device, iface->interface);
		if (rc != EOK) {
			usb_log_error("Failed to create interface child: %s.\n",
			    str_error(rc));
		}
	}

	return true;
}

/**
 * @}
 */
