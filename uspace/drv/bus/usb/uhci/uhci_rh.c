/*
 * Copyright (c) 2013 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
#include <async.h>
#include <ddi.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <time.h>

#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/classes/hub.h>
#include <usb/request.h>
#include <usb/host/endpoint.h>
#include <usb/usb.h>

#include "uhci_rh.h"

enum {
	UHCI_RH_PORT_COUNT = 2,
	UHCI_PORT_BYTES = STATUS_BYTES(UHCI_RH_PORT_COUNT),
};

/** Hub descriptor. */
static const struct {
	/** Common hub descriptor header */
	usb_hub_descriptor_header_t header;
	/** Port removable status bits */
	uint8_t removable[UHCI_PORT_BYTES];
	/** Port powered status bits */
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

/** Initialize uhci rh structure.
 * @param instance Memory place to initialize.
 * @param ports Pointer to TWO UHCI RH port registers.
 * @param name device name, passed to virthub init
 * @return Error code, EOK on success.
 */
errno_t uhci_rh_init(uhci_rh_t *instance, ioport16_t *ports, const char *name)
{
	assert(instance);
	instance->ports[0] = ports;
	instance->ports[1] = ports + 1;
	instance->reset_changed[0] = false;
	instance->reset_changed[1] = false;
	return virthub_base_init(&instance->base, name, &ops, instance,
	    NULL, &hub_descriptor.header, HUB_STATUS_CHANGE_PIPE);
}

/** Schedule USB batch for the root hub.
 *
 * @param instance UHCI rh instance
 * @param batch USB communication batch
 * @return EOK.
 *
 * The result of scheduling is always EOK. The result of communication does
 * not have to be.
 */
errno_t uhci_rh_schedule(uhci_rh_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);

	do {
		batch->error = virthub_base_request(&instance->base, batch->target,
		    batch->dir, (void *) batch->setup.buffer,
		    batch->dma_buffer.virt, batch->size, &batch->transferred_size);
		if (batch->error == ENAK)
			fibril_usleep(instance->base.endpoint_descriptor.poll_interval * 1000);
		//TODO This is flimsy, but we can't exit early because
		//ENAK is technically an error condition
	} while (batch->error == ENAK);
	usb_transfer_batch_finish(batch);
	return EOK;
}

/** UHCI port register bits */
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
	fibril_usleep(50000);
	port_status = pio_read_16(port);
	port_status &= ~STATUS_IN_RESET;
	pio_write_16(port, port_status);
	while ((port_status = pio_read_16(port)) & STATUS_IN_RESET)
		;
	/*
	 * PIO delay, should not be longer than 3ms as the device might
	 * enter suspend state.
	 */
	udelay(10);
	/*
	 * Drop ConnectionChange as some UHCI hw
	 * sets this bit after reset, that is incorrect
	 */
	port_status &= ~STATUS_WC_BITS;
	pio_write_16(port, port_status | STATUS_ENABLED | STATUS_CONNECTED_CHANGED);
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

#define RH_DEBUG(hub, port, msg, ...) \
do { \
	if ((int)port >= 0) \
		usb_log_debug("RH(%p-%d): " msg, hub, port, ##__VA_ARGS__); \
	else \
		usb_log_debug("RH(%p):" msg, hub, ##__VA_ARGS__); \
} while (0)

/** USB HUB port state request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 *
 * Do not confuse with port status. Port state reports data line states,
 * it is usefull for debuging purposes only.
 */
static errno_t req_get_port_state(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	uhci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(1, port, hub);
	if (setup_packet->value != 0)
		return EINVAL;

	const uint16_t value = pio_read_16(hub->ports[port]);
	data[0] = ((value & STATUS_LINE_D_MINUS) ? 1 : 0) |
	    ((value & STATUS_LINE_D_PLUS) ? 2 : 0);
	RH_DEBUG(hub, port, "Bus state %" PRIx8 "(source %" PRIx16 ")",
	    data[0], value);
	*act_size = 1;
	return EOK;
}

#define BIT_VAL(val, bit) \
	((val & bit) ? 1 : 0)
#define UHCI2USB(val, bit, mask) \
	(BIT_VAL(val, bit) ? (mask) : 0)

/** Port status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 *
 * Converts status reported via ioport to USB format.
 * @note: reset change status needs to be handled in sw.
 */
static errno_t req_get_port_status(usbvirt_device_t *device,
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
	    UHCI2USB(val, STATUS_CONNECTED, USB_HUB_PORT_STATUS_CONNECTION) |
	    UHCI2USB(val, STATUS_ENABLED, USB_HUB_PORT_STATUS_ENABLE) |
	    UHCI2USB(val, STATUS_SUSPEND, USB2_HUB_PORT_STATUS_SUSPEND) |
	    UHCI2USB(val, STATUS_IN_RESET, USB_HUB_PORT_STATUS_RESET) |
	    UHCI2USB(val, STATUS_ALWAYS_ONE, USB2_HUB_PORT_STATUS_POWER) |
	    UHCI2USB(val, STATUS_LOW_SPEED, USB2_HUB_PORT_STATUS_LOW_SPEED) |
	    UHCI2USB(val, STATUS_CONNECTED_CHANGED, USB_HUB_PORT_STATUS_C_CONNECTION) |
	    UHCI2USB(val, STATUS_ENABLED_CHANGED, USB2_HUB_PORT_STATUS_C_ENABLE) |
#if 0
	    UHCI2USB(val, STATUS_SUSPEND, USB2_HUB_PORT_STATUS_C_SUSPEND) |
#endif
	    (hub->reset_changed[port] ?  USB_HUB_PORT_STATUS_C_RESET : 0));
	RH_DEBUG(hub, port, "Port status %" PRIx32 " (source %" PRIx16
	    "%s)", uint32_usb2host(status), val,
	    hub->reset_changed[port] ? "-reset" : "");
	memcpy(data, &status, sizeof(status));
	*act_size = sizeof(status);
	return EOK;
}

/** Port clear feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_clear_port_feature(usbvirt_device_t *device,
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
	case USB2_HUB_FEATURE_PORT_ENABLE:
		RH_DEBUG(hub, port, "Clear port enable (status %"
		    PRIx16 ")", status);
		pio_write_16(hub->ports[port], val & ~STATUS_ENABLED);
		break;
	case USB2_HUB_FEATURE_PORT_SUSPEND:
		RH_DEBUG(hub, port, "Clear port suspend (status %"
		    PRIx16 ")", status);
		pio_write_16(hub->ports[port], val & ~STATUS_SUSPEND);
		// TODO we should do resume magic
		usb_log_warning("Resume is not implemented on port %u", port);
		break;
	case USB_HUB_FEATURE_PORT_POWER:
		RH_DEBUG(hub, port, "Clear port power (status %" PRIx16 ")",
		    status);
		/* We are always powered */
		usb_log_warning("Tried to power off port %u", port);
		break;
	case USB_HUB_FEATURE_C_PORT_CONNECTION:
		RH_DEBUG(hub, port, "Clear port conn change (status %"
		    PRIx16 ")", status);
		pio_write_16(hub->ports[port], val | STATUS_CONNECTED_CHANGED);
		break;
	case USB_HUB_FEATURE_C_PORT_RESET:
		RH_DEBUG(hub, port, "Clear port reset change (status %"
		    PRIx16 ")", status);
		hub->reset_changed[port] = false;
		break;
	case USB2_HUB_FEATURE_C_PORT_ENABLE:
		RH_DEBUG(hub, port, "Clear port enable change (status %"
		    PRIx16 ")", status);
		pio_write_16(hub->ports[port], status | STATUS_ENABLED_CHANGED);
		break;
	case USB2_HUB_FEATURE_C_PORT_SUSPEND:
		RH_DEBUG(hub, port, "Clear port suspend change (status %"
		    PRIx16 ")", status);
		//TODO
		return ENOTSUP;
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
		RH_DEBUG(hub, port, "Clear port OC change (status %"
		    PRIx16 ")", status);
		/* UHCI Does not report over current */
		//TODO: newer chips do, but some have broken wiring
		break;
	default:
		RH_DEBUG(hub, port, "Clear unknown feature %d (status %"
		    PRIx16 ")", feature, status);
		usb_log_warning("Clearing feature %d is unsupported",
		    feature);
		return ESTALL;
	}
	return EOK;
}

/** Port set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_set_port_feature(usbvirt_device_t *device,
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
		RH_DEBUG(hub, port, "Set port reset before (status %" PRIx16
		    ")", status);
		uhci_port_reset_enable(hub->ports[port]);
		hub->reset_changed[port] = true;
		RH_DEBUG(hub, port, "Set port reset after (status %" PRIx16
		    ")", pio_read_16(hub->ports[port]));
		break;
	case USB2_HUB_FEATURE_PORT_SUSPEND:
		RH_DEBUG(hub, port, "Set port suspend (status %" PRIx16
		    ")", status);
		pio_write_16(hub->ports[port],
		    (status & ~STATUS_WC_BITS) | STATUS_SUSPEND);
		usb_log_warning("Suspend is not implemented on port %u", port);
		break;
	case USB_HUB_FEATURE_PORT_POWER:
		RH_DEBUG(hub, port, "Set port power (status %" PRIx16
		    ")", status);
		/* We are always powered */
		usb_log_warning("Tried to power port %u", port);
		break;
	case USB_HUB_FEATURE_C_PORT_CONNECTION:
	case USB2_HUB_FEATURE_C_PORT_ENABLE:
	case USB2_HUB_FEATURE_C_PORT_SUSPEND:
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:
		RH_DEBUG(hub, port, "Set port change flag (status %" PRIx16
		    ")", status);
		/*
		 * These are voluntary and don't have to be set
		 * there is no way we could do it on UHCI anyway
		 */
		break;
	default:
		RH_DEBUG(hub, port, "Set unknown feature %d (status %" PRIx16
		    ")", feature, status);
		usb_log_warning("Setting feature %d is unsupported",
		    feature);
		return ESTALL;
	}
	return EOK;
}

/** Status change handler.
 * @param device Virtual hub device
 * @param endpoint Endpoint number
 * @param tr_type Transfer type
 * @param buffer Response destination
 * @param buffer_size Bytes available in buffer
 * @param actual_size Size us the used part of the dest buffer.
 *
 * Produces status mask. Bit 0 indicates hub status change the other bits
 * represent port status change. Endian does not matter as UHCI root hubs
 * only need 1 byte.
 */
static errno_t req_status_change_handler(usbvirt_device_t *device,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	uhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (buffer_size < 1)
		return ESTALL;

	const uint16_t status_a = pio_read_16(hub->ports[0]);
	const uint16_t status_b = pio_read_16(hub->ports[1]);
	const uint8_t status =
	    ((((status_a & STATUS_CHANGE_BITS) != 0) || hub->reset_changed[0]) ?
	    0x2 : 0) |
	    ((((status_b & STATUS_CHANGE_BITS) != 0) || hub->reset_changed[1]) ?
	    0x4 : 0);
	if (status)
		RH_DEBUG(hub, -1, "Event mask %" PRIx8
		    " (status_a %" PRIx16 "%s),"
		    " (status_b %" PRIx16 "%s)", status,
		    status_a, hub->reset_changed[0] ? "-reset" : "",
		    status_b, hub->reset_changed[1] ? "-reset" : "");
	((uint8_t *)buffer)[0] = status;
	*actual_size = 1;
	return (status != 0 ? EOK : ENAK);
}

/** UHCI root hub request handlers */
static const usbvirt_control_request_handler_t control_transfer_handlers[] = {
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
		/*
		 * Hub features are overcurrent and supply good,
		 * this request may only clear changes that we never report
		 */
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
		/*
		 * UHCI can't report OC condition or,
		 * lose power source
		 */
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
		/*
		 * Hub features are overcurrent and supply good,
		 * this request may only set changes that we never report
		 */
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

static usbvirt_device_ops_t ops = {
	.control = control_transfer_handlers,
	.data_in[HUB_STATUS_CHANGE_PIPE] = req_status_change_handler,
};
