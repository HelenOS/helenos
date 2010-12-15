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
 * @brief Virtual USB hub operations.
 */
#include <errno.h>
#include <usb/classes/hub.h>
#include "virthub.h"
#include "hub.h"

/** Callback when device changes states. */
static void on_state_change(struct usbvirt_device *dev,
    usbvirt_device_state_t old_state, usbvirt_device_state_t new_state)
{
	hub_t *hub = (hub_t *)dev->device_data;

	hub_acquire(hub);

	switch (new_state) {
		case USBVIRT_STATE_CONFIGURED:
			hub_set_port_state_all(hub, HUB_PORT_STATE_POWERED_OFF);
			break;
		case USBVIRT_STATE_ADDRESS:
			hub_set_port_state_all(hub, HUB_PORT_STATE_NOT_CONFIGURED);
			break;
		default:
			break;
	}

	hub_release(hub);
}

/** Callback for data request. */
static int req_on_data(struct usbvirt_device *dev,
    usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size)
{
	if (endpoint != HUB_STATUS_CHANGE_PIPE) {
		return EINVAL;
	}
	
	hub_t *hub = (hub_t *)dev->device_data;

	hub_acquire(hub);

	uint8_t change_map = hub_get_status_change_bitmap(hub);
		
	uint8_t *b = (uint8_t *) buffer;
	if (size > 0) {
		*b = change_map;
		*actual_size = 1;
	}
	
	hub_release(hub);

	return EOK;
}


static int req_clear_hub_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return ENOTSUP;
}

static int req_clear_port_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	int rc;
	size_t port = request->index - 1;
	usb_hub_class_feature_t feature = request->value;
	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	hub_port_state_t port_state = hub_get_port_state(hub, port);

	switch (feature) {
		case USB_HUB_FEATURE_PORT_ENABLE:
			if ((port_state != HUB_PORT_STATE_NOT_CONFIGURED)
			    && (port_state != HUB_PORT_STATE_POWERED_OFF)) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_DISABLED);
			}
			rc = EOK;
			break;

		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port_state != HUB_PORT_STATE_SUSPENDED) {
				rc = EOK;
				break;
			}
			hub_set_port_state(hub, port, HUB_PORT_STATE_RESUMING);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_PORT_POWER:
			if (port_state != HUB_PORT_STATE_NOT_CONFIGURED) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_POWERED_OFF);
			}
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_CONNECTION:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_CONNECTION);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_ENABLE:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_ENABLE);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_SUSPEND:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_SUSPEND);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_OVER_CURRENT);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_RESET:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_RESET);
			rc = EOK;
			break;

		default:
			rc = ENOTSUP;
			break;
	}

	hub_release(hub);

	return rc;
}

static int req_get_bus_state(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return ENOTSUP;
}

static int req_get_descriptor(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	if (request->value_high == USB_DESCTYPE_HUB) {
		int rc = dev->control_transfer_reply(dev, 0,
		    &hub_descriptor, hub_descriptor.length);

		return rc;
	}
	/* Let the framework handle all the rest. */
	return EFORWARD;
}

static int req_get_hub_status(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	uint32_t hub_status = 0;

	return dev->control_transfer_reply(dev, 0,
	    &hub_status, sizeof(hub_status));
}

static int req_get_port_status(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	uint32_t status = hub_get_port_status(hub, request->index - 1);

	hub_release(hub);

	return dev->control_transfer_reply(dev, 0, &status, 4);
}

static int req_set_hub_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return ENOTSUP;
}

static int req_set_port_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	int rc;
	size_t port = request->index - 1;
	usb_hub_class_feature_t feature = request->value;
	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	hub_port_state_t port_state = hub_get_port_state(hub, port);

	switch (feature) {
		case USB_HUB_FEATURE_PORT_RESET:
			if (port_state != HUB_PORT_STATE_POWERED_OFF) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_RESETTING);
			}
			rc = EOK;
			break;

		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port_state == HUB_PORT_STATE_ENABLED) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_SUSPENDED);
			}
			rc = EOK;
			break;

		case USB_HUB_FEATURE_PORT_POWER:
			if (port_state == HUB_PORT_STATE_POWERED_OFF) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_DISCONNECTED);
			}
			rc = EOK;
			break;

		default:
			break;
	}

	hub_release(hub);

	return rc;
}



#define CLASS_REQ_IN(recipient) \
	USBVIRT_MAKE_CONTROL_REQUEST_TYPE(USB_DIRECTION_IN, \
	USBVIRT_REQUEST_TYPE_CLASS, recipient)
#define CLASS_REQ_OUT(recipient) \
	USBVIRT_MAKE_CONTROL_REQUEST_TYPE(USB_DIRECTION_OUT, \
	USBVIRT_REQUEST_TYPE_CLASS, recipient)

#define REC_OTHER USBVIRT_REQUEST_RECIPIENT_OTHER
#define REC_DEVICE USBVIRT_REQUEST_RECIPIENT_DEVICE
#define DIR_IN USB_DIRECTION_IN
#define DIR_OUT USB_DIRECTION_OUT

#define CLASS_REQ(direction, recipient, req) \
	.request_type = USBVIRT_MAKE_CONTROL_REQUEST_TYPE(direction, \
	    USBVIRT_REQUEST_TYPE_CLASS, recipient), \
	.request = req

#define STD_REQ(direction, recipient, req) \
	.request_type = USBVIRT_MAKE_CONTROL_REQUEST_TYPE(direction, \
	    USBVIRT_REQUEST_TYPE_STANDARD, recipient), \
	.request = req

/** Hub operations on control endpoint zero. */
static usbvirt_control_transfer_handler_t endpoint_zero_handlers[] = {
	{
		STD_REQ(DIR_IN, REC_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ(DIR_IN, REC_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ(DIR_IN, REC_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ(DIR_OUT, REC_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		.callback = req_clear_hub_feature
	},
	{
		CLASS_REQ(DIR_OUT, REC_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature
	},
	{
		CLASS_REQ(DIR_IN, REC_OTHER, USB_HUB_REQUEST_GET_STATE),
		.name = "GetBusState",
		.callback = req_get_bus_state
	},
	{
		CLASS_REQ(DIR_IN, REC_DEVICE, USB_HUB_REQUEST_GET_DESCRIPTOR),
		.name = "GetHubDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ(DIR_IN, REC_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		.callback = req_get_hub_status
	},
	{
		CLASS_REQ(DIR_IN, REC_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ(DIR_OUT, REC_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		.callback = req_set_hub_feature
	},
	{
		CLASS_REQ(DIR_OUT, REC_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature
	},
	USBVIRT_CONTROL_TRANSFER_HANDLER_LAST
};


/** Hub operations. */
usbvirt_device_ops_t hub_ops = {
	.control_transfer_handlers = endpoint_zero_handlers,
	.on_data = NULL,
	.on_data_request = req_on_data,
	.on_state_change = on_state_change,
};

/**
 * @}
 */
