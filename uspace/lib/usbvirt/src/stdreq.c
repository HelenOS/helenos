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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Standard control request handlers.
 */
#include "private.h"
#include <usb/dev/request.h>
#include <assert.h>
#include <errno.h>

/** Helper for replying to control read transfer from virtual USB device.
 *
 * This function takes care of copying data to answer buffer taking care
 * of buffer sizes properly.
 *
 * @param setup_packet The setup packet.
 * @param data Data buffer to write to.
 * @param act_size Where to write actual size of returned data.
 * @param actual_data Data to be returned.
 * @param actual_data_size Size of answer data (@p actual_data) in bytes.
 */
void usbvirt_control_reply_helper(const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size,
    const void *actual_data, size_t actual_data_size)
{
	size_t expected_size = setup_packet->length;
	if (expected_size < actual_data_size) {
		actual_data_size = expected_size;
	}

	memcpy(data, actual_data, actual_data_size);

	if (act_size != NULL) {
		*act_size = actual_data_size;
	}
}

/** NOP handler */
errno_t req_nop(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	return EOK;
}

/** GET_DESCRIPTOR handler. */
static errno_t req_get_descriptor(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint8_t type = setup_packet->value_high;
	uint8_t index = setup_packet->value_low;

	/*
	 * Standard device descriptor.
	 */
	if ((type == USB_DESCTYPE_DEVICE) && (index == 0)) {
		if (device->descriptors && device->descriptors->device) {
			usbvirt_control_reply_helper(setup_packet, data, act_size,
			    device->descriptors->device,
			    device->descriptors->device->length);
			return EOK;
		} else {
			return EFORWARD;
		}
	}

	/*
	 * Configuration descriptor together with interface, endpoint and
	 * class-specific descriptors.
	 */
	if (type == USB_DESCTYPE_CONFIGURATION) {
		if (!device->descriptors) {
			return EFORWARD;
		}
		if (index >= device->descriptors->configuration_count) {
			return EFORWARD;
		}
		/* Copy the data. */
		const usbvirt_device_configuration_t *config =
		    &device->descriptors->configuration[index];
		uint8_t *all_data = malloc(config->descriptor->total_length);
		if (all_data == NULL) {
			return ENOMEM;
		}

		uint8_t *ptr = all_data;
		memcpy(ptr, config->descriptor, config->descriptor->length);
		ptr += config->descriptor->length;
		size_t i;
		for (i = 0; i < config->extra_count; i++) {
			const usbvirt_device_configuration_extras_t *extra =
			    &config->extra[i];
			memcpy(ptr, extra->data, extra->length);
			ptr += extra->length;
		}

		usbvirt_control_reply_helper(setup_packet, data, act_size,
		    all_data, config->descriptor->total_length);

		free(all_data);

		return EOK;
	}

	return EFORWARD;
}

static errno_t req_set_address(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint16_t new_address = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}

	if (new_address > 127) {
		return EINVAL;
	}

	device->address = new_address;

	return EOK;
}

static errno_t req_set_configuration(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint16_t configuration_value = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}

	/*
	 * Configuration value is 1 byte information.
	 */
	if (configuration_value > 255) {
		return EINVAL;
	}

	/*
	 * Do nothing when in default state. According to specification,
	 * this is not specified.
	 */
	if (device->state == USBVIRT_STATE_DEFAULT) {
		return EOK;
	}

	usbvirt_device_state_t new_state;
	if (configuration_value == 0) {
		new_state = USBVIRT_STATE_ADDRESS;
	} else {
		// FIXME: check that this configuration exists
		new_state = USBVIRT_STATE_CONFIGURED;
	}

	if (device->ops && device->ops->state_changed) {
		device->ops->state_changed(device, device->state, new_state);
	}
	device->state = new_state;

	return EOK;
}

static errno_t req_get_dev_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	if (setup_packet->length != 2)
		return ESTALL;
	data[0] = (device->self_powered ? 1 : 0) | (device->remote_wakeup ? 2 : 0);
	data[1] = 0;
	*act_size = 2;
	return EOK;
}
static errno_t req_get_iface_ep_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	if (setup_packet->length != 2)
		return ESTALL;
	data[0] = 0;
	data[1] = 0;
	*act_size = 2;
	return EOK;
}

/** Standard request handlers. */
usbvirt_control_request_handler_t library_handlers[] = {
	{
		STD_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_SET_ADDRESS),
		.name = "SetAddress",
		.callback = req_set_address
	},
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetStdDescriptor",
		.callback = req_get_descriptor
	},
	{
		STD_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_SET_CONFIGURATION),
		.name = "SetConfiguration",
		.callback = req_set_configuration
	},
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_STATUS),
		.name = "GetDeviceStatus",
		.callback = req_get_dev_status,
	},
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_INTERFACE, USB_DEVREQ_GET_STATUS),
		.name = "GetInterfaceStatus",
		.callback = req_get_iface_ep_status,
	},
	{
		/* virtual EPs by default cannot be stalled */
		STD_REQ_IN(USB_REQUEST_RECIPIENT_ENDPOINT, USB_DEVREQ_GET_STATUS),
		.name = "GetEndpointStatus",
		.callback = req_get_iface_ep_status,
	},
	{ .callback = NULL }
};

/**
 * @}
 */
