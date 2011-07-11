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
#include <errno.h>
#include <str_error.h>

#include <usb/debug.h>

#include "root_hub.h"
#include <usb/classes/classes.h>
#include <usb/dev/driver.h>
#include "ohci_regs.h"

#include <usb/dev/request.h>
#include <usb/classes/hub.h>

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
	.length = sizeof (usb_standard_device_descriptor_t),
	.max_packet_size = 8,
	.vendor_id = 0x16db,
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
	.length = sizeof (usb_standard_configuration_descriptor_t),
	.max_power = 100,
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
	.length = sizeof (usb_standard_interface_descriptor_t),
	.str_interface = 0,
};

/**
 * standart ohci root hub endpoint descriptor
 */
static const usb_standard_endpoint_descriptor_t ohci_rh_ep_descriptor = {
	.attributes = USB_TRANSFER_INTERRUPT,
	.descriptor_type = USB_DESCTYPE_ENDPOINT,
	.endpoint_address = 1 + (1 << 7),
	.length = sizeof (usb_standard_endpoint_descriptor_t),
	.max_packet_size = 8,
	.poll_interval = 255,
};

/**
 * bitmask of hub features that are valid to be cleared
 */
static const uint32_t hub_clear_feature_valid_mask =
    RHS_OCIC_FLAG |
    RHS_CLEAR_PORT_POWER;

/**
 * bitmask of hub features that are cleared by writing 1 (and not 0)
 */
static const uint32_t hub_clear_feature_by_writing_one_mask =
    RHS_CLEAR_PORT_POWER;

/**
 * bitmask of hub features that are valid to be set
 */
static const uint32_t hub_set_feature_valid_mask =
    RHS_LPSC_FLAG |
    RHS_OCIC_FLAG;

/**
 * bitmask of hub features that are set by writing 1 and cleared by writing 0
 */
static const uint32_t hub_set_feature_direct_mask =
    RHS_SET_PORT_POWER;

/**
 * bitmask of port features that are valid to be set
 */
static const uint32_t port_set_feature_valid_mask =
    RHPS_SET_PORT_ENABLE |
    RHPS_SET_PORT_SUSPEND |
    RHPS_SET_PORT_RESET |
    RHPS_SET_PORT_POWER;

/**
 * bitmask of port features that can be cleared
 */
static const uint32_t port_clear_feature_valid_mask =
    RHPS_CCS_FLAG |
    RHPS_SET_PORT_SUSPEND |
    RHPS_POCI_FLAG |
    RHPS_SET_PORT_POWER |
    RHPS_CSC_FLAG |
    RHPS_PESC_FLAG |
    RHPS_PSSC_FLAG |
    RHPS_OCIC_FLAG |
    RHPS_PRSC_FLAG;

//note that USB_HUB_FEATURE_PORT_POWER bit is translated into
//USB_HUB_FEATURE_PORT_LOW_SPEED for port set feature request

/**
 * bitmask with port status changes
 */
static const uint32_t port_status_change_mask = RHPS_CHANGE_WC_MASK;

static int create_serialized_hub_descriptor(rh_t *instance);

static int rh_init_descriptors(rh_t *instance);

static void create_interrupt_mask_in_instance(rh_t *instance);

static int process_get_port_status_request(
    rh_t *instance, uint16_t port, usb_transfer_batch_t *request);

static int process_get_hub_status_request(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_get_status_request(
    rh_t *instance, usb_transfer_batch_t *request);


static int process_get_descriptor_request(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_get_configuration_request(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_hub_feature_set_request(rh_t *instance, uint16_t feature);

static int process_hub_feature_clear_request(
    rh_t *instance, uint16_t feature);

static int process_port_feature_set_request(
    rh_t *instance, uint16_t feature, uint16_t port);

static int process_port_feature_clear_request(
    rh_t *instance, uint16_t feature, uint16_t port);

static int process_request_with_input(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_request_with_output(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_request_without_data(
    rh_t *instance, usb_transfer_batch_t *request);

static int process_ctrl_request(rh_t *instance, usb_transfer_batch_t *request);

static int process_interrupt_mask_in_instance(
    rh_t *instance, usb_transfer_batch_t *request);

static bool is_zeros(const void *buffer, size_t size);

/**
 * Register address to this device
 *
 * @param instance Root hub instance
 * @param address New address
 * @return Error code
 */
static inline int process_address_set_request(rh_t *instance, uint16_t address)
	{ return ENOTSUP; }

/** Root hub initialization
 * @return Error code.
 */
int rh_init(rh_t *instance, ohci_regs_t *regs) {
	assert(instance);
	instance->registers = regs;
	instance->port_count =
	    (instance->registers->rh_desc_a >> RHDA_NDS_SHIFT) & RHDA_NDS_MASK;
	int opResult = rh_init_descriptors(instance);
	if (opResult != EOK) {
		return opResult;
	}
	// set port power mode to no-power-switching
	instance->registers->rh_desc_a |= RHDA_NPS_FLAG;
	instance->unfinished_interrupt_transfer = NULL;
	instance->interrupt_mask_size = (instance->port_count + 8) / 8;
	instance->interrupt_buffer = malloc(instance->interrupt_mask_size);
	if (!instance->interrupt_buffer)
		return ENOMEM;

	usb_log_info("Root hub (%zu ports) initialized.\n",
	    instance->port_count);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Process root hub request.
 *
 * @param instance Root hub instance
 * @param request Structure containing both request and response information
 * @return Error code
 */
int rh_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	int opResult;
	switch (request->ep->transfer_type)
	{
	case USB_TRANSFER_CONTROL:
		usb_log_debug("Root hub got CONTROL packet\n");
		opResult = process_ctrl_request(instance, request);
		usb_transfer_batch_finish_error(request, opResult);
		break;
	case USB_TRANSFER_INTERRUPT:
		usb_log_debug("Root hub got INTERRUPT packet\n");
		create_interrupt_mask_in_instance(instance);
		if (is_zeros(instance->interrupt_buffer,
		    instance->interrupt_mask_size)) {
			usb_log_debug("No changes..\n");
			instance->unfinished_interrupt_transfer = request;
			//will be finished later
		} else {
			usb_log_debug("Processing changes..\n");
			process_interrupt_mask_in_instance(instance, request);
		}
		break;
	default:
		usb_log_error("Root hub got unsupported request.\n");
		usb_transfer_batch_finish_error(request, EINVAL);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process interrupt on a hub
 *
 * If there is no pending interrupt transfer, nothing happens.
 * @param instance
 */
void rh_interrupt(rh_t *instance)
{
	if (!instance->unfinished_interrupt_transfer)
		return;

	usb_log_debug("Finalizing interrupt transfer\n");
	create_interrupt_mask_in_instance(instance);
	process_interrupt_mask_in_instance(instance,
	    instance->unfinished_interrupt_transfer);
}
/*----------------------------------------------------------------------------*/
/**
 * Create hub descriptor used in hub-driver <-> hub communication
 *
 * This means creating bit array from data in root hub registers. For more
 * info see usb hub specification.
 *
 * @param instance Root hub instance
 * @return Error code
 */
int create_serialized_hub_descriptor(rh_t *instance)
{
	assert(instance);

	const size_t size = 7 +
	    ((instance->port_count + 7) / 8) * 2;
	uint8_t * result = malloc(size);
	if (!result)
	    return ENOMEM;

	bzero(result, size);
	//size
	result[0] = size;
	//descriptor type
	result[1] = USB_DESCTYPE_HUB;
	result[2] = instance->port_count;
	const uint32_t hub_desc_reg = instance->registers->rh_desc_a;
	result[3] =
	    ((hub_desc_reg >> 8) % 2) +
	    (((hub_desc_reg >> 9) % 2) << 1) +
	    (((hub_desc_reg >> 10) % 2) << 2) +
	    (((hub_desc_reg >> 11) % 2) << 3) +
	    (((hub_desc_reg >> 12) % 2) << 4);
	result[4] = 0;
	result[5] = 50; /*descriptor->pwr_on_2_good_time*/
	result[6] = 50;

	size_t port = 1;
	for (; port <= instance->port_count; ++port) {
		const uint8_t is_non_removable =
		    instance->registers->rh_desc_b >> port % 2;
		result[7 + port / 8] +=
		    is_non_removable << (port % 8);
	}
	const size_t var_size = (instance->port_count + 7) / 8;
	size_t i = 0;
	for (; i < var_size; ++i) {
		result[7 + var_size + i] = 255;
	}
	instance->hub_descriptor = result;
	instance->descriptor_size = size;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Initialize hub descriptors.
 *
 * Device and full configuration descriptor are created. These need to
 * be initialized only once per hub.
 * @param instance Root hub instance
 * @return Error code
 */
int rh_init_descriptors(rh_t *instance)
{
	assert(instance);

	memcpy(&instance->descriptors.device, &ohci_rh_device_descriptor,
	    sizeof (ohci_rh_device_descriptor)
	    );
	usb_standard_configuration_descriptor_t descriptor;
	memcpy(&descriptor, &ohci_rh_conf_descriptor,
	    sizeof (ohci_rh_conf_descriptor));

	int opResult = create_serialized_hub_descriptor(instance);
	if (opResult != EOK) {
		return opResult;
	}
	descriptor.total_length =
	    sizeof (usb_standard_configuration_descriptor_t) +
	    sizeof (usb_standard_endpoint_descriptor_t) +
	    sizeof (usb_standard_interface_descriptor_t) +
	    instance->descriptor_size;

	uint8_t * full_config_descriptor = malloc(descriptor.total_length);
	if (!full_config_descriptor) {
		return ENOMEM;
	}
	memcpy(full_config_descriptor, &descriptor, sizeof (descriptor));
	memcpy(full_config_descriptor + sizeof (descriptor),
	    &ohci_rh_iface_descriptor, sizeof (ohci_rh_iface_descriptor));
	memcpy(full_config_descriptor + sizeof (descriptor) +
	    sizeof (ohci_rh_iface_descriptor),
	    &ohci_rh_ep_descriptor, sizeof (ohci_rh_ep_descriptor));
	memcpy(full_config_descriptor + sizeof (descriptor) +
	    sizeof (ohci_rh_iface_descriptor) +
	    sizeof (ohci_rh_ep_descriptor),
	    instance->hub_descriptor, instance->descriptor_size);

	instance->descriptors.configuration = full_config_descriptor;
	instance->descriptors.configuration_size = descriptor.total_length;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Create answer to port status_request
 *
 * Copy content of corresponding port status register to answer buffer. The
 * format of the port status register and port status data is the same (
 * see OHCI root hub and USB hub documentation).
 *
 * @param instance Root hub instance
 * @param port Port number, counted from 1
 * @param request Structure containing both request and response information
 * @return Error code
 */
int process_get_port_status_request(
    rh_t *instance, uint16_t port, usb_transfer_batch_t * request)
{
	assert(instance);
	assert(request);

	if (port < 1 || port > instance->port_count)
		return EINVAL;

	const uint32_t data = instance->registers->rh_port_status[port - 1];
	memcpy(request->data_buffer, &data, 4);
	request->transfered_size = 4;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Create answer to port status_request.
 *
 * This copies flags in hub status register into the buffer. The format of the
 * status register and status message is the same, according to USB hub
 * specification and OHCI root hub specification.
 *
 * @param instance Root hub instance.
 * @param request Structure containing both request and response information.
 * @return Error code
 */
int process_get_hub_status_request(
    rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	/* bits, 0,1,16,17 -- TODO: What do they mean?? Why not 0x0303 */
	const uint32_t mask = 1 | (1 << 1) | (1 << 16) | (1 << 17);
	const uint32_t data = mask & instance->registers->rh_status;
	memcpy(request->data_buffer, &data, 4);
	request->transfered_size = 4;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Create answer to status request.
 *
 * This might be either hub status or port status request. If neither,
 * ENOTSUP is returned.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
int process_get_status_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	const usb_device_request_setup_packet_t *request_packet =
	    (usb_device_request_setup_packet_t*)request->setup_buffer;

	const usb_hub_bm_request_type_t request_type =
	    request_packet->request_type;

	if (request->buffer_size < 4) {
		usb_log_error("Buffer too small for get status request.\n");
		return EOVERFLOW;
	}

	if (request_type == USB_HUB_REQ_TYPE_GET_HUB_STATUS)
		return process_get_hub_status_request(instance, request);
	if (request_type == USB_HUB_REQ_TYPE_GET_PORT_STATUS)
		return process_get_port_status_request(instance,
		    request_packet->index, request);

	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
/**
 * Create bitmap of changes to answer status interrupt.
 *
 * Result contains bitmap where bit 0 indicates change on hub and
 * bit i indicates change on i`th port (i>0). For more info see
 * Hub and Port status bitmap specification in USB specification
 * (chapter 11.13.4).
 * Uses instance`s interrupt buffer to store the interrupt information.
 * @param instance root hub instance
 */
void create_interrupt_mask_in_instance(rh_t *instance)
{
	assert(instance);

	uint8_t * bitmap = (uint8_t*) (instance->interrupt_buffer);
	uint32_t mask = (1 << (USB_HUB_FEATURE_C_HUB_LOCAL_POWER + 16))
	    | (1 << (USB_HUB_FEATURE_C_HUB_OVER_CURRENT + 16));
	bzero(bitmap, instance->interrupt_mask_size);
	if ((instance->registers->rh_status & mask) != 0) {
		bitmap[0] = 1;
	}
	mask = port_status_change_mask;
	size_t port = 1;
	for (; port <= instance->port_count; ++port) {
		if ((mask & instance->registers->rh_port_status[port - 1]) != 0) {

			bitmap[(port) / 8] += 1 << (port % 8);
		}
	}
}
/*----------------------------------------------------------------------------*/
/**
 * Create answer to a descriptor request.
 *
 * This might be a request for standard (configuration, device, endpoint or
 * interface) or device specific (hub) descriptor.
 * @param instance Root hub instance
 * @param request Structure containing both request and response information
 * @return Error code
 */
int process_get_descriptor_request(
    rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	const usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	size_t size;
	const void * result_descriptor = NULL;
	const uint16_t setup_request_value = setup_request->value_high;
	//(setup_request->value_low << 8);
	switch (setup_request_value)
	{
	case USB_DESCTYPE_HUB:
		usb_log_debug2("USB_DESCTYPE_HUB\n");
		result_descriptor = instance->hub_descriptor;
		size = instance->descriptor_size;
		break;

	case USB_DESCTYPE_DEVICE:
		usb_log_debug2("USB_DESCTYPE_DEVICE\n");
		result_descriptor = &ohci_rh_device_descriptor;
		size = sizeof(ohci_rh_device_descriptor);
		break;

	case USB_DESCTYPE_CONFIGURATION:
		usb_log_debug2("USB_DESCTYPE_CONFIGURATION\n");
		result_descriptor = instance->descriptors.configuration;
		size = instance->descriptors.configuration_size;
		break;

	case USB_DESCTYPE_INTERFACE:
		usb_log_debug2("USB_DESCTYPE_INTERFACE\n");
		result_descriptor = &ohci_rh_iface_descriptor;
		size = sizeof(ohci_rh_iface_descriptor);
		break;

	case USB_DESCTYPE_ENDPOINT:
		usb_log_debug2("USB_DESCTYPE_ENDPOINT\n");
		result_descriptor = &ohci_rh_ep_descriptor;
		size = sizeof(ohci_rh_ep_descriptor);
		break;

	default:
		usb_log_debug2("USB_DESCTYPE_EINVAL %d \n"
		    "\ttype %d\n\trequest %d\n\tvalue "
		    "%d\n\tindex %d\n\tlen %d\n ",
		    setup_request->value,
		    setup_request->request_type, setup_request->request,
		    setup_request_value, setup_request->index,
		    setup_request->length);
		return EINVAL;
	}
	if (request->buffer_size < size) {
		size = request->buffer_size;
	}
	memcpy(request->data_buffer, result_descriptor, size);
	request->transfered_size = size;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Answer to get configuration request.
 *
 * Root hub works independently on the configuration.
 * Set and get configuration requests do not have any meaning,
 * dummy values are returned.
 *
 * @param instance Root hub instance
 * @param request Structure containing both request and response information
 * @return Error code
 */
int process_get_configuration_request(
    rh_t *instance, usb_transfer_batch_t *request)
{
	assert(request);

	if (request->buffer_size != 1)
		return EINVAL;
	request->data_buffer[0] = 1;
	request->transfered_size = 1;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process feature-enabling request on hub
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @return error code
 */
static int process_hub_feature_set_request(rh_t *instance,
    uint16_t feature) {
	if (!((1 << feature) & hub_set_feature_valid_mask))
		return EINVAL;
	if (feature == USB_HUB_FEATURE_C_HUB_LOCAL_POWER)
		feature = USB_HUB_FEATURE_C_HUB_LOCAL_POWER << 16;
	instance->registers->rh_status =
	    (instance->registers->rh_status | (1 << feature))
	    & (~hub_clear_feature_by_writing_one_mask);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process feature-disabling request on hub
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @return error code
 */
int process_hub_feature_clear_request(rh_t *instance, uint16_t feature)
{
	assert(instance);

	if (!((1 << feature) & hub_clear_feature_valid_mask))
		return EINVAL;

	//is the feature cleared directly?
	if ((1 << feature) & hub_set_feature_direct_mask) {
		instance->registers->rh_status =
		    (instance->registers->rh_status & (~(1 << feature)))
		    & (~hub_clear_feature_by_writing_one_mask);
	} else {//the feature is cleared by writing '1'

		instance->registers->rh_status =
		    (instance->registers->rh_status
		    & (~hub_clear_feature_by_writing_one_mask))
		    | (1 << feature);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process feature-enabling request on hub
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @param port port number, counted from 1
 * @param enable enable or disable the specified feature
 * @return error code
 */
int process_port_feature_set_request(
    rh_t *instance, uint16_t feature, uint16_t port)
{
	assert(instance);

	if (!((1 << feature) & port_set_feature_valid_mask))
		return EINVAL;
	if (port < 1 || port > instance->port_count)
		return EINVAL;

	instance->registers->rh_port_status[port - 1] =
	    (instance->registers->rh_port_status[port - 1] | (1 << feature))
	    & (~port_clear_feature_valid_mask);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process feature-disabling request on hub
 *
 * @param instance root hub instance
 * @param feature feature selector
 * @param port port number, counted from 1
 * @param enable enable or disable the specified feature
 * @return error code
 */
int process_port_feature_clear_request(
    rh_t *instance, uint16_t feature, uint16_t port)
{
	assert(instance);

	if (!((1 << feature) & port_clear_feature_valid_mask))
		return EINVAL;
	if (port < 1 || port > instance->port_count)
		return EINVAL;

	/* Some weird stuff... */
	if (feature == USB_HUB_FEATURE_PORT_POWER)
		feature = USB_HUB_FEATURE_PORT_LOW_SPEED;
	if (feature == USB_HUB_FEATURE_PORT_SUSPEND)
		feature = USB_HUB_FEATURE_PORT_OVER_CURRENT;

	instance->registers->rh_port_status[port - 1] =
	    (instance->registers->rh_port_status[port - 1]
	    & (~port_clear_feature_valid_mask))
	    | (1 << feature);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * process one of requests that requere output data
 *
 * Request can be one of USB_DEVREQ_GET_STATUS, USB_DEVREQ_GET_DESCRIPTOR or
 * USB_DEVREQ_GET_CONFIGURATION.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
int process_request_with_output(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	const usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	switch (setup_request->request)
	{
	case USB_DEVREQ_GET_STATUS:
		usb_log_debug("USB_DEVREQ_GET_STATUS\n");
		return process_get_status_request(instance, request);
	case USB_DEVREQ_GET_DESCRIPTOR:
		usb_log_debug("USB_DEVREQ_GET_DESCRIPTOR\n");
		return process_get_descriptor_request(instance, request);
	case USB_DEVREQ_GET_CONFIGURATION:
		usb_log_debug("USB_DEVREQ_GET_CONFIGURATION\n");
		return process_get_configuration_request(instance, request);
	}
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
/**
 * process one of requests that carry input data
 *
 * Request can be one of USB_DEVREQ_SET_DESCRIPTOR or
 * USB_DEVREQ_SET_CONFIGURATION.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
int process_request_with_input(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	const usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	request->transfered_size = 0;
	if (setup_request->request == USB_DEVREQ_SET_CONFIGURATION) {
		//set and get configuration requests do not have any meaning,
		//only dummy values are returned
		return EOK;
	}
	/* USB_DEVREQ_SET_DESCRIPTOR is also not supported */
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
/**
 * process one of requests that do not request nor carry additional data
 *
 * Request can be one of USB_DEVREQ_CLEAR_FEATURE, USB_DEVREQ_SET_FEATURE or
 * USB_DEVREQ_SET_ADDRESS.
 * @param instance root hub instance
 * @param request structure containing both request and response information
 * @return error code
 */
int process_request_without_data(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	const usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	request->transfered_size = 0;
	const int request_type = setup_request->request_type;
	switch (setup_request->request)
	{
	case USB_DEVREQ_CLEAR_FEATURE:
		if (request_type == USB_HUB_REQ_TYPE_SET_HUB_FEATURE) {
			usb_log_debug("USB_HUB_REQ_TYPE_SET_HUB_FEATURE\n");
			return process_hub_feature_clear_request(instance,
			    setup_request->value);
		}
		if (request_type == USB_HUB_REQ_TYPE_SET_PORT_FEATURE) {
			usb_log_debug("USB_HUB_REQ_TYPE_SET_PORT_FEATURE\n");
			return process_port_feature_clear_request(instance,
			    setup_request->value, setup_request->index);
		}
		usb_log_error("Invalid HUB clear feature request type: %d\n",
		    request_type);
		return EINVAL;

	case USB_DEVREQ_SET_FEATURE:
		if (request_type == USB_HUB_REQ_TYPE_SET_HUB_FEATURE) {
			usb_log_debug("USB_HUB_REQ_TYPE_SET_HUB_FEATURE\n");
			return process_hub_feature_set_request(instance,
			    setup_request->value);
		}
		if (request_type == USB_HUB_REQ_TYPE_SET_PORT_FEATURE) {
			usb_log_debug("USB_HUB_REQ_TYPE_SET_PORT_FEATURE\n");
			return process_port_feature_set_request(instance,
			    setup_request->value, setup_request->index);
		}
		usb_log_error("Invalid HUB set feature request type: %d\n",
		    request_type);
		return EINVAL;

	case USB_DEVREQ_SET_ADDRESS:
		usb_log_debug("USB_DEVREQ_SET_ADDRESS\n");
		return process_address_set_request(instance,
		    setup_request->value);

	default:
		usb_log_error("Invalid HUB request: %d\n",
		    setup_request->request);
		return ENOTSUP;
	}
}
/*----------------------------------------------------------------------------*/
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
int process_ctrl_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	if (!request->setup_buffer) {
		usb_log_error("Root hub received empty transaction!");
		return EINVAL;
	}
	if (sizeof(usb_device_request_setup_packet_t) > request->setup_size) {
		usb_log_error("Setup packet too small\n");
		return EOVERFLOW;
	}
	usb_log_debug2("CTRL packet: %s.\n",
	    usb_debug_str_buffer((uint8_t *) request->setup_buffer, 8, 8));
	const usb_device_request_setup_packet_t *setup_request =
	    (usb_device_request_setup_packet_t *) request->setup_buffer;
	switch (setup_request->request)
	{
	case USB_DEVREQ_GET_STATUS:
	case USB_DEVREQ_GET_DESCRIPTOR:
	case USB_DEVREQ_GET_CONFIGURATION:
		usb_log_debug2("Processing request with output\n");
		return process_request_with_output(instance, request);
	case USB_DEVREQ_CLEAR_FEATURE:
	case USB_DEVREQ_SET_FEATURE:
	case USB_DEVREQ_SET_ADDRESS:
		usb_log_debug2("Processing request without "
		    "additional data\n");
		return process_request_without_data(instance, request);
	case USB_DEVREQ_SET_DESCRIPTOR:
	case USB_DEVREQ_SET_CONFIGURATION:
		usb_log_debug2("Processing request with input\n");
		return process_request_with_input(instance, request);
	default:
		usb_log_error("Received unsupported request: %d.\n",
		    setup_request->request);
		return ENOTSUP;
	}
}
/*----------------------------------------------------------------------------*/
/**
 * process hanging interrupt request
 *
 * If an interrupt transfer has been received and there was no change,
 * the driver stores the transfer information and waits for change to occur.
 * This routine is called when that happens and it finalizes the interrupt
 * transfer.
 *
 * @param instance hub instance
 * @param request batch request to be processed
 *
 * @return
 */
int process_interrupt_mask_in_instance(
    rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);

	memcpy(request->data_buffer, instance->interrupt_buffer,
	    instance->interrupt_mask_size);
	request->transfered_size = instance->interrupt_mask_size;
	instance->unfinished_interrupt_transfer = NULL;
	usb_transfer_batch_finish_error(request, EOK);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * return whether the buffer is full of zeros
 *
 * Convenience function.
 * @param buffer
 * @param size
 * @return
 */
bool is_zeros(const void *buffer, size_t size)
{
	if (!buffer) return true;
	const char * const end = buffer + size;
	const char *data = buffer;
	for (; data < end; ++data) {
		if (*data)
			return false;
	}
	return true;
}

/**
 * @}
 */
