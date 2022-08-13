/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Virtual USB device main routines.
 */

#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usb/descriptor.h>
#include <usb/usb.h>
#include <usbvirt/device.h>

#include "virthub_base.h"

#define HUB_CONFIGURATION_ID   1

/** Standard device descriptor. */
const usb_standard_device_descriptor_t virthub_device_descriptor = {
	.length = sizeof(usb_standard_device_descriptor_t),
	.descriptor_type = USB_DESCTYPE_DEVICE,
	.usb_spec_version = 0x110,
	.device_class = USB_CLASS_HUB,
	.device_subclass = 0,
	.device_protocol = 0,
	.max_packet_size = 64,
	.configuration_count = 1
};

/** Standard interface descriptor. */
const usb_standard_interface_descriptor_t virthub_interface_descriptor = {
	.length = sizeof(usb_standard_interface_descriptor_t),
	.descriptor_type = USB_DESCTYPE_INTERFACE,
	.interface_number = 0,
	.alternate_setting = 0,
	.endpoint_count = 1,
	.interface_class = USB_CLASS_HUB,
	.interface_subclass = 0,
	.interface_protocol = 0,
	.str_interface = 0
};

/** Endpoint descriptor. */
const usb_standard_endpoint_descriptor_t virthub_endpoint_descriptor = {
	.length = sizeof(usb_standard_endpoint_descriptor_t),
	.descriptor_type = USB_DESCTYPE_ENDPOINT,
	.endpoint_address = 1 | 128,
	.attributes = USB_TRANSFER_INTERRUPT,
	.max_packet_size = 8,
	.poll_interval = 0xFF,
};

/** Standard configuration descriptor. */
const usb_standard_configuration_descriptor_t virthub_configuration_descriptor_without_hub_size = {
	.length = sizeof(virthub_configuration_descriptor_without_hub_size),
	.descriptor_type = USB_DESCTYPE_CONFIGURATION,
	.total_length =
	    sizeof(virthub_configuration_descriptor_without_hub_size) +
	    sizeof(virthub_interface_descriptor) +
	    sizeof(virthub_endpoint_descriptor),
	.interface_count = 1,
	.configuration_number = HUB_CONFIGURATION_ID,
	.str_configuration = 0,
	.attributes = 0, /* We are self-powered device */
	.max_power = 0,
};

const usbvirt_device_configuration_extras_t virthub_interface_descriptor_ex = {
	.data = (uint8_t *) &virthub_interface_descriptor,
	.length = sizeof(virthub_interface_descriptor),
};

/**
 * @}
 */
