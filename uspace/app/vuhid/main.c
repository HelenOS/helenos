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

/** @addtogroup usbvirthid
 * @{
 */
/**
 * @file
 * Virtual USB HID device.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/hid/hid.h>
#include <usbvirt/device.h>

#include "virthid.h"
#include "ifaces.h"
#include "stdreq.h"

static usbvirt_control_request_handler_t endpoint_zero_handlers[] = {
	{
		.req_direction = USB_DIRECTION_IN,
		.req_type = USB_REQUEST_TYPE_STANDARD,
		.req_recipient = USB_REQUEST_RECIPIENT_INTERFACE,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.name = "Get_Descriptor",
		.callback = req_get_descriptor
	},
	{
		.req_direction = USB_DIRECTION_OUT,
		.req_recipient = USB_REQUEST_RECIPIENT_INTERFACE,
		.req_type = USB_REQUEST_TYPE_CLASS,
		.request = USB_HIDREQ_SET_PROTOCOL,
		.name = "Set_Protocol",
		.callback = req_set_protocol
	},
	{
		.req_direction = USB_DIRECTION_OUT,
		.req_recipient = USB_REQUEST_RECIPIENT_INTERFACE,
		.req_type = USB_REQUEST_TYPE_CLASS,
		.request = USB_HIDREQ_SET_REPORT,
		.name = "Set_Report",
		.callback = req_set_report
	},
	{
		.callback = NULL
	}
};

/** Keyboard callbacks.
 * We abuse the fact that static variables are zero-filled.
 */
static usbvirt_device_ops_t hid_ops = {
	.control = endpoint_zero_handlers,
};

/** Standard configuration descriptor. */
static usb_standard_configuration_descriptor_t std_configuration_descriptor = {
	.length = sizeof(usb_standard_configuration_descriptor_t),
	.descriptor_type = USB_DESCTYPE_CONFIGURATION,
	/* Will be patched at runtime. */
	.total_length = sizeof(usb_standard_configuration_descriptor_t),
	.interface_count = 0,
	.configuration_number = 1,
	.str_configuration = 0,
	.attributes = 128, /* denotes bus-powered device */
	.max_power = 50
};

/** Standard device descriptor. */
static usb_standard_device_descriptor_t std_device_descriptor = {
	.length = sizeof(usb_standard_device_descriptor_t),
	.descriptor_type = USB_DESCTYPE_DEVICE,
	.usb_spec_version = 0x110,
	.device_class = USB_CLASS_USE_INTERFACE,
	.device_subclass = 0,
	.device_protocol = 0,
	.max_packet_size = 64,
	.configuration_count = 1
};

/** HID configuration. */
usbvirt_device_configuration_t configuration = {
	.descriptor = &std_configuration_descriptor,
	.extra = NULL,
	.extra_count = 0
};

/** HID standard descriptors. */
usbvirt_descriptors_t descriptors = {
	.device = &std_device_descriptor,
	.configuration = &configuration,
	.configuration_count = 1,
};

static vuhid_data_t vuhid_data = {
	.in_endpoints_mapping = { NULL },
	.in_endpoint_first_free = 1,
	.out_endpoints_mapping = { NULL },
	.out_endpoint_first_free = 1,

	.iface_count = 0,
	.iface_died_count = 0
	// mutex and CV must be initialized elsewhere
};


/** Keyboard device.
 * Rest of the items will be initialized later.
 */
static usbvirt_device_t hid_dev = {
	.ops = &hid_ops,
	.descriptors = &descriptors,
	.name = "HID",
	.device_data = &vuhid_data
};


int main(int argc, char * argv[])
{
	int rc;

	log_init("vuhid");

	fibril_mutex_initialize(&vuhid_data.iface_count_mutex);
	fibril_condvar_initialize(&vuhid_data.iface_count_cv);

	/* Determine which interfaces to initialize. */
	int i;
	for (i = 1; i < argc; i++) {
		rc = add_interface_by_id(available_hid_interfaces, argv[i],
		    &hid_dev);
		if (rc != EOK) {
			fprintf(stderr, "Failed to add device `%s': %s.\n",
			    argv[i], str_error(rc));
		} else {
			printf("Added device `%s'.\n", argv[i]);
		}
	}

	for (i = 0; i < (int) hid_dev.descriptors->configuration->extra_count; i++) {
		usb_log_debug("Found extra descriptor: %s.\n",
		    usb_debug_str_buffer(
		        hid_dev.descriptors->configuration->extra[i].data,
		        hid_dev.descriptors->configuration->extra[i].length,
		        0));
	}

	rc = usbvirt_device_plug(&hid_dev, "/virt/usbhc/hc");
	if (rc != EOK) {
		printf("Unable to start communication with VHCD: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	printf("Connected to VHCD...\n");

	wait_for_interfaces_death(&hid_dev);
	
	printf("Terminating...\n");
	
	usbvirt_device_unplug(&hid_dev);

	return 0;
}


/** @}
 */
