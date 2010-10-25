/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Virtual USB hub.
 */
#include <usb/classes.h>
#include <usbvirt/hub.h>
#include <usbvirt/device.h>
#include <errno.h>
#include <stdlib.h>

#include "vhcd.h"
#include "hub.h"
#include "hubintern.h"


/** Standard device descriptor. */
usb_standard_device_descriptor_t std_device_descriptor = {
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
usb_standard_interface_descriptor_t std_interface_descriptor = {
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

hub_descriptor_t hub_descriptor = {
	.length = sizeof(hub_descriptor_t),
	.type = USB_DESCTYPE_HUB,
	.port_count = HUB_PORT_COUNT,
	.characteristics = 0, 
	.power_on_warm_up = 50, /* Huh? */
	.max_current = 100, /* Huh again. */
	.removable_device = { 0 },
	.port_power = { 0xFF }
};

/** Endpoint descriptor. */
usb_standard_endpoint_descriptor_t endpoint_descriptor = {
	.length = sizeof(usb_standard_endpoint_descriptor_t),
	.descriptor_type = USB_DESCTYPE_ENDPOINT,
	.endpoint_address = HUB_STATUS_CHANGE_PIPE | 128,
	.attributes = USB_TRANSFER_INTERRUPT,
	.max_packet_size = 8,
	.poll_interval = 0xFF
};

/** Standard configuration descriptor. */
usb_standard_configuration_descriptor_t std_configuration_descriptor = {
	.length = sizeof(usb_standard_configuration_descriptor_t),
	.descriptor_type = USB_DESCTYPE_CONFIGURATION,
	.total_length = 
		sizeof(usb_standard_configuration_descriptor_t)
		+ sizeof(std_interface_descriptor)
		+ sizeof(hub_descriptor)
		+ sizeof(endpoint_descriptor)
		,
	.interface_count = 1,
	.configuration_number = HUB_CONFIGURATION_ID,
	.str_configuration = 0,
	.attributes = 128, /* denotes bus-powered device */
	.max_power = 50
};

/** All hub configuration descriptors. */
static usbvirt_device_configuration_extras_t extra_descriptors[] = {
	{
		.data = (uint8_t *) &std_interface_descriptor,
		.length = sizeof(std_interface_descriptor)
	},
	{
		.data = (uint8_t *) &hub_descriptor,
		.length = sizeof(hub_descriptor)
	},
	{
		.data = (uint8_t *) &endpoint_descriptor,
		.length = sizeof(endpoint_descriptor)
	}
};

/** Hub configuration. */
usbvirt_device_configuration_t configuration = {
	.descriptor = &std_configuration_descriptor,
	.extra = extra_descriptors,
	.extra_count = sizeof(extra_descriptors)/sizeof(extra_descriptors[0])
};

/** Hub standard descriptors. */
usbvirt_descriptors_t descriptors = {
	.device = &std_device_descriptor,
	.configuration = &configuration,
	.configuration_count = 1,
};

/** Hub as a virtual device. */
usbvirt_device_t virthub_dev = {
	.ops = &hub_ops,
	.descriptors = &descriptors,
};

/** Hub device. */
hub_device_t hub_dev;

/** Initialize virtual hub. */
void hub_init(void)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub_dev.ports[i];
		
		port->device = NULL;
		port->state = HUB_PORT_STATE_NOT_CONFIGURED;
		port->status_change = 0;
	}
	
	usbvirt_connect_local(&virthub_dev);
	
	dprintf(1, "virtual hub (%d ports) created", HUB_PORT_COUNT);
}

/** Connect device to the hub.
 *
 * @param device Device to be connected.
 * @return Port where the device was connected to.
 */
size_t hub_add_device(virtdev_connection_t *device)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub_dev.ports[i];
		
		if (port->device != NULL) {
			continue;
		}
		
		port->device = device;
		
		/*
		 * TODO:
		 * If the hub was configured, we can normally
		 * announce the plug-in.
		 * Otherwise, we will wait until hub is configured
		 * and announce changes in single burst.
		 */
		//if (port->state == HUB_PORT_STATE_DISCONNECTED) {
			port->state = HUB_PORT_STATE_DISABLED;
			set_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
		//}
		
		return i;
	}
	
	return (size_t)-1;
}

/** Disconnect device from the hub. */
void hub_remove_device(virtdev_connection_t *device)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub_dev.ports[i];
		
		if (port->device != device) {
			continue;
		}
		
		port->device = NULL;
		port->state = HUB_PORT_STATE_DISCONNECTED;
		
		set_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
	}
}

/** Tell whether device port is open.
 *
 * @return Whether communication to and from the device can go through the hub.
 */
bool hub_can_device_signal(virtdev_connection_t * device)
{
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		if (hub_dev.ports[i].device == device) {
			return hub_dev.ports[i].state == HUB_PORT_STATE_ENABLED;
		}
	}
	
	return false;
}

/** Format hub port status.
 *
 * @param result Buffer where to store status string.
 * @param len Number of characters that is possible to store in @p result
 * 	(excluding trailing zero).
 */
void hub_get_port_statuses(char *result, size_t len)
{
	if (len > HUB_PORT_COUNT) {
		len = HUB_PORT_COUNT;
	}
	size_t i;
	for (i = 0; i < len; i++) {
		result[i] = hub_port_state_as_char(hub_dev.ports[i].state);
	}
	result[len] = 0;
}

/**
 * @}
 */
