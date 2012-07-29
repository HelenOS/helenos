/*
 * Copyright (c) 2011 Jan Vesely
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
#include <byteorder.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/dev/request.h>
#include <usb/classes/hub.h>

#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usb/dev/driver.h>
#include "ohci_regs.h"
#include "root_hub.h"

/**
 * standart device descriptor for ohci root hub
 */
static const usb_standard_device_descriptor_t ohci_rh_device_descriptor = {
	.configuration_count = 1,
	.descriptor_type = USB_DESCTYPE_DEVICE,
	.device_class = USB_CLASS_HUB,
	.device_protocol = 0,
	.device_subclass = 0,
	.device_version = 0,
	.length = sizeof(usb_standard_device_descriptor_t),
	.max_packet_size = 64,
	.vendor_id = 0x16db, /* HelenOS does not have USB vendor ID assigned.*/
	.product_id = 0x0001,
	.str_serial_number = 0,
	.usb_spec_version = 0x110,
};

/**
 * standart configuration descriptor with filled common values
 * for ohci root hubs
 */
static const usb_standard_configuration_descriptor_t ohci_rh_conf_descriptor = {
	.attributes = 1 << 7,
	.configuration_number = 1,
	.descriptor_type = USB_DESCTYPE_CONFIGURATION,
	.interface_count = 1,
	.length = sizeof(usb_standard_configuration_descriptor_t),
	.max_power = 0, /* root hubs don't need no power */
	.str_configuration = 0,
};

/**
 * standart ohci root hub interface descriptor
 */
static const usb_standard_interface_descriptor_t ohci_rh_iface_descriptor = {
	.alternate_setting = 0,
	.descriptor_type = USB_DESCTYPE_INTERFACE,
	.endpoint_count = 1,
	.interface_class = USB_CLASS_HUB,
	.interface_number = 1,
	.interface_protocol = 0,
	.interface_subclass = 0,
	.length = sizeof(usb_standard_interface_descriptor_t),
	.str_interface = 0,
};

/**
 * standart ohci root hub endpoint descriptor
 */
static const usb_standard_endpoint_descriptor_t ohci_rh_ep_descriptor = {
	.attributes = USB_TRANSFER_INTERRUPT,
	.descriptor_type = USB_DESCTYPE_ENDPOINT,
	.endpoint_address = 1 | (1 << 7),
	.length = sizeof(usb_standard_endpoint_descriptor_t),
	.max_packet_size = 2,
	.poll_interval = 255,
};

static void create_serialized_hub_descriptor(rh_t *instance);
static void rh_init_descriptors(rh_t *instance);
static uint16_t create_interrupt_mask(const rh_t *instance);
static void get_status(const rh_t *instance, usb_transfer_batch_t *request);
static void get_descriptor(const rh_t *instance, usb_transfer_batch_t *request);
static void set_feature(const rh_t *instance, usb_transfer_batch_t *request);
static void clear_feature(const rh_t *instance, usb_transfer_batch_t *request);
static int set_feature_port(
    const rh_t *instance, uint16_t feature, uint16_t port);
static int clear_feature_port(
    const rh_t *instance, uint16_t feature, uint16_t port);
static void control_request(rh_t *instance, usb_transfer_batch_t *request);
static inline void interrupt_request(
    usb_transfer_batch_t *request, uint16_t mask, size_t size)
{
	assert(request);
	usb_log_debug("Sending interrupt vector(%zu) %hhx:%hhx.\n",
	    size, ((uint8_t*)&mask)[0], ((uint8_t*)&mask)[1]);
	usb_transfer_batch_finish_error(request, &mask, size, EOK);
	usb_transfer_batch_destroy(request);
}

#define TRANSFER_END_DATA(request, data, bytes) \
do { \
	usb_transfer_batch_finish_error(request, data, bytes, EOK); \
	usb_transfer_batch_destroy(request); \
	return; \
} while (0)

#define TRANSFER_END(request, error) \
do { \
	usb_transfer_batch_finish_error(request, NULL, 0, error); \
	usb_transfer_batch_destroy(request); \
	return; \
} while (0)

/** Root Hub driver structure initialization.
 *
 * Reads info registers and prepares descriptors. Sets power mode.
 */
void rh_init(rh_t *instance, ohci_regs_t *regs)
{
	assert(instance);
	assert(regs);

	instance->registers = regs;
	instance->port_count = OHCI_RD(regs->rh_desc_a) & RHDA_NDS_MASK;
	usb_log_debug2("rh_desc_a: %x.\n", OHCI_RD(regs->rh_desc_a));
	if (instance->port_count > 15) {
		usb_log_warning("OHCI specification does not allow more than 15"
		    " ports. Max 15 ports will be used");
		instance->port_count = 15;
	}

	/* Don't forget the hub status bit and round up */
	instance->interrupt_mask_size = 1 + (instance->port_count / 8);
	instance->unfinished_interrupt_transfer = NULL;

#if defined OHCI_POWER_SWITCH_no
	usb_log_debug("OHCI rh: Set power mode to no power switching.\n");
	/* Set port power mode to no power-switching. (always on) */
	OHCI_SET(regs->rh_desc_a, RHDA_NPS_FLAG);

	/* Set to no over-current reporting */
	OHCI_SET(regs->rh_desc_a, RHDA_NOCP_FLAG);

#elif defined OHCI_POWER_SWITCH_ganged
	usb_log_debug("OHCI rh: Set power mode to ganged power switching.\n");
	/* Set port power mode to ganged power-switching. */
	OHCI_CLR(regs->rh_desc_a, RHDA_NPS_FLAG);
	OHCI_CLR(regs->rh_desc_a, RHDA_PSM_FLAG);

	/* Turn off power (hub driver will turn this back on)*/
	OHCI_WR(regs->rh_status, RHS_CLEAR_GLOBAL_POWER);

	/* Set to global over-current */
	OHCI_CLR(regs->rh_desc_a, RHDA_NOCP_FLAG);
	OHCI_CLR(regs->rh_desc_a, RHDA_OCPM_FLAG);
#else
	usb_log_debug("OHCI rh: Set power mode to per-port power switching.\n");
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

	fibril_mutex_initialize(&instance->guard);
	rh_init_descriptors(instance);

	usb_log_info("Root hub (%zu ports) initialized.\n",
	    instance->port_count);
}

/**
 * Process root hub request.
 *
 * @param instance Root hub instance
 * @param request Structure containing both request and response information
 * @return Error code
 */
void rh_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	switch (request->ep->transfer_type)
	{
	case USB_TRANSFER_CONTROL:
		usb_log_debug("Root hub got CONTROL packet\n");
		control_request(instance, request);
		break;

	case USB_TRANSFER_INTERRUPT:
		usb_log_debug("Root hub got INTERRUPT packet\n");
		fibril_mutex_lock(&instance->guard);
		assert(instance->unfinished_interrupt_transfer == NULL);
		const uint16_t mask = create_interrupt_mask(instance);
		if (mask == 0) {
			usb_log_debug("No changes(%hx)...\n", mask);
			instance->unfinished_interrupt_transfer = request;
		} else {
			usb_log_debug("Processing changes...\n");
			interrupt_request(
			    request, mask, instance->interrupt_mask_size);
		}
		fibril_mutex_unlock(&instance->guard);
		break;

	default:
		usb_log_error("Root hub got unsupported request.\n");
		TRANSFER_END(request, ENOTSUP);
	}
}

/**
 * Process interrupt on a hub device.
 *
 * If there is no pending interrupt transfer, nothing happens.
 * @param instance
 */
void rh_interrupt(rh_t *instance)
{
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	if (instance->unfinished_interrupt_transfer) {
		usb_log_debug("Finalizing interrupt transfer\n");
		const uint16_t mask = create_interrupt_mask(instance);
		interrupt_request(instance->unfinished_interrupt_transfer,
		    mask, instance->interrupt_mask_size);
		instance->unfinished_interrupt_transfer = NULL;
	}
	fibril_mutex_unlock(&instance->guard);
}

/**
 * Create hub descriptor.
 *
 * For descriptor format see USB hub specification (chapter 11.15.2.1, pg. 263)
 *
 * @param instance Root hub instance
 * @return Error code
 */
void create_serialized_hub_descriptor(rh_t *instance)
{
	assert(instance);

	/* 7 bytes + 2 port bit fields (port count + global bit) */
	size_t size = 7 + (instance->interrupt_mask_size * 2);
	assert(size <= HUB_DESCRIPTOR_MAX_SIZE);
	instance->hub_descriptor_size = size;

	const uint32_t hub_desc = OHCI_RD(instance->registers->rh_desc_a);
	const uint32_t port_desc = OHCI_RD(instance->registers->rh_desc_b);

	/* bDescLength */
	instance->descriptors.hub[0] = size;
	/* bDescriptorType */
	instance->descriptors.hub[1] = USB_DESCTYPE_HUB;
	/* bNmbrPorts */
	instance->descriptors.hub[2] = instance->port_count;
	/* wHubCharacteristics */
	instance->descriptors.hub[3] = 0 |
	    /* The lowest 2 bits indicate power switching mode */
	    (((hub_desc & RHDA_PSM_FLAG)  ? 1 : 0) << 0) |
	    (((hub_desc & RHDA_NPS_FLAG)  ? 1 : 0) << 1) |
	    /* Bit 3 indicates device type (compound device) */
	    (((hub_desc & RHDA_DT_FLAG)   ? 1 : 0) << 2) |
	    /* Bits 4,5 indicate over-current protection mode */
	    (((hub_desc & RHDA_OCPM_FLAG) ? 1 : 0) << 3) |
	    (((hub_desc & RHDA_NOCP_FLAG) ? 1 : 0) << 4);

	/* Reserved */
	instance->descriptors.hub[4] = 0;
	/* bPwrOn2PwrGood */
	instance->descriptors.hub[5] = hub_desc >> RHDA_POTPGT_SHIFT;
	/* bHubContrCurrent, root hubs don't need no power. */
	instance->descriptors.hub[6] = 0;

	/* Device Removable and some legacy 1.0 stuff*/
	instance->descriptors.hub[7] = (port_desc >> RHDB_DR_SHIFT) & 0xff;
	instance->descriptors.hub[8] = 0xff;
	if (instance->interrupt_mask_size == 2) {
		instance->descriptors.hub[8] =
		    (port_desc >> RHDB_DR_SHIFT) >> 8;
		instance->descriptors.hub[9]  = 0xff;
		instance->descriptors.hub[10] = 0xff;
	}
}

/** Initialize hub descriptors.
 *
 * A full configuration descriptor is assembled. The configuration and endpoint
 * descriptors have local modifications.
 * @param instance Root hub instance
 * @return Error code
 */
void rh_init_descriptors(rh_t *instance)
{
	assert(instance);

	instance->descriptors.configuration = ohci_rh_conf_descriptor;
	instance->descriptors.interface = ohci_rh_iface_descriptor;
	instance->descriptors.endpoint = ohci_rh_ep_descriptor;
	create_serialized_hub_descriptor(instance);

	instance->descriptors.endpoint.max_packet_size =
	    instance->interrupt_mask_size;

	instance->descriptors.configuration.total_length = uint16_host2usb(
	    sizeof(usb_standard_configuration_descriptor_t) +
	    sizeof(usb_standard_endpoint_descriptor_t) +
	    sizeof(usb_standard_interface_descriptor_t) +
	    instance->hub_descriptor_size);
}

/**
 * Create bitmap of changes to answer status interrupt.
 *
 * Result contains bitmap where bit 0 indicates change on hub and
 * bit i indicates change on i`th port (i>0). For more info see
 * Hub and Port status bitmap specification in USB specification
 * (chapter 11.13.4).
 * @param instance root hub instance
 * @return Mask of changes.
 */
uint16_t create_interrupt_mask(const rh_t *instance)
{
	assert(instance);
	uint16_t mask = 0;

	/* Only local power source change and over-current change can happen */
	if (OHCI_RD(instance->registers->rh_status)
	    & (RHS_LPSC_FLAG | RHS_OCIC_FLAG)) {
		mask |= 1;
	}
	for (size_t port = 1; port <= instance->port_count; ++port) {
		/* Write-clean bits are those that indicate change */
		if (OHCI_RD(instance->registers->rh_port_status[port - 1])
		    & RHPS_CHANGE_WC_MASK) {
			mask |= (1 << port);
		}
	}
	usb_log_debug2("OHCI root hub interrupt mask: %hx.\n", mask);
	return uint16_host2usb(mask);
}

/**
 * Create answer to status request.
 *
 * This might be either hub status or port status request. If neither,
 * ENOTSUP is returned.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
void get_status(const rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);


	usb_device_request_setup_packet_t *request_packet =
	    (usb_device_request_setup_packet_t*)request->setup_buffer;

	const uint16_t index = uint16_usb2host(request_packet->index);

	switch (request_packet->request_type)
	{
	case USB_HUB_REQ_TYPE_GET_HUB_STATUS:
	/* Hub status: just filter relevant info from rh_status reg */
		if (request->buffer_size < 4) {
			usb_log_error("Buffer(%zu) too small for hub get "
			    "status request.\n", request->buffer_size);
			TRANSFER_END(request, EOVERFLOW);
		} else {
			const uint32_t data =
			    OHCI_RD(instance->registers->rh_status) &
			        (RHS_LPS_FLAG | RHS_LPSC_FLAG
			            | RHS_OCI_FLAG | RHS_OCIC_FLAG);
			TRANSFER_END_DATA(request, &data, sizeof(data));
		}

	/* Copy appropriate rh_port_status register, OHCI designers were
	 * kind enough to make those bit values match USB specification */
	case USB_HUB_REQ_TYPE_GET_PORT_STATUS:
		if (request->buffer_size < 4) {
			usb_log_error("Buffer(%zu) too small for hub get "
			    "status request.\n", request->buffer_size);
			TRANSFER_END(request, EOVERFLOW);
		} else {
			const unsigned port = index;
			if (port < 1 || port > instance->port_count)
				TRANSFER_END(request, EINVAL);
			/* Register format matches the format of port status
			 * field */
			const uint32_t data = uint32_host2usb(OHCI_RD(
			    instance->registers->rh_port_status[port - 1]));
			TRANSFER_END_DATA(request, &data, sizeof(data));
		}
	case SETUP_REQUEST_TO_HOST(USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE):
		if (request->buffer_size < 2) {
			usb_log_error("Buffer(%zu) too small for hub generic "
			    "get status request.\n", request->buffer_size);
			TRANSFER_END(request, EOVERFLOW);
		} else {
			const uint16_t data =
			    uint16_host2usb(USB_DEVICE_STATUS_SELF_POWERED);
			TRANSFER_END_DATA(request, &data, sizeof(data));
		}

	case SETUP_REQUEST_TO_HOST(USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE):
		/* Hubs are allowed to have only one interface */
		if (index != 0)
			TRANSFER_END(request, EINVAL);
		/* Fall through, as the answer will be the same: 0x0000 */
	case SETUP_REQUEST_TO_HOST(USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_ENDPOINT):
		/* Endpoint 0 (default control) and 1 (interrupt) */
		if (index >= 2)
			TRANSFER_END(request, EINVAL);

		if (request->buffer_size < 2) {
			usb_log_error("Buffer(%zu) too small for hub generic "
			    "get status request.\n", request->buffer_size);
			TRANSFER_END(request, EOVERFLOW);
		} else {
			/* Endpoints are OK. (We don't halt) */
			const uint16_t data = 0;
			TRANSFER_END_DATA(request, &data, sizeof(data));
		}

	default:
		usb_log_error("Unsupported GET_STATUS request.\n");
		TRANSFER_END(request, ENOTSUP);
	}

}

/**
 * Create answer to a descriptor request.
 *
 * This might be a request for standard (configuration, device, endpoint or
 * interface) or device specific (hub) descriptor.
 * @param instance Root hub instance
 * @param request Structure containing both request and response information
 * @return Error code
 */
void get_descriptor(const rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	/* "The wValue field specifies the descriptor type in the high byte
	 * and the descriptor index in the low byte (refer to Table 9-5)." */
	const int desc_type = uint16_usb2host(setup_request->value) >> 8;
	switch (desc_type)
	{
	case USB_DESCTYPE_HUB:
		usb_log_debug2("USB_DESCTYPE_HUB\n");
		/* Hub descriptor was generated locally.
		 * Class specific request. */
		TRANSFER_END_DATA(request, instance->descriptors.hub,
		    instance->hub_descriptor_size);

	case USB_DESCTYPE_DEVICE:
		usb_log_debug2("USB_DESCTYPE_DEVICE\n");
		/* Device descriptor is shared
		 * (No one should ask for it, as the device is already setup)
		 * Standard USB device request. */
		TRANSFER_END_DATA(request, &ohci_rh_device_descriptor,
		    sizeof(ohci_rh_device_descriptor));

	case USB_DESCTYPE_CONFIGURATION:
		usb_log_debug2("USB_DESCTYPE_CONFIGURATION\n");
		/* Start with configuration and add others depending on
		 * request size. Standard USB request. */
		TRANSFER_END_DATA(request, &instance->descriptors,
		    instance->descriptors.configuration.total_length);

	case USB_DESCTYPE_INTERFACE:
		usb_log_debug2("USB_DESCTYPE_INTERFACE\n");
		/* Use local interface descriptor. There is one and it
		 * might be modified. Hub driver should not ask or this
		 * descriptor as it is not part of standard requests set. */
		TRANSFER_END_DATA(request, &instance->descriptors.interface,
		    sizeof(instance->descriptors.interface));

	case USB_DESCTYPE_ENDPOINT:
		/* Use local endpoint descriptor. There is one
		 * it might have max_packet_size field modified. Hub driver
		 * should not ask for this descriptor as it is not part
		 * of standard requests set. */
		usb_log_debug2("USB_DESCTYPE_ENDPOINT\n");
		TRANSFER_END_DATA(request, &instance->descriptors.endpoint,
		    sizeof(instance->descriptors.endpoint));

	default:
		usb_log_debug2("USB_DESCTYPE_EINVAL %d \n"
		    "\ttype %d\n\trequest %d\n\tvalue "
		    "%d\n\tindex %d\n\tlen %d\n ",
		    setup_request->value,
		    setup_request->request_type, setup_request->request,
		    desc_type, setup_request->index,
		    setup_request->length);
		TRANSFER_END(request, EINVAL);
	}

	TRANSFER_END(request, ENOTSUP);
}

/**
 * process feature-enabling request on hub
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @param port port number, counted from 1
 * @param enable enable or disable the specified feature
 * @return error code
 */
int set_feature_port(const rh_t *instance, uint16_t feature, uint16_t port)
{
	assert(instance);

	if (port < 1 || port > instance->port_count)
		return EINVAL;

	switch (feature) {
	case USB_HUB_FEATURE_PORT_POWER:   /*8*/
		{
			const uint32_t rhda =
			    OHCI_RD(instance->registers->rh_desc_a);
			/* No power switching */
			if (rhda & RHDA_NPS_FLAG)
				return EOK;
			/* Ganged power switching, one port powers all */
			if (!(rhda & RHDA_PSM_FLAG)) {
				OHCI_WR(instance->registers->rh_status,
				    RHS_SET_GLOBAL_POWER);
				return EOK;
			}
		}
			/* Fall through */
	case USB_HUB_FEATURE_PORT_ENABLE:  /*1*/
	case USB_HUB_FEATURE_PORT_SUSPEND: /*2*/
	case USB_HUB_FEATURE_PORT_RESET:   /*4*/
		usb_log_debug2("Setting port POWER, ENABLE, SUSPEND or RESET "
		    "on port %"PRIu16".\n", port);
		OHCI_WR(instance->registers->rh_port_status[port - 1],
		    1 << feature);
		return EOK;
	default:
		return ENOTSUP;
	}
}

/**
 * Process feature clear request.
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @param port port number, counted from 1
 * @param enable enable or disable the specified feature
 * @return error code
 */
int clear_feature_port(const rh_t *instance, uint16_t feature, uint16_t port)
{
	assert(instance);

	if (port < 1 || port > instance->port_count)
		return EINVAL;

	/* Enabled features to clear: see page 269 of USB specs */
	switch (feature)
	{
	case USB_HUB_FEATURE_PORT_POWER:          /*8*/
		{
			const uint32_t rhda =
			    OHCI_RD(instance->registers->rh_desc_a);
			/* No power switching */
			if (rhda & RHDA_NPS_FLAG)
				return ENOTSUP;
			/* Ganged power switching, one port powers all */
			if (!(rhda & RHDA_PSM_FLAG)) {
				OHCI_WR(instance->registers->rh_status,
				    RHS_CLEAR_GLOBAL_POWER);
				return EOK;
			}
			OHCI_WR(instance->registers->rh_port_status[port - 1],
			    RHPS_CLEAR_PORT_POWER);
			return EOK;
		}

	case USB_HUB_FEATURE_PORT_ENABLE:         /*1*/
		OHCI_WR(instance->registers->rh_port_status[port - 1],
		    RHPS_CLEAR_PORT_ENABLE);
		return EOK;

	case USB_HUB_FEATURE_PORT_SUSPEND:        /*2*/
		OHCI_WR(instance->registers->rh_port_status[port - 1],
		    RHPS_CLEAR_PORT_SUSPEND);
		return EOK;

	case USB_HUB_FEATURE_C_PORT_CONNECTION:   /*16*/
	case USB_HUB_FEATURE_C_PORT_ENABLE:       /*17*/
	case USB_HUB_FEATURE_C_PORT_SUSPEND:      /*18*/
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT: /*19*/
	case USB_HUB_FEATURE_C_PORT_RESET:        /*20*/
		usb_log_debug2("Clearing port C_CONNECTION, C_ENABLE, "
		    "C_SUSPEND, C_OC or C_RESET on port %"PRIu16".\n", port);
		/* Bit offsets correspond to the feature number */
		OHCI_WR(instance->registers->rh_port_status[port - 1],
		    1 << feature);
		return EOK;

	default:
		return ENOTSUP;
	}
}

/**
 * process one of requests that do not request nor carry additional data
 *
 * Request can be one of USB_DEVREQ_CLEAR_FEATURE, USB_DEVREQ_SET_FEATURE or
 * USB_DEVREQ_SET_ADDRESS.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
void set_feature(const rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	switch (setup_request->request_type)
	{
	case USB_HUB_REQ_TYPE_SET_PORT_FEATURE:
		usb_log_debug("USB_HUB_REQ_TYPE_SET_PORT_FEATURE\n");
		const int ret = set_feature_port(instance,
		    setup_request->value, setup_request->index);
		TRANSFER_END(request, ret);

	case USB_HUB_REQ_TYPE_SET_HUB_FEATURE:
		/* Chapter 11.16.2 specifies that hub can be recipient
		 * only for C_HUB_LOCAL_POWER and C_HUB_OVER_CURRENT
		 * features. It makes no sense to SET either. */
		usb_log_error("Invalid HUB set feature request.\n");
		TRANSFER_END(request, ENOTSUP);
	//TODO: Consider standard USB requests: REMOTE WAKEUP, ENDPOINT STALL
	default:
		usb_log_error("Invalid set feature request type: %d\n",
		    setup_request->request_type);
		TRANSFER_END(request, ENOTSUP);
	}
}

/**
 * process one of requests that do not request nor carry additional data
 *
 * Request can be one of USB_DEVREQ_CLEAR_FEATURE, USB_DEVREQ_SET_FEATURE or
 * USB_DEVREQ_SET_ADDRESS.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
void clear_feature(const rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;

	switch (setup_request->request_type)
	{
	case USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE:
		usb_log_debug("USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE\n");
		const int ret = clear_feature_port(instance,
		    setup_request->value, setup_request->index);
		TRANSFER_END(request, ret);

	case USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE:
		usb_log_debug("USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE\n");
		/*
		 * Chapter 11.16.2 specifies that only C_HUB_LOCAL_POWER and
		 * C_HUB_OVER_CURRENT are supported.
		 * C_HUB_OVER_CURRENT is represented by OHCI RHS_OCIC_FLAG.
		 * C_HUB_LOCAL_POWER is not supported
		 * as root hubs do not support local power status feature.
		 * (OHCI pg. 127) */
		if (uint16_usb2host(setup_request->value)
		    == USB_HUB_FEATURE_C_HUB_OVER_CURRENT) {
			OHCI_WR(instance->registers->rh_status, RHS_OCIC_FLAG);
			TRANSFER_END(request, EOK);
		}
	//TODO: Consider standard USB requests: REMOTE WAKEUP, ENDPOINT STALL
	default:
		usb_log_error("Invalid clear feature request type: %d\n",
		    setup_request->request_type);
		TRANSFER_END(request, ENOTSUP);
	}
}

/**
 * Process hub control request.
 *
 * If needed, writes answer into the request structure.
 * Request can be one of
 * USB_DEVREQ_GET_STATUS,
 * USB_DEVREQ_GET_DESCRIPTOR,
 * USB_DEVREQ_GET_CONFIGURATION,
 * USB_DEVREQ_CLEAR_FEATURE,
 * USB_DEVREQ_SET_FEATURE,
 * USB_DEVREQ_SET_ADDRESS,
 * USB_DEVREQ_SET_DESCRIPTOR or
 * USB_DEVREQ_SET_CONFIGURATION.
 *
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
void control_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	if (!request->setup_buffer) {
		usb_log_error("Root hub received empty transaction!");
		TRANSFER_END(request, EBADMEM);
	}

	if (sizeof(usb_device_request_setup_packet_t) > request->setup_size) {
		usb_log_error("Setup packet too small\n");
		TRANSFER_END(request, EOVERFLOW);
	}

	usb_log_debug2("CTRL packet: %s.\n",
	    usb_debug_str_buffer((uint8_t *) request->setup_buffer, 8, 8));
	usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	switch (setup_request->request)
	{
	case USB_DEVREQ_GET_STATUS:
		usb_log_debug("USB_DEVREQ_GET_STATUS\n");
		get_status(instance, request);
		break;

	case USB_DEVREQ_GET_DESCRIPTOR:
		usb_log_debug("USB_DEVREQ_GET_DESCRIPTOR\n");
		get_descriptor(instance, request);
		break;

	case USB_DEVREQ_GET_CONFIGURATION:
		usb_log_debug("USB_DEVREQ_GET_CONFIGURATION\n");
		if (request->buffer_size == 0)
			TRANSFER_END(request, EOVERFLOW);
		const uint8_t config = 1;
		TRANSFER_END_DATA(request, &config, sizeof(config));

	case USB_DEVREQ_CLEAR_FEATURE:
		usb_log_debug2("USB_DEVREQ_CLEAR_FEATURE\n");
		clear_feature(instance, request);
		break;

	case USB_DEVREQ_SET_FEATURE:
		usb_log_debug2("USB_DEVREQ_SET_FEATURE\n");
		set_feature(instance, request);
		break;

	case USB_DEVREQ_SET_ADDRESS:
		usb_log_debug("USB_DEVREQ_SET_ADDRESS: %u\n",
		    setup_request->value);
		if (uint16_usb2host(setup_request->value) > 127)
			TRANSFER_END(request, EINVAL);

		instance->address = uint16_usb2host(setup_request->value);
		TRANSFER_END(request, EOK);

	case USB_DEVREQ_SET_CONFIGURATION:
		usb_log_debug("USB_DEVREQ_SET_CONFIGURATION: %u\n",
		    uint16_usb2host(setup_request->value));
		/* We have only one configuration, it's number is 1 */
		if (uint16_usb2host(setup_request->value) != 1)
			TRANSFER_END(request, EINVAL);
		TRANSFER_END(request, EOK);

	/* Both class specific and std is optional for hubs */
	case USB_DEVREQ_SET_DESCRIPTOR:
	/* Hubs have only one interface GET/SET is not supported */
	case USB_DEVREQ_GET_INTERFACE:
	case USB_DEVREQ_SET_INTERFACE:
	default:
		/* Hub class GET_STATE(2) falls in here too. */
		usb_log_error("Received unsupported request: %d.\n",
		    setup_request->request);
		TRANSFER_END(request, ENOTSUP);
	}
}
/**
 * @}
 */
