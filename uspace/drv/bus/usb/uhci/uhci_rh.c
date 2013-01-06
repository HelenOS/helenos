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

#include <assert.h>
#include <macros.h>
#include <usb/debug.h>
#include <ddi.h>

#include "uhci_rh.h"

enum {
	UHCI_RH_PORT_COUNT = 2,
	/* 1 byte for hub status bit and 2 port status bits */
	UHCI_PORT_BYTES = 1,
};

/** Hub descriptor. */
static const struct {
	usb_hub_descriptor_header_t header;
	uint8_t removable[UHCI_PORT_BYTES];
	uint8_t powered[UHCI_PORT_BYTES];
} __attribute__((packed)) hub_descriptor = {
	.header = {
		.length = sizeof(hub_descriptor),
		.descriptor_type = USB_DESCTYPE_HUB,
		.port_count = UHCI_RH_PORT_COUNT,
		.characteristics =
		    HUB_CHAR_NO_POWER_SWITCH_FLAG | HUB_CHAR_NO_OC_FLAG,
		.power_good_time = 50,
		.max_current = 0,
	},
	.removable = { 0 },
	.powered = { 0xFF },
};


static usbvirt_device_ops_t ops;

int uhci_rh_init(uhci_rh_t *instance, ioport16_t *ports, const char *name)
{
	assert(instance);
	instance->ports[0] = ports;
	instance->ports[1] = ports + 1;
	instance->reset_changed[0] = false;
	instance->reset_changed[1] = false;
	return virthub_base_init(&instance->base, name, &ops, instance,
	    NULL, &hub_descriptor.header);
}

int uhci_rh_schedule(uhci_rh_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);

	const usb_target_t target = {{
		.address = batch->ep->address,
		.endpoint = batch->ep->endpoint
	}};
	batch->error = virthub_base_request(&instance->base, target,
	    usb_transfer_batch_direction(batch),
	    (void*)batch->setup_buffer, batch->buffer,
	    batch->buffer_size, &batch->transfered_size);
	usb_transfer_batch_finish(batch, NULL);
	usb_transfer_batch_destroy(batch);
	return EOK;
}
enum {
	STATUS_CONNECTED         = (1 << 0),
	STATUS_CONNECTED_CHANGED = (1 << 1),
	STATUS_ENABLED           = (1 << 2),
	STATUS_ENABLED_CHANGED   = (1 << 3),
	STATUS_LINE_D_PLUS       = (1 << 4),
	STATUS_LINE_D_MINUS      = (1 << 5),
	STATUS_RESUME            = (1 << 6),
	STATUS_ALWAYS_ONE        = (1 << 7),

	STATUS_LOW_SPEED = (1 <<  8),
	STATUS_IN_RESET  = (1 <<  9),
	STATUS_SUSPEND   = (1 << 12),

	STATUS_CHANGE_BITS = STATUS_CONNECTED_CHANGED | STATUS_ENABLED_CHANGED,
	STATUS_WC_BITS = STATUS_CHANGE_BITS,
};
/* HUB ROUTINES IMPLEMENTATION */
static void uhci_port_reset_enable(ioport16_t *port)
{
	assert(port);
	uint16_t port_status = pio_read_16(port);
	/* We don't wan to remove changes, that's hub drivers work */
	port_status &= ~STATUS_WC_BITS;
	port_status |= STATUS_IN_RESET;
	pio_write_16(port, port_status);
	async_usleep(50000);
	port_status = pio_read_16(port);
	port_status &= ~STATUS_IN_RESET;
	pio_write_16(port, port_status);
	while ((port_status = pio_read_16(port)) & STATUS_IN_RESET);
	/* PIO delay, should not be longer than 3ms as the device might
	 * enter suspend state. */
	udelay(10);
	/* Enable the port. */
	port_status &= ~STATUS_WC_BITS;
	pio_write_16(port, port_status | STATUS_ENABLED);
}
#define TEST_SIZE_INIT(size, port, hub) \
do { \
	if (uint16_usb2host(setup_packet->length) != size) \
		return ESTALL; \
	port = uint16_usb2host(setup_packet->index) - 1; \
	if (port != 0 && port != 1) \
		return EINVAL; \
	hub = virthub_get_data(device); \
	assert(hub);\
} while (0)

#define RH_DEBUG(d, port, msg, ...) \
	if ((int)port >= 0) \
		usb_log_debug("%s: rh-%d: " msg, d->name, port, ##__VA_ARGS__); \
	else \
		usb_log_debug("%s: rh: " msg, d->name, ##__VA_ARGS__) \

static int req_get_port_state(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	uhci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(1, port, hub);
	if (setup_packet->value != 0)
		return EINVAL;

	const uint16_t value = pio_read_16(hub->ports[port]);
	data[0] = ((value & STATUS_LINE_D_MINUS) ? 1 : 0)
	    | ((value & STATUS_LINE_D_PLUS) ? 2 : 0);
	RH_DEBUG(device, port, "Bus state %" PRIx8 "(source %" PRIx16")\n",
	    data[0], value);
	*act_size = 1;
	return EOK;
}

#define BIT_VAL(val, bit) \
	((val & bit) ? 1 : 0)
#define UHCI2USB(val, bit, feat) \
	(BIT_VAL(val, bit) << feat)

static int req_get_port_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	uhci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(4, port, hub);
	if (setup_packet->value != 0)
		return EINVAL;

	const uint16_t val = pio_read_16(hub->ports[port]);
	const uint32_t status = uint32_host2usb(
	    UHCI2USB(val, STATUS_CONNECTED, USB_HUB_FEATURE_PORT_CONNECTION) |
	    UHCI2USB(val, STATUS_ENABLED, USB_HUB_FEATURE_PORT_ENABLE) |
	    UHCI2USB(val, STATUS_SUSPEND, USB_HUB_FEATURE_PORT_SUSPEND) |
	    UHCI2USB(val, STATUS_IN_RESET, USB_HUB_FEATURE_PORT_RESET) |
	    UHCI2USB(val, STATUS_ALWAYS_ONE, USB_HUB_FEATURE_PORT_POWER) |
	    UHCI2USB(val, STATUS_LOW_SPEED, USB_HUB_FEATURE_PORT_LOW_SPEED) |
	    UHCI2USB(val, STATUS_CONNECTED_CHANGED, USB_HUB_FEATURE_C_PORT_CONNECTION) |
	    UHCI2USB(val, STATUS_ENABLED_CHANGED, USB_HUB_FEATURE_C_PORT_ENABLE) |
//	    UHCI2USB(val, STATUS_SUSPEND, USB_HUB_FEATURE_C_PORT_SUSPEND) |
	    ((hub->reset_changed[port] ? 1 : 0) << USB_HUB_FEATURE_C_PORT_RESET)
	);
	RH_DEBUG(device, port, "Port status %" PRIx32 " (source %" PRIx16
	    "%s)\n", uint32_usb2host(status), val,
	    hub->reset_changed[port] ? "-reset" : "");
	memcpy(data, &status, sizeof(status));
	*act_size = sizeof(status);;
	return EOK;
}

static int req_clear_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	uhci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	const uint16_t status = pio_read_16(hub->ports[port]);
	const uint16_t val = status & (~STATUS_WC_BITS);
	switch (feature) {
	case USB_HUB_FEATURE_PORT_ENABLE:
		RH_DEBUG(device, port, "Clear port enable (status %" PRIx16 ")\n",
		    status);
		pio_write_16(hub->ports[port], val & ~STATUS_ENABLED);
		break;
	case USB_HUB_FEATURE_PORT_SUSPEND:
		RH_DEBUG(device, port, "Clear port suspend (status %" PRIx16 ")\n",
		    status);
		pio_write_16(hub->ports[port], val & ~STATUS_SUSPEND);
		// TODO we should do resume magic
		usb_log_warning("Resume is not implemented on port %u\n", port);
		break;
	case USB_HUB_FEATURE_PORT_POWER:
		RH_DEBUG(device, port, "Clear port power (status %" PRIx16 ")\n",
		    status);
		/* We are always powered */
		usb_log_warning("Tried to power off port %u\n", port);
		break;
	case USB_HUB_FEATURE_C_PORT_CONNECTION:
		RH_DEBUG(device, port, "Clear port conn change (status %" PRIx16
		    ")\n", status);
		pio_write_16(hub->ports[port], val | STATUS_CONNECTED_CHANGED);
		break;
	case USB_HUB_FEATURE_C_PORT_RESET:
		RH_DEBUG(device, port, "Clear port reset change (status %" PRIx16
		    ")\n", status);
		hub->reset_changed[port] = false;
		break;
	case USB_HUB_FEATURE_C_PORT_ENABLE:
		RH_DEBUG(device, port, "Clear port enable change (status %" PRIx16
		    ")\n", status);
		pio_write_16(hub->ports[port], status | STATUS_ENABLED_CHANGED);
		break;
	case USB_HUB_FEATURE_C_PORT_SUSPEND:
		RH_DEBUG(device, port, "Clear port suspend change (status %" PRIx16
		    ")\n", status);
		//TODO
		return ENOTSUP;
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
		RH_DEBUG(device, port, "Clear port OC change (status %" PRIx16
		    ")\n", status);
		/* UHCI Does not report over current */
		break;
	default:
		RH_DEBUG(device, port, "Clear unknown feature %d (status %" PRIx16
		    ")\n", feature, status);
		usb_log_warning("Clearing feature %d is unsupported\n",
		    feature);
		return ESTALL;
	}
	return EOK;
}

static int req_set_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	uhci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	const uint16_t status = pio_read_16(hub->ports[port]);
	switch (feature) {
	case USB_HUB_FEATURE_PORT_RESET:
		RH_DEBUG(device, port, "Set port reset before (status %" PRIx16
		    ")\n", status);
		uhci_port_reset_enable(hub->ports[port]);
		hub->reset_changed[port] = true;
		RH_DEBUG(device, port, "Set port reset after (status %" PRIx16
		    ")\n", pio_read_16(hub->ports[port]));
		break;
	case USB_HUB_FEATURE_PORT_SUSPEND:
		RH_DEBUG(device, port, "Set port suspend (status %" PRIx16
		    ")\n", status);
		pio_write_16(hub->ports[port],
		    (status & ~STATUS_WC_BITS) | STATUS_SUSPEND);
		usb_log_warning("Suspend is not implemented on port %u\n", port);
		break;
	case USB_HUB_FEATURE_PORT_POWER:
		RH_DEBUG(device, port, "Set port power (status %" PRIx16
		    ")\n", status);
		/* We are always powered */
		usb_log_warning("Tried to power port %u\n", port);
	case USB_HUB_FEATURE_C_PORT_CONNECTION:
	case USB_HUB_FEATURE_C_PORT_ENABLE:
	case USB_HUB_FEATURE_C_PORT_SUSPEND:
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
		RH_DEBUG(device, port, "Set port change flag (status %" PRIx16
		    ")\n", status);
		/* These are voluntary and don't have to be set
		 * there is no way we could do it on UHCI anyway */
		break;
	default:
		RH_DEBUG(device, port, "Set unknown feature %d (status %" PRIx16
		    ")\n", feature, status);
		usb_log_warning("Setting feature %d is unsupported\n",
		    feature);
		return ESTALL;
	}
	return EOK;
}

static usbvirt_control_request_handler_t control_transfer_handlers[] = {
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_DESCRIPTOR),
		.name = "GetHubDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATE),
		.name = "GetBusState",
		.callback = req_get_port_state,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		/* Hub features are overcurrent and supply good,
		 * this request may only set changes that we never report*/
		.callback = req_nop,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		/* UHCI can't report OC condition or,
		 * lose power source */
		.callback = virthub_base_get_null_status,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		/* Hub features are overcurrent and supply good,
		 * this request may only set changes that we never report*/
		.callback = req_nop,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature
	},
	{
		.callback = NULL
	}
};

static int req_status_change_handler(usbvirt_device_t *dev,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size);

static usbvirt_device_ops_t ops = {
        .control = control_transfer_handlers,
        .data_in[HUB_STATUS_CHANGE_PIPE] = req_status_change_handler,
};

static int req_status_change_handler(usbvirt_device_t *device,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	if (buffer_size < 1)
		return ESTALL;
	uhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	const uint16_t status_a = pio_read_16(hub->ports[0]);
	const uint16_t status_b = pio_read_16(hub->ports[1]);
	const uint8_t status =
	    ((((status_a & STATUS_CHANGE_BITS) != 0) || hub->reset_changed[0]) ?
	        0x2 : 0) |
	    ((((status_b & STATUS_CHANGE_BITS) != 0) || hub->reset_changed[1]) ?
	        0x4 : 0);

	RH_DEBUG(device, -1, "Event mask %" PRIx8
	    " (status_a %" PRIx16 "%s),"
	    " (status_b %" PRIx16 "%s)\n", status,
	    status_a, hub->reset_changed[0] ? "-reset" : "",
	    status_b, hub->reset_changed[1] ? "-reset" : "" );
	((uint8_t *)buffer)[0] = status;
	*actual_size = 1;
	return EOK;
}
