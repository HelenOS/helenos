/*
 * Copyright (c) 2013 Jan Vesely
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
