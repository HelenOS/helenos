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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief
 */

#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usbvirt/device.h>
#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ddf/driver.h>

#include "virthub.h"
#include "hub.h"


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

/** Hub descriptor. */
hub_descriptor_t hub_descriptor = {
	.length = sizeof(hub_descriptor_t),
	.type = USB_DESCTYPE_HUB,
	.port_count = HUB_PORT_COUNT,
	.characteristics = HUB_CHAR_NO_POWER_SWITCH_FLAG | HUB_CHAR_NO_OC_FLAG,
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

/** Initializes virtual hub device.
 *
 * @param dev Virtual USB device backend.
 * @return Error code.
 */
errno_t virthub_init(usbvirt_device_t *dev, const char* name)
{
	if (dev == NULL) {
		return EBADMEM;
	}
	dev->ops = &hub_ops;
	dev->descriptors = &descriptors;
	dev->address = 0;

	char *n = str_dup(name);
	if (!n)
		return ENOMEM;

	hub_t *hub = malloc(sizeof(hub_t));
	if (hub == NULL) {
		free(n);
		return ENOMEM;
	}

	dev->name = n;
	hub_init(hub);
	dev->device_data = hub;

	return EOK;
}

/** Connect a device to a virtual hub.
 *
 * @param dev Virtual device representing the hub.
 * @param conn Device to be connected.
 * @return Port device was connected to.
 */
int virthub_connect_device(usbvirt_device_t *dev, vhc_virtdev_t *conn)
{
	assert(dev != NULL);
	assert(conn != NULL);

	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);
	size_t port = hub_connect_device(hub, conn);
	hub_release(hub);

	return port;
}

/** Disconnect a device from a virtual hub.
 *
 * @param dev Virtual device representing the hub.
 * @param conn Device to be disconnected.
 * @return Error code.
 */
errno_t virthub_disconnect_device(usbvirt_device_t *dev, vhc_virtdev_t *conn)
{
	assert(dev != NULL);
	assert(conn != NULL);

	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);
	hub_disconnect_device(hub, conn);
	hub_release(hub);

	return EOK;
}

/** Whether trafic is propagated to given device.
 *
 * @param dev Virtual device representing the hub.
 * @param conn Connected device.
 * @return Whether port is signalling to the device.
 */
bool virthub_is_device_enabled(usbvirt_device_t *dev, vhc_virtdev_t *conn)
{
	assert(dev != NULL);
	assert(conn != NULL);

	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	hub_port_state_t state = HUB_PORT_STATE_UNKNOWN;
	size_t port = hub_find_device(hub, conn);
	if (port != (size_t) -1) {
		state = hub_get_port_state(hub, port);
	}
	hub_release(hub);

	return state == HUB_PORT_STATE_ENABLED;
}

/** Format status of a virtual hub.
 *
 * @param dev Virtual device representing the hub.
 * @param[out] status Hub status information.
 * @param[in] len Size of the @p status buffer.
 */
void virthub_get_status(usbvirt_device_t *dev, char *status, size_t len)
{
	assert(dev != NULL);
	if (len == 0) {
		return;
	}

	hub_t *hub = (hub_t *) dev->device_data;

	char port_status[HUB_PORT_COUNT + 1];

	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		port_status[i] = hub_port_state_to_char(
		    hub_get_port_state(hub, i));
	}
	port_status[HUB_PORT_COUNT] = 0;

	snprintf(status, len, "vhub:%s", port_status);
}

/**
 * @}
 */
