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
static int on_class_request(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data);
static int on_data_request(struct usbvirt_device *dev,
    usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *actual_size);
static void set_port_state(hub_port_t *, hub_port_state_t);

/** Standard USB requests. */
static usbvirt_standard_device_request_ops_t standard_request_ops = {
	.on_get_status = NULL,
	.on_clear_feature = NULL,
	.on_set_feature = NULL,
	.on_set_address = NULL,
	.on_get_descriptor = on_get_descriptor,
	.on_set_descriptor = NULL,
	.on_get_configuration = NULL,
	.on_set_configuration = on_set_configuration,
	.on_get_interface = NULL,
	.on_set_interface = NULL,
	.on_synch_frame = NULL
};

/** Hub operations. */
usbvirt_device_ops_t hub_ops = {
	.standard_request_ops = &standard_request_ops,
	.on_class_device_request = on_class_request,
	.on_data = NULL,
	.on_data_request = on_data_request
};

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
	
	if (change->port->state == change->old_state) {
		set_port_state(change->port, change->new_state);
	}
	
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
	dprintf(1, "setting port %d state to %d (%c)", port->index,
	    state, hub_port_state_as_char(state));
	
	if (state == HUB_PORT_STATE_POWERED_OFF) {
		clear_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
		clear_port_status_change(port, HUB_STATUS_C_PORT_ENABLE);
		clear_port_status_change(port, HUB_STATUS_C_PORT_RESET);
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
		set_port_status_change(port, HUB_STATUS_C_PORT_RESET);
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
	
	switch (feature) {
		case USB_HUB_FEATURE_PORT_ENABLE:
			if ((port->state != HUB_PORT_STATE_NOT_CONFIGURED)
			    && (port->state != HUB_PORT_STATE_POWERED_OFF)) {
				set_port_state(port, HUB_PORT_STATE_DISABLED);
			}
			return EOK;
		
		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port->state != HUB_PORT_STATE_SUSPENDED) {
				return EOK;
			}
			set_port_state(port, HUB_PORT_STATE_RESUMING);
			return EOK;
			
		case USB_HUB_FEATURE_PORT_POWER:
			if (port->state != HUB_PORT_STATE_NOT_CONFIGURED) {
				set_port_state(port, HUB_PORT_STATE_POWERED_OFF);
			}
			return EOK;
		
		case USB_HUB_FEATURE_C_PORT_CONNECTION:
			clear_port_status_change(port, HUB_STATUS_C_PORT_CONNECTION);
			return EOK;
		
		case USB_HUB_FEATURE_C_PORT_ENABLE:
			clear_port_status_change(port, HUB_STATUS_C_PORT_ENABLE);
			return EOK;
		
		case USB_HUB_FEATURE_C_PORT_SUSPEND:
			clear_port_status_change(port, HUB_STATUS_C_PORT_SUSPEND);
			return EOK;
			
		case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
			clear_port_status_change(port, HUB_STATUS_C_PORT_OVER_CURRENT);
			return EOK;
	}
	
	return ENOTSUP;
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

static int get_port_status(uint16_t portindex)
{
	_GET_PORT(port, portindex);
	
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
	
	switch (feature) {
		case USB_HUB_FEATURE_PORT_RESET:
			if (port->state != HUB_PORT_STATE_POWERED_OFF) {
				set_port_state(port, HUB_PORT_STATE_RESETTING);
			}
			return EOK;
		
		case USB_HUB_FEATURE_PORT_SUSPEND:
			if (port->state == HUB_PORT_STATE_ENABLED) {
				set_port_state(port, HUB_PORT_STATE_SUSPENDED);
			}
			return EOK;
		
		case USB_HUB_FEATURE_PORT_POWER:
			if (port->state == HUB_PORT_STATE_POWERED_OFF) {
				set_port_state(port, HUB_PORT_STATE_DISCONNECTED);
			}
			return EOK;
	}
	return ENOTSUP;
}

#undef _GET_PORT


/** Callback for class request. */
static int on_class_request(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{	
	dprintf(2, "hub class request (%d)", (int) request->request);
	
	uint8_t recipient = request->request_type & 31;
	uint8_t direction = request->request_type >> 7;
	
#define _VERIFY(cond) \
	do { \
		if (!(cond)) { \
			dprintf(0, "WARN: invalid class request (%s not met).\n", \
			    NAME, #cond); \
			return EINVAL; \
		} \
	} while (0)
	
	switch (request->request) {
		case USB_HUB_REQUEST_CLEAR_FEATURE:
			_VERIFY(direction == 0);
			_VERIFY(request->length == 0);
			if (recipient == 0) {
				_VERIFY(request->index == 0);
				return clear_hub_feature(request->value);
			} else {
				_VERIFY(recipient == 3);
				return clear_port_feature(request->value,
				    request->index);
			}
			
		case USB_HUB_REQUEST_GET_STATE:
			return get_bus_state(request->index);
			
		case USB_HUB_REQUEST_GET_DESCRIPTOR:
			return get_hub_descriptor(dev, request->value_low,
			    request->value_high, request->length);
			
		case USB_HUB_REQUEST_GET_STATUS:
			if (recipient == 0) {
				return get_hub_status();
			} else {
				return get_port_status(request->index);
			}
			
		case USB_HUB_REQUEST_SET_FEATURE:
			if (recipient == 0) {
				return set_hub_feature(request->value);
			} else {
				return set_port_feature(request->value, request->index);
			}
			
		default:
			dprintf(0, "WARN: unknown request (%d)!\n",
			    request->request);
			break;
	}
	
#undef _VERIFY	


	return EOK;
}

void clear_port_status_change(hub_port_t *port, uint16_t change)
{
	port->status_change &= (~change);
}

void set_port_status_change(hub_port_t *port, uint16_t change)
{
	port->status_change |= change;
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
		
		if (port->status_change != 0) {
			change_map |= (1 << (i + 1));
		}
	}
	
	uint8_t *b = (uint8_t *) buffer;
	if (size > 0) {
		*b = change_map;
		*actual_size = 1;
	}
	
	return EOK;
}


/**
 * @}
 */
