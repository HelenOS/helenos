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
#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usbvirt/hub.h>
#include <usbvirt/device.h>
#include <errno.h>

#include "vhcd.h"
#include "hub.h"
#include "hubintern.h"

/** Produce a byte from bit values.
 */
#define MAKE_BYTE(b0, b1, b2, b3, b4, b5, b6, b7) \
	(( \
		((b0) << 0) \
		| ((b1) << 1) \
		| ((b2) << 2) \
		| ((b3) << 3) \
		| ((b4) << 4) \
		| ((b5) << 5) \
		| ((b6) << 6) \
		| ((b7) << 7) \
	))

static int on_get_descriptor(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data);
static int on_set_configuration(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data);
static int on_data_request(struct usbvirt_device *dev,
    usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size);
static void set_port_state(hub_port_t *, hub_port_state_t);
static void clear_port_status_change_nl(hub_port_t *, uint16_t);
static void set_port_state_nl(hub_port_t *, hub_port_state_t);
static int get_port_status(uint16_t portindex);

/** Callback for GET_DESCRIPTOR request. */
static int on_get_descriptor(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{
	if (request->value_high == USB_DESCTYPE_HUB) {
		int rc = dev->control_transfer_reply(dev, 0,
		    &hub_descriptor, hub_descriptor.length);
		
		return rc;
	}
	/* Let the framework handle all the rest. */
	return EFORWARD;
}

/** Callback for SET_CONFIGURATION request. */
int on_set_configuration(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{
	/* We must suspend power source to all ports. */
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub_dev.ports[i];
		
		set_port_state(port, HUB_PORT_STATE_POWERED_OFF);
	}
	
	/* Let the framework handle the rest of the job. */
	return EFORWARD;
}

struct delay_port_state_change {
	suseconds_t delay;
	hub_port_state_t old_state;
	hub_port_state_t new_state;
	hub_port_t *port;
};

static int set_port_state_delayed_fibril(void *arg)
{
	struct delay_port_state_change *change
	    = (struct delay_port_state_change *) arg;
	
	async_usleep(change->delay);
	
	fibril_mutex_lock(&change->port->guard);
	if (change->port->state == change->old_state) {
		set_port_state_nl(change->port, change->new_state);
	}
	fibril_mutex_unlock(&change->port->guard);
	
	free(change);
	
	return EOK;
}

static void set_port_state_delayed(hub_port_t *port,
    suseconds_t delay_time,
    hub_port_state_t old_state, hub_port_state_t new_state)
{
	struct delay_port_state_change *change
	    = malloc(sizeof(struct delay_port_state_change));
	change->port = port;
	change->delay = delay_time;
	change->old_state = old_state;
	change->new_state = new_state;
	fid_t fibril = fibril_create(set_port_state_delayed_fibril, change);
	if (fibril == 0) {
		printf("Failed to create fibril\n");
		return;
	}
	fibril_add_ready(fibril);
}

/** Change port status and updates status change status fields.
 */
void set_port_state(hub_port_t *port, hub_port_state_t state)
{
	fibril_mutex_lock(&port->guard);
	set_port_state_nl(port, state);
	fibril_mutex_unlock(&port->guard);
}

void set_port_state_nl(hub_port_t *port, hub_port_state_t state)
{

	dprintf(2, "setting port %d state to %d (%c) from %c (change=%u)",
	    port->index,
	    state, hub_port_state_as_char(state),
	    hub_port_state_as_char(port->state),
	    (unsigned int) port->status_change);
	
	if (state == HUB_PORT_STATE_POWERED_OFF) {
		clear_port_status_change_nl(port, HUB_STATUS_C_PORT_CONNECTION);
		clear_port_status_change_nl(port, HUB_STATUS_C_PORT_ENABLE);
		clear_port_status_change_nl(port, HUB_STATUS_C_PORT_RESET);
	}
	if (state == HUB_PORT_STATE_RESUMING) {
		set_port_state_delayed(port, 10*1000,
		    HUB_PORT_STATE_RESUMING, HUB_PORT_STATE_ENABLED);
	}
	if (state == HUB_PORT_STATE_RESETTING) {
		set_port_state_delayed(port, 10*1000,
		    HUB_PORT_STATE_RESETTING, HUB_PORT_STATE_ENABLED);
	}
	if ((port->state == HUB_PORT_STATE_RESETTING)
	    && (state == HUB_PORT_STATE_ENABLED)) {
		set_port_status_change_nl(port, HUB_STATUS_C_PORT_RESET);
	}
	
	port->state = state;
}

/** Get access to a port or return with EINVAL. */
#define _GET_PORT(portvar, index) \
	do { \
		if (virthub_dev.state != USBVIRT_STATE_CONFIGURED) { \
			return EINVAL; \
		} \
		if (((index) == 0) || ((index) > HUB_PORT_COUNT)) { \
			return EINVAL; \
		} \
	} while (false); \
	hub_port_t *portvar = &hub_dev.ports[index-1]


static int clear_hub_feature(uint16_t feature)
{
	return ENOTSUP;
}

static int clear_port_feature(uint16_t feature, uint16_t portindex)
{	
	_GET_PORT(port, portindex);
	
	fibril_mutex_lock(&port->guard);
	int rc = ENOTSUP;

	switch (feature) {
		case USB_HUB_FEATURE_PORT_ENABLE:
			if ((port->state != HUB_PORT_STATE_NOT_CONFIGURED)
			    && (port->state != HUB_PORT_STATE_POWERED_OFF)) {
				set_port_state_nl(port, HUB_PORT_STATE_DISABLED);
			}
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port->state != HUB_PORT_STATE_SUSPENDED) {
				rc = EOK;
				break;
			}
			set_port_state_nl(port, HUB_PORT_STATE_RESUMING);
			rc = EOK;
			break;
			
		case USB_HUB_FEATURE_PORT_POWER:
			if (port->state != HUB_PORT_STATE_NOT_CONFIGURED) {
				set_port_state_nl(port, HUB_PORT_STATE_POWERED_OFF);
			}
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_C_PORT_CONNECTION:
			clear_port_status_change_nl(port, HUB_STATUS_C_PORT_CONNECTION);
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_C_PORT_ENABLE:
			clear_port_status_change_nl(port, HUB_STATUS_C_PORT_ENABLE);
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_C_PORT_SUSPEND:
			clear_port_status_change_nl(port, HUB_STATUS_C_PORT_SUSPEND);
			rc = EOK;
			break;
			
		case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
			clear_port_status_change_nl(port, HUB_STATUS_C_PORT_OVER_CURRENT);
			rc = EOK;
			break;

		case USB_HUB_FEATURE_C_PORT_RESET:
			clear_port_status_change_nl(port, HUB_STATUS_C_PORT_RESET);
			rc = EOK;
			break;
	}
	
	fibril_mutex_unlock(&port->guard);

	return rc;
}

static int get_bus_state(uint16_t portindex)
{
	return ENOTSUP;
}

static int get_hub_descriptor(struct usbvirt_device *dev,
    uint8_t descriptor_index,
    uint8_t descriptor_type, uint16_t length)
{
	if (descriptor_type == USB_DESCTYPE_HUB) {
		int rc = dev->control_transfer_reply(dev, 0,
		    &hub_descriptor, hub_descriptor.length);

		return rc;

	}

	return ENOTSUP;
}

static int get_hub_status(void)
{
	uint32_t hub_status = 0;
	
	return virthub_dev.control_transfer_reply(&virthub_dev, 0,
	    &hub_status, 4);
}

int get_port_status(uint16_t portindex)
{
	_GET_PORT(port, portindex);
	
	fibril_mutex_lock(&port->guard);

	uint32_t status;
	status = MAKE_BYTE(
	    /* Current connect status. */
	    port->device == NULL ? 0 : 1,
	    /* Port enabled/disabled. */
	    port->state == HUB_PORT_STATE_ENABLED ? 1 : 0,
	    /* Suspend. */
	    (port->state == HUB_PORT_STATE_SUSPENDED)
	        || (port->state == HUB_PORT_STATE_RESUMING) ? 1 : 0,
	    /* Over-current. */
	    0,
	    /* Reset. */
	    port->state == HUB_PORT_STATE_RESETTING ? 1 : 0,
	    /* Reserved. */
	    0, 0, 0)
	    
	    | (MAKE_BYTE(
	    /* Port power. */
	    port->state == HUB_PORT_STATE_POWERED_OFF ? 0 : 1,
	    /* Full-speed device. */
	    0,
	    /* Reserved. */
	    0, 0, 0, 0, 0, 0
	    )) << 8;
	    
	status |= (port->status_change << 16);
	
	fibril_mutex_unlock(&port->guard);

	dprintf(2, "GetPortStatus(port=%d, status=%u)\n", (int)portindex,
	    (unsigned int) status);
	return virthub_dev.control_transfer_reply(&virthub_dev, 0, &status, 4);
}


static int set_hub_feature(uint16_t feature)
{
	return ENOTSUP;
}

static int set_port_feature(uint16_t feature, uint16_t portindex)
{
	_GET_PORT(port, portindex);
	
	fibril_mutex_lock(&port->guard);

	int rc = ENOTSUP;

	switch (feature) {
		case USB_HUB_FEATURE_PORT_RESET:
			if (port->state != HUB_PORT_STATE_POWERED_OFF) {
				set_port_state_nl(port, HUB_PORT_STATE_RESETTING);
			}
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port->state == HUB_PORT_STATE_ENABLED) {
				set_port_state_nl(port, HUB_PORT_STATE_SUSPENDED);
			}
			rc = EOK;
			break;
		
		case USB_HUB_FEATURE_PORT_POWER:
			if (port->state == HUB_PORT_STATE_POWERED_OFF) {
				set_port_state_nl(port, HUB_PORT_STATE_DISCONNECTED);
			}
			rc = EOK;
			break;
	}

	fibril_mutex_unlock(&port->guard);
	return rc;
}

#undef _GET_PORT


void clear_port_status_change_nl(hub_port_t *port, uint16_t change)
{
	port->status_change &= (~change);
	dprintf(2, "cleared port %d status change %d (%u)", port->index,
	    (int)change, (unsigned int) port->status_change);
}

void set_port_status_change_nl(hub_port_t *port, uint16_t change)
{
	port->status_change |= change;
	dprintf(2, "set port %d status change %d (%u)", port->index,
	    (int)change, (unsigned int) port->status_change);

}

void clear_port_status_change(hub_port_t *port, uint16_t change)
{
	fibril_mutex_lock(&port->guard);
	clear_port_status_change_nl(port, change);
	fibril_mutex_unlock(&port->guard);
}

void set_port_status_change(hub_port_t *port, uint16_t change)
{
	fibril_mutex_lock(&port->guard);
	set_port_status_change_nl(port, change);
	fibril_mutex_unlock(&port->guard);
}

/** Callback for data request. */
static int on_data_request(struct usbvirt_device *dev,
    usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size)
{
	if (endpoint != HUB_STATUS_CHANGE_PIPE) {
		return EINVAL;
	}
	
	uint8_t change_map = 0;
	
	size_t i;
	for (i = 0; i < HUB_PORT_COUNT; i++) {
		hub_port_t *port = &hub_dev.ports[i];
		
		fibril_mutex_lock(&port->guard);
		if (port->status_change != 0) {
			change_map |= (1 << (i + 1));
		}
		fibril_mutex_unlock(&port->guard);
	}
	
	uint8_t *b = (uint8_t *) buffer;
	if (size > 0) {
		*b = change_map;
		*actual_size = 1;
	}
	
	return EOK;
}



static int req_clear_hub_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return clear_hub_feature(request->value);
}

static int req_clear_port_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return clear_port_feature(request->value, request->index);
}

static int req_get_bus_state(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return get_bus_state(request->index);
}

static int req_get_hub_descriptor(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return get_hub_descriptor(dev, request->value_low,
	    request->value_high, request->length);
}

static int req_get_hub_status(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return get_hub_status();
}

static int req_get_port_status(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return get_port_status(request->index);
}

static int req_set_hub_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return set_hub_feature(request->value);
}

static int req_set_port_feature(usbvirt_device_t *dev,
    usb_device_request_setup_packet_t *request,
    uint8_t *data)
{
	return set_port_feature(request->value, request->index);
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
		STD_REQ(DIR_OUT, REC_DEVICE, USB_DEVREQ_SET_CONFIGURATION),
		.name = "SetConfiguration",
		.callback = on_set_configuration
	},
	{
		STD_REQ(DIR_IN, REC_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = on_get_descriptor
	},
	{
		CLASS_REQ(DIR_IN, REC_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = on_get_descriptor
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
		.callback = req_get_hub_descriptor
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
	.on_data_request = on_data_request
};

/**
 * @}
 */
