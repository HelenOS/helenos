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
 * @brief Virtual USB hub operations.
 */

#include <errno.h>
#include <usb/classes/hub.h>
#include <usbvirt/device.h>
#include "virthub.h"
#include "hub.h"

/** Callback when device changes states. */
static void on_state_change(usbvirt_device_t *dev,
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
static errno_t req_on_status_change_pipe(usbvirt_device_t *dev,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	if (endpoint != HUB_STATUS_CHANGE_PIPE) {
		return ESTALL;
	}
	if (tr_type != USB_TRANSFER_INTERRUPT) {
		return ESTALL;
	}

	hub_t *hub = dev->device_data;

	hub_acquire(hub);

	if (!hub->signal_changes) {
		hub_release(hub);

		return ENAK;
	}


	uint8_t change_map = hub_get_status_change_bitmap(hub);

	uint8_t *b = (uint8_t *) buffer;
	if (buffer_size > 0) {
		*b = change_map;
		*actual_size = 1;
	} else {
		*actual_size = 0;
	}

	hub->signal_changes = false;

	hub_release(hub);

	return EOK;
}

/** Handle ClearHubFeature request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_clear_hub_feature(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	return ENOTSUP;
}

/** Handle ClearPortFeature request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_clear_port_feature(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	errno_t rc;
	size_t port = request->index - 1;
	usb_hub_class_feature_t feature = request->value;
	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	hub_port_state_t port_state = hub_get_port_state(hub, port);

	switch (feature) {
		case USB2_HUB_FEATURE_PORT_ENABLE:
			if ((port_state != HUB_PORT_STATE_NOT_CONFIGURED)
			    && (port_state != HUB_PORT_STATE_POWERED_OFF)) {
				hub_set_port_state(hub, port, HUB_PORT_STATE_DISABLED);
			}
			rc = EOK;
			break;

		case USB2_HUB_FEATURE_PORT_SUSPEND:
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

		case USB2_HUB_FEATURE_C_PORT_ENABLE:
			hub_clear_port_status_change(hub, port, HUB_STATUS_C_PORT_ENABLE);
			rc = EOK;
			break;

		case USB2_HUB_FEATURE_C_PORT_SUSPEND:
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

/** Handle GetBusState request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_get_bus_state(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	return ENOTSUP;
}

/** Handle GetDescriptor request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_get_descriptor(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	if (request->value_high == USB_DESCTYPE_HUB) {
		usbvirt_control_reply_helper(request, data, act_size,
		    &hub_descriptor, hub_descriptor.length);

		return EOK;
	}
	/* Let the framework handle all the rest. */
	return EFORWARD;
}

/** Handle GetHubStatus request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_get_hub_status(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	uint32_t hub_status = 0;

	usbvirt_control_reply_helper(request, data, act_size,
	    &hub_status, sizeof(hub_status));

	return EOK;
}

/** Handle GetPortStatus request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_get_port_status(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	hub_t *hub = (hub_t *) dev->device_data;

	hub_acquire(hub);

	uint32_t status = hub_get_port_status(hub, request->index - 1);

	hub_release(hub);

	usbvirt_control_reply_helper(request, data, act_size,
	    &status, sizeof(status));

	return EOK;
}

/** Handle SetHubFeature request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_set_hub_feature(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	return ENOTSUP;
}

/** Handle SetPortFeature request.
 *
 * @param dev Virtual device representing the hub.
 * @param request The SETUP packet of the control request.
 * @param data Extra data (when DATA stage present).
 * @return Error code.
 */
static errno_t req_set_port_feature(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	errno_t rc = ENOTSUP;
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

		case USB2_HUB_FEATURE_PORT_SUSPEND:
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



/** Recipient: other. */
#define REC_OTHER USB_REQUEST_RECIPIENT_OTHER
/** Recipient: device. */
#define REC_DEVICE USB_REQUEST_RECIPIENT_DEVICE


/** Hub operations on control endpoint zero. */
static usbvirt_control_request_handler_t endpoint_zero_handlers[] = {
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetStdDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ_IN(REC_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetClassDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ_IN(REC_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ_OUT(REC_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		.callback = req_clear_hub_feature
	},
	{
		CLASS_REQ_OUT(REC_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature
	},
	{
		CLASS_REQ_IN(REC_OTHER, USB_HUB_REQUEST_GET_STATE),
		.name = "GetBusState",
		.callback = req_get_bus_state
	},
	{
		CLASS_REQ_IN(REC_DEVICE, USB_HUB_REQUEST_GET_DESCRIPTOR),
		.name = "GetHubDescriptor",
		.callback = req_get_descriptor
	},
	{
		CLASS_REQ_IN(REC_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		.callback = req_get_hub_status
	},
	{
		CLASS_REQ_IN(REC_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ_OUT(REC_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		.callback = req_set_hub_feature
	},
	{
		CLASS_REQ_OUT(REC_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature
	},
	{
		.callback = NULL
	}
};


/** Hub operations. */
usbvirt_device_ops_t hub_ops = {
	.control = endpoint_zero_handlers,
	.data_in[HUB_STATUS_CHANGE_PIPE] = req_on_status_change_pipe,
	.state_changed = on_state_change,
};

/**
 * @}
 */
