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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#include <assert.h>
#include <errno.h>
#include <mem.h>
#include <stddef.h>
#include <stdint.h>

#include <usb/classes/hub.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb/usb.h>

#include <usb/host/endpoint.h>
#include <usbvirt/device.h>

#include "ohci_rh.h"

enum {
	HUB_STATUS_CHANGE_PIPE = 1,
};

static usbvirt_device_ops_t ops;

/** Initialize internal USB HUB class descriptor.
 * @param instance OHCI root hub.
 * Use register based info to create accurate descriptor.
 */
static void ohci_rh_hub_desc_init(ohci_rh_t *instance)
{
	assert(instance);
	const unsigned dsize = sizeof(usb_hub_descriptor_header_t) +
	    STATUS_BYTES(instance->port_count) * 2;
	assert(dsize <= sizeof(instance->hub_descriptor));
	const uint32_t hub_desc = OHCI_RD(instance->registers->rh_desc_a);
	const uint32_t port_desc = OHCI_RD(instance->registers->rh_desc_b);

	instance->hub_descriptor.header.length = dsize;
	instance->hub_descriptor.header.descriptor_type = USB_DESCTYPE_HUB;
	instance->hub_descriptor.header.port_count = instance->port_count;
	instance->hub_descriptor.header.characteristics = 0 |
	    /* Bits 0,1 indicate power switching mode */
	    ((hub_desc & RHDA_PSM_FLAG)  ? 0x01 : 0) |
	    ((hub_desc & RHDA_NPS_FLAG)  ? 0x02 : 0) |
	    /* Bit 2 indicates device type (compound device) */
	    ((hub_desc & RHDA_DT_FLAG)   ? 0x04 : 0) |
	    /* Bits 3,4 indicate over-current protection mode */
	    ((hub_desc & RHDA_OCPM_FLAG) ? 0x08 : 0) |
	    ((hub_desc & RHDA_NOCP_FLAG) ? 0x10 : 0);
	instance->hub_descriptor.header.power_good_time =
	    hub_desc >> RHDA_POTPGT_SHIFT;
	/* bHubContrCurrent, root hubs don't need no power. */
	instance->hub_descriptor.header.max_current = 0;

	/* Device Removable and some legacy 1.0 stuff*/
	instance->hub_descriptor.rempow[0] =
	    (port_desc >> RHDB_DR_SHIFT) & 0xff;
	if (STATUS_BYTES(instance->port_count) == 1) {
		instance->hub_descriptor.rempow[1] = 0xff;
	} else {
		instance->hub_descriptor.rempow[1] =
		     ((port_desc >> RHDB_DR_SHIFT) >> 8) & 0xff;
	}

	instance->hub_descriptor.rempow[2] = 0xff;
	instance->hub_descriptor.rempow[3] = 0xff;

}
/** Initialize OHCI root hub.
 * @param instance Place to initialize.
 * @param regs OHCI device registers.
 * @param name Device name.
 * return Error code, EOK on success.
 *
 * Selects preconfigured port powering mode, sets up descriptor, and
 * initializes internal virtual hub.
 */
errno_t ohci_rh_init(ohci_rh_t *instance, ohci_regs_t *regs, const char *name)
{
	assert(instance);
	instance->registers = regs;
	instance->port_count = OHCI_RD(regs->rh_desc_a) & RHDA_NDS_MASK;
	usb_log_debug2("rh_desc_a: %x.\n", OHCI_RD(regs->rh_desc_a));
	if (instance->port_count > OHCI_MAX_PORTS) {
		usb_log_warning("OHCI specification does not allow %d ports. "
		    "Max %d ports will be used.\n", instance->port_count,
		    OHCI_MAX_PORTS);
		instance->port_count = OHCI_MAX_PORTS;
	}
	usb_log_info("%s: Found %u ports.\n", name, instance->port_count);

#if defined OHCI_POWER_SWITCH_no
	usb_log_info("%s: Set power mode to no power switching.\n", name);
	/* Set port power mode to no power-switching. (always on) */
	OHCI_SET(regs->rh_desc_a, RHDA_NPS_FLAG);

	/* Set to no over-current reporting */
	OHCI_SET(regs->rh_desc_a, RHDA_NOCP_FLAG);

#elif defined OHCI_POWER_SWITCH_ganged
	usb_log_info("%s: Set power mode to ganged power switching.\n", name);
	/* Set port power mode to ganged power-switching. */
	OHCI_CLR(regs->rh_desc_a, RHDA_NPS_FLAG);
	OHCI_CLR(regs->rh_desc_a, RHDA_PSM_FLAG);

	/* Turn off power (hub driver will turn this back on)*/
	OHCI_WR(regs->rh_status, RHS_CLEAR_GLOBAL_POWER);

	/* Set to global over-current */
	OHCI_CLR(regs->rh_desc_a, RHDA_NOCP_FLAG);
	OHCI_CLR(regs->rh_desc_a, RHDA_OCPM_FLAG);
#else
	usb_log_info("%s: Set power mode to per-port power switching.\n", name);
	/* Set port power mode to per port power-switching. */
	OHCI_CLR(regs->rh_desc_a, RHDA_NPS_FLAG);
	OHCI_SET(regs->rh_desc_a, RHDA_PSM_FLAG);

	/* Control all ports by global switch and turn them off */
	OHCI_CLR(regs->rh_desc_b, RHDB_PCC_MASK << RHDB_PCC_SHIFT);
	OHCI_WR(regs->rh_status, RHS_CLEAR_GLOBAL_POWER);

	/* Return control to per port state */
	OHCI_SET(regs->rh_desc_b, RHDB_PCC_MASK << RHDB_PCC_SHIFT);

	/* Set per port over-current */
	OHCI_CLR(regs->rh_desc_a, RHDA_NOCP_FLAG);
	OHCI_SET(regs->rh_desc_a, RHDA_OCPM_FLAG);
#endif

	ohci_rh_hub_desc_init(instance);
	instance->unfinished_interrupt_transfer = NULL;
	return virthub_base_init(&instance->base, name, &ops, instance,
	    NULL, &instance->hub_descriptor.header, HUB_STATUS_CHANGE_PIPE);
}

/** Schedule USB request.
 * @param instance OCHI root hub instance.
 * @param batch USB requst batch to schedule.
 * @return Always EOK.
 * Most requests complete even before this function returns,
 * status change requests might be postponed until there is something to report.
 */
errno_t ohci_rh_schedule(ohci_rh_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	const usb_target_t target = {{
		.address = batch->ep->address,
		.endpoint = batch->ep->endpoint,
	}};
	batch->error = virthub_base_request(&instance->base, target,
	    usb_transfer_batch_direction(batch), (void*)batch->setup_buffer,
	    batch->buffer, batch->buffer_size, &batch->transfered_size);
	if (batch->error == ENAK) {
		/* This is safe because only status change interrupt transfers
		 * return NAK. The assertion holds true because the batch
		 * existence prevents communication with that ep */
		assert(instance->unfinished_interrupt_transfer == NULL);
		instance->unfinished_interrupt_transfer = batch;
	} else {
		usb_transfer_batch_finish(batch, NULL);
		usb_transfer_batch_destroy(batch);
	}
	return EOK;
}

/** Handle OHCI RHSC interrupt.
 * @param instance OHCI root hub isntance.
 * @return Always EOK.
 *
 * Interrupt means there is a change of status to report. It may trigger
 * processing of a postponed request.
 */
errno_t ohci_rh_interrupt(ohci_rh_t *instance)
{
	//TODO atomic swap needed
	usb_transfer_batch_t *batch = instance->unfinished_interrupt_transfer;
	instance->unfinished_interrupt_transfer = NULL;
	if (batch) {
		const usb_target_t target = {{
			.address = batch->ep->address,
			.endpoint = batch->ep->endpoint,
		}};
		batch->error = virthub_base_request(&instance->base, target,
		    usb_transfer_batch_direction(batch),
		    (void*)batch->setup_buffer,
		    batch->buffer, batch->buffer_size, &batch->transfered_size);
		usb_transfer_batch_finish(batch, NULL);
		usb_transfer_batch_destroy(batch);
	}
	return EOK;
}

/* HUB ROUTINES IMPLEMENTATION */
#define TEST_SIZE_INIT(size, port, hub) \
do { \
	hub = virthub_get_data(device); \
	assert(hub);\
	if (uint16_usb2host(setup_packet->length) != size) \
		return ESTALL; \
	port = uint16_usb2host(setup_packet->index) - 1; \
	if (port > hub->port_count) \
		return EINVAL; \
} while (0)

/** Hub status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_get_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ohci_rh_t *hub = virthub_get_data(device);
	assert(hub);
	if (uint16_usb2host(setup_packet->length) != 4)
		return ESTALL;
	const uint32_t val = OHCI_RD(hub->registers->rh_status) &
	    (RHS_LPS_FLAG | RHS_LPSC_FLAG | RHS_OCI_FLAG | RHS_OCIC_FLAG);
	memcpy(data, &val, sizeof(val));
	*act_size = sizeof(val);
	return EOK;
}

/** Hub set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_clear_hub_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ohci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	/*
	 * Chapter 11.16.2 specifies that only C_HUB_LOCAL_POWER and
	 * C_HUB_OVER_CURRENT are supported.
	 * C_HUB_LOCAL_POWER is not supported
	 * because root hubs do not support local power status feature.
	 * C_HUB_OVER_CURRENT is represented by OHCI RHS_OCIC_FLAG.
	 * (OHCI pg. 127)
	 */
	const unsigned feature = uint16_usb2host(setup_packet->value);
	if (feature == USB_HUB_FEATURE_C_HUB_OVER_CURRENT) {
		OHCI_WR(hub->registers->rh_status, RHS_OCIC_FLAG);
	}
	return EOK;
}

/** Port status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_get_port_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ohci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(4, port, hub);
	if (setup_packet->value != 0)
		return EINVAL;

	const uint32_t status = OHCI_RD(hub->registers->rh_port_status[port]);
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
	ohci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	/* Enabled features to clear: see page 269 of USB specs */
	switch (feature)
	{
	case USB_HUB_FEATURE_PORT_POWER:          /*8*/
		{
			const uint32_t rhda =
			    OHCI_RD(hub->registers->rh_desc_a);
			/* No power switching */
			if (rhda & RHDA_NPS_FLAG)
				return ENOTSUP;
			/* Ganged power switching, one port powers all */
			if (!(rhda & RHDA_PSM_FLAG)) {
				OHCI_WR(hub->registers->rh_status,
				    RHS_CLEAR_GLOBAL_POWER);
				return EOK;
			}
			OHCI_WR(hub->registers->rh_port_status[port],
			    RHPS_CLEAR_PORT_POWER);
			return EOK;
		}

	case USB_HUB_FEATURE_PORT_ENABLE:         /*1*/
		OHCI_WR(hub->registers->rh_port_status[port],
		    RHPS_CLEAR_PORT_ENABLE);
		return EOK;

	case USB_HUB_FEATURE_PORT_SUSPEND:        /*2*/
		OHCI_WR(hub->registers->rh_port_status[port],
		    RHPS_CLEAR_PORT_SUSPEND);
		return EOK;

	case USB_HUB_FEATURE_C_PORT_CONNECTION:   /*16*/
	case USB_HUB_FEATURE_C_PORT_ENABLE:       /*17*/
	case USB_HUB_FEATURE_C_PORT_SUSPEND:      /*18*/
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT: /*19*/
	case USB_HUB_FEATURE_C_PORT_RESET:        /*20*/
		usb_log_debug2("Clearing port C_CONNECTION, C_ENABLE, "
		    "C_SUSPEND, C_OC or C_RESET on port %u.\n", port);
		/* Bit offsets correspond to the feature number */
		OHCI_WR(hub->registers->rh_port_status[port],
		    1 << feature);
		return EOK;

	default:
		return ENOTSUP;
	}
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
	ohci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	
	switch (feature) {
	case USB_HUB_FEATURE_PORT_POWER:   /*8*/
		{
			const uint32_t rhda = OHCI_RD(hub->registers->rh_desc_a);
			
			/* No power switching */
			if (rhda & RHDA_NPS_FLAG)
				return EOK;
			
			/* Ganged power switching, one port powers all */
			if (!(rhda & RHDA_PSM_FLAG)) {
				OHCI_WR(hub->registers->rh_status,RHS_SET_GLOBAL_POWER);
				return EOK;
			}
		}
		/* Fall through, for per port power */
		/* Fallthrough */
	case USB_HUB_FEATURE_PORT_ENABLE:  /*1*/
	case USB_HUB_FEATURE_PORT_SUSPEND: /*2*/
	case USB_HUB_FEATURE_PORT_RESET:   /*4*/
		usb_log_debug2("Setting port POWER, ENABLE, SUSPEND or RESET "
		    "on port %u.\n", port);
		/* Bit offsets correspond to the feature number */
		OHCI_WR(hub->registers->rh_port_status[port], 1 << feature);
		return EOK;
	default:
		return ENOTSUP;
	}
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
	ohci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (buffer_size < STATUS_BYTES(hub->port_count))
		return ESTALL;

	uint16_t mask = 0;

	/* Only local power source change and over-current change can happen */
	if (OHCI_RD(hub->registers->rh_status) & (RHS_LPSC_FLAG | RHS_OCIC_FLAG)) {
		mask |= 1;
	}

	for (unsigned port = 1; port <= hub->port_count; ++port) {
		/* Write-clean bits are those that indicate change */
		if (OHCI_RD(hub->registers->rh_port_status[port - 1])
		    & RHPS_CHANGE_WC_MASK) {
			mask |= (1 << port);
		}
	}

	usb_log_debug2("OHCI root hub interrupt mask: %hx.\n", mask);

	if (mask == 0)
		return ENAK;
	mask = uint16_host2usb(mask);
	memcpy(buffer, &mask, STATUS_BYTES(hub->port_count));
	*actual_size = STATUS_BYTES(hub->port_count);
	return EOK;
}

/** OHCI root hub request handlers */
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
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		.callback = req_clear_hub_feature,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		.callback = req_get_status,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		.callback = req_nop,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature,
	},
	{
		.callback = NULL
	}
};

/** Virtual OHCI root hub ops */
static usbvirt_device_ops_t ops = {
        .control = control_transfer_handlers,
        .data_in[HUB_STATUS_CHANGE_PIPE] = req_status_change_handler,
};
