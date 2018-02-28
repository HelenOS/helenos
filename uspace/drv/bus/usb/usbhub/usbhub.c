/*
 * Copyright (c) 2010 Matus Dekanek
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek
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

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * @brief usb hub main functionality
 */

#include <ddf/driver.h>
#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <stdio.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/dev/pipes.h>
#include <usb/classes/classes.h>
#include <usb/descriptor.h>
#include <usb/dev/recognise.h>
#include <usb/dev/request.h>
#include <usb/classes/hub.h>
#include <usb/dev/poll.h>
#include <usbhc_iface.h>

#include "usbhub.h"
#include "status.h"

#define HUB_FNC_NAME "hub"

#define HUB_STATUS_CHANGE_EP(protocol) { \
	.transfer_type = USB_TRANSFER_INTERRUPT, \
	.direction = USB_DIRECTION_IN, \
	.interface_class = USB_CLASS_HUB, \
	.interface_subclass = 0, \
	.interface_protocol = (protocol), \
	.flags = 0 \
}

/**
 * Hub status-change endpoint description.
 *
 * According to USB 2.0 specification, there are two possible arrangements of
 * endpoints, depending on whether the hub has a MTT or not.
 *
 * Under any circumstances, there shall be exactly one endpoint descriptor.
 * Though to be sure, let's map the protocol precisely. The possible
 * combinations are:
 *	                      | bDeviceProtocol | bInterfaceProtocol
 *	Only single TT        |       0         |         0
 *	MTT in Single-TT mode |       2         |         1
 *	MTT in MTT mode       |       2         |         2     (iface alt. 1)
 */
static const usb_endpoint_description_t
	status_change_single_tt_only = HUB_STATUS_CHANGE_EP(0),
	status_change_mtt_available = HUB_STATUS_CHANGE_EP(1);

const usb_endpoint_description_t *usb_hub_endpoints [] = {
	&status_change_single_tt_only,
	&status_change_mtt_available,
};

/** Standard get hub global status request */
static const usb_device_request_setup_packet_t get_hub_status_request = {
	.request_type = USB_HUB_REQ_TYPE_GET_HUB_STATUS,
	.request = USB_HUB_REQUEST_GET_STATUS,
	.index = 0,
	.value = 0,
	.length = sizeof(usb_hub_status_t),
};

static errno_t usb_set_first_configuration(usb_device_t *);
static errno_t usb_hub_process_hub_specific_info(usb_hub_dev_t *);
static void usb_hub_over_current(const usb_hub_dev_t *, usb_hub_status_t);
static errno_t usb_hub_polling_init(usb_hub_dev_t *, usb_endpoint_mapping_t *);
static void usb_hub_global_interrupt(const usb_hub_dev_t *);

static bool usb_hub_polling_error_callback(usb_device_t *dev,
	errno_t err_code, void *arg)
{
	assert(dev);
	assert(arg);

	usb_log_error("Device %s polling error: %s",
		usb_device_get_name(dev), str_error(err_code));

	return true;
}

/**
 * Initialize hub device driver structure.
 *
 * Creates hub representation and fibril that periodically checks hub's status.
 * Hub representation is passed to the fibril.
 * @param usb_dev generic usb device information
 * @return error code
 */
errno_t usb_hub_device_add(usb_device_t *usb_dev)
{
	errno_t err;
	assert(usb_dev);

	/* Create driver soft-state structure */
	usb_hub_dev_t *hub_dev =
	    usb_device_data_alloc(usb_dev, sizeof(usb_hub_dev_t));
	if (hub_dev == NULL) {
		usb_log_error("Failed to create hub driver structure.");
		return ENOMEM;
	}
	hub_dev->usb_device = usb_dev;
	hub_dev->speed = usb_device_get_speed(usb_dev);

	/* Set hub's first configuration. (There should be only one) */
	if ((err = usb_set_first_configuration(usb_dev))) {
		usb_log_error("Could not set hub configuration: %s", str_error(err));
		return err;
	}

	/* Get port count and create attached_devices. */
	if ((err = usb_hub_process_hub_specific_info(hub_dev))) {
		usb_log_error("Could process hub specific info, %s", str_error(err));
		return err;
	}

	const usb_endpoint_description_t *status_change = hub_dev->mtt_available
	    ? &status_change_mtt_available
	    : &status_change_single_tt_only;

	usb_endpoint_mapping_t *status_change_mapping
		= usb_device_get_mapped_ep_desc(hub_dev->usb_device, status_change);
	if (!status_change_mapping) {
		usb_log_error("Failed to map the Status Change Endpoint of a hub.");
		return EIO;
	}

	/* Create hub control function. */
	usb_log_debug("Creating DDF function '" HUB_FNC_NAME "'.");
	hub_dev->hub_fun = usb_device_ddf_fun_create(hub_dev->usb_device,
	    fun_exposed, HUB_FNC_NAME);
	if (hub_dev->hub_fun == NULL) {
		usb_log_error("Failed to create hub function.");
		return ENOMEM;
	}

	/* Bind hub control function. */
	if ((err = ddf_fun_bind(hub_dev->hub_fun))) {
		usb_log_error("Failed to bind hub function: %s.", str_error(err));
		goto err_ddf_fun;
	}

	/* Start hub operation. */
	if ((err = usb_hub_polling_init(hub_dev, status_change_mapping))) {
		usb_log_error("Failed to start polling: %s.", str_error(err));
		goto err_bound;
	}

	usb_log_info("Controlling %s-speed hub '%s' (%p: %zu ports).",
	    usb_str_speed(hub_dev->speed),
	    usb_device_get_name(hub_dev->usb_device), hub_dev,
	    hub_dev->port_count);

	return EOK;

err_bound:
	ddf_fun_unbind(hub_dev->hub_fun);
err_ddf_fun:
	ddf_fun_destroy(hub_dev->hub_fun);
	return err;
}

static errno_t usb_hub_cleanup(usb_hub_dev_t *hub)
{
	free(hub->polling.buffer);
	usb_polling_fini(&hub->polling);

	for (size_t port = 0; port < hub->port_count; ++port) {
		usb_port_fini(&hub->ports[port].base);
	}
	free(hub->ports);

	const errno_t ret = ddf_fun_unbind(hub->hub_fun);
	if (ret != EOK) {
		usb_log_error("(%p) Failed to unbind '%s' function: %s.",
		   hub, HUB_FNC_NAME, str_error(ret));
		return ret;
	}
	ddf_fun_destroy(hub->hub_fun);

	usb_log_info("(%p) USB hub driver stopped and cleaned.", hub);

	/* Device data (usb_hub_dev_t) will be freed by usbdev. */
	return EOK;
}

/**
 * Turn off power to all ports.
 *
 * @param usb_dev generic usb device information
 * @return error code
 */
errno_t usb_hub_device_remove(usb_device_t *usb_dev)
{
	assert(usb_dev);
	usb_hub_dev_t *hub = usb_device_data_get(usb_dev);
	assert(hub);

	usb_log_info("(%p) USB hub removed, joining polling fibril.", hub);

	/* Join polling fibril (ignoring error code). */
	usb_polling_join(&hub->polling);
	usb_log_info("(%p) USB hub polling stopped, freeing memory.", hub);

	/* Destroy hub. */
	return usb_hub_cleanup(hub);
}

/**
 * Remove all attached devices
 * @param usb_dev generic usb device information
 * @return error code
 */
errno_t usb_hub_device_gone(usb_device_t *usb_dev)
{
	assert(usb_dev);
	usb_hub_dev_t *hub = usb_device_data_get(usb_dev);
	assert(hub);

	usb_log_info("(%p) USB hub gone, joining polling fibril.", hub);

	/* Join polling fibril (ignoring error code). */
	usb_polling_join(&hub->polling);
	usb_log_info("(%p) USB hub polling stopped, freeing memory.", hub);

	/* Destroy hub. */
	return usb_hub_cleanup(hub);
}

/**
 * Initialize and start the polling of the Status Change Endpoint.
 *
 * @param mapping The mapping of Status Change Endpoint
 */
static errno_t usb_hub_polling_init(usb_hub_dev_t *hub_dev,
	usb_endpoint_mapping_t *mapping)
{
	errno_t err;
	usb_polling_t *polling = &hub_dev->polling;

	if ((err = usb_polling_init(polling)))
		return err;

	polling->device = hub_dev->usb_device;
	polling->ep_mapping = mapping;
	polling->request_size = ((hub_dev->port_count + 1 + 7) / 8);
	polling->buffer = malloc(polling->request_size);
	polling->on_data = hub_port_changes_callback;
	polling->on_error = usb_hub_polling_error_callback;
	polling->arg = hub_dev;

	if ((err = usb_polling_start(polling))) {
		/* Polling is already initialized. */
		free(polling->buffer);
		usb_polling_fini(polling);
		return err;
	}

	return EOK;
}

/**
 * Callback for polling hub for changes.
 *
 * @param dev Device where the change occured.
 * @param change_bitmap Bitmap of changed ports.
 * @param change_bitmap_size Size of the bitmap in bytes.
 * @param arg Custom argument, points to @c usb_hub_dev_t.
 * @return Whether to continue polling.
 */
bool hub_port_changes_callback(usb_device_t *dev,
    uint8_t *change_bitmap, size_t change_bitmap_size, void *arg)
{
	usb_hub_dev_t *hub = arg;
	assert(hub);

	/* It is an error condition if we didn't receive enough data */
	if (change_bitmap_size == 0) {
		return false;
	}

	/* Lowest bit indicates global change */
	const bool change = change_bitmap[0] & 1;
	if (change) {
		usb_hub_global_interrupt(hub);
	}

	/* Nth bit indicates change on port N */
	for (size_t port = 0; port < hub->port_count; ++port) {
		const size_t bit = port + 1;
		const bool change = (change_bitmap[bit / 8] >> (bit % 8)) & 1;
		if (change) {
			usb_hub_port_process_interrupt(&hub->ports[port]);
		}
	}
	return true;
}

static void usb_hub_power_ports(usb_hub_dev_t *hub_dev)
{
	if (!hub_dev->power_switched) {
		usb_log_info("(%p): Power switching not supported, "
		    "ports always powered.", hub_dev);
		return;
	}

	usb_log_info("(%p): Hub port power switching enabled (%s).", hub_dev,
	    hub_dev->per_port_power ? "per port" : "ganged");

	for (unsigned int port = 0; port < hub_dev->port_count; ++port) {
		usb_log_debug("(%p): Powering port %u.", hub_dev, port + 1);
		const errno_t ret = usb_hub_set_port_feature(hub_dev, port + 1,
		    USB_HUB_FEATURE_PORT_POWER);

		if (ret != EOK) {
			usb_log_error("(%p-%u): Cannot power on port: %s.",
			    hub_dev, hub_dev->ports[port].port_number,
			    str_error(ret));
			/* Continue to try at least other ports */
		}
	}
}

/**
 * Load hub-specific information into hub_dev structure and process if needed
 *
 * Read port count and initialize structures holding per port information.
 * If there are any non-removable devices, start initializing them.
 * This function is hub-specific and should be run only after the hub is
 * configured using usb_set_first_configuration function.
 * @param hub_dev hub representation
 * @return error code
 */
static errno_t usb_hub_process_hub_specific_info(usb_hub_dev_t *hub_dev)
{
	assert(hub_dev);

	usb_log_debug("(%p): Retrieving descriptor.", hub_dev);
	usb_pipe_t *control_pipe = usb_device_get_default_pipe(hub_dev->usb_device);

	usb_descriptor_type_t desc_type = hub_dev->speed >= USB_SPEED_SUPER
		? USB_DESCTYPE_SSPEED_HUB : USB_DESCTYPE_HUB;

	/* Get hub descriptor. */
	usb_hub_descriptor_header_t descriptor;
	size_t received_size;
	errno_t opResult = usb_request_get_descriptor(control_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_DEVICE,
	    desc_type, 0, 0, &descriptor,
	    sizeof(usb_hub_descriptor_header_t), &received_size);
	if (opResult != EOK) {
		usb_log_error("(%p): Failed to receive hub descriptor: %s.",
		    hub_dev, str_error(opResult));
		return opResult;
	}

	usb_log_debug("(%p): Setting port count to %d.", hub_dev,
	    descriptor.port_count);
	hub_dev->port_count = descriptor.port_count;
	hub_dev->control_pipe = control_pipe;

	usb_log_debug("(%p): Setting hub depth to %u.", hub_dev,
	    usb_device_get_depth(hub_dev->usb_device));
	if ((opResult = usb_hub_set_depth(hub_dev))) {
		usb_log_error("(%p): Failed to set hub depth: %s.",
		    hub_dev, str_error(opResult));
		return opResult;
	}

	hub_dev->ports = calloc(hub_dev->port_count, sizeof(usb_hub_port_t));
	if (!hub_dev->ports) {
		return ENOMEM;
	}

	for (size_t port = 0; port < hub_dev->port_count; ++port) {
		usb_hub_port_init(&hub_dev->ports[port], hub_dev, port + 1);
	}

	hub_dev->power_switched =
	    !(descriptor.characteristics & HUB_CHAR_NO_POWER_SWITCH_FLAG);
	hub_dev->per_port_power =
	    descriptor.characteristics & HUB_CHAR_POWER_PER_PORT_FLAG;

	const uint8_t protocol = usb_device_descriptors(hub_dev->usb_device)
		->device.device_protocol;
	hub_dev->mtt_available = (protocol == 2);

	usb_hub_power_ports(hub_dev);

	return EOK;
}

/**
 * Set configuration of and USB device
 *
 * Check whether there is at least one configuration and sets the first one.
 * This function should be run prior to running any hub-specific action.
 * @param usb_device usb device representation
 * @return error code
 */
static errno_t usb_set_first_configuration(usb_device_t *usb_device)
{
	assert(usb_device);
	/* Get number of possible configurations from device descriptor */
	const size_t configuration_count =
	    usb_device_descriptors(usb_device)->device.configuration_count;
	usb_log_debug("Hub has %zu configurations.", configuration_count);

	if (configuration_count < 1) {
		usb_log_error("There are no configurations available");
		return EINVAL;
	}

	const size_t config_size =
	    usb_device_descriptors(usb_device)->full_config_size;
	const usb_standard_configuration_descriptor_t *config_descriptor =
	    usb_device_descriptors(usb_device)->full_config;

	if (config_size < sizeof(usb_standard_configuration_descriptor_t)) {
	    usb_log_error("Configuration descriptor is not big enough"
	        " to fit standard configuration descriptor.\n");
	    return EOVERFLOW;
	}

	/* Set configuration. Use the configuration that was in
	 * usb_device->descriptors.configuration i.e. The first one. */
	errno_t opResult = usb_request_set_configuration(
	    usb_device_get_default_pipe(usb_device),
	    config_descriptor->configuration_number);
	if (opResult != EOK) {
		usb_log_error("Failed to set hub configuration: %s.",
		    str_error(opResult));
	} else {
		usb_log_debug("\tUsed configuration %d",
		    config_descriptor->configuration_number);
	}

	return opResult;
}

/**
 * Process hub over current change
 *
 * This means either to power off the hub or power it on.
 * @param hub_dev hub instance
 * @param status hub status bitmask
 * @return error code
 */
static void usb_hub_over_current(const usb_hub_dev_t *hub_dev,
    usb_hub_status_t status)
{
	if (status & USB_HUB_STATUS_OVER_CURRENT) {
		/* Hub should remove power from all ports if it detects OC */
		usb_log_warning("(%p) Detected hub over-current condition, "
		    "all ports should be powered off.", hub_dev);
		return;
	}

	/* Ports are always powered. */
	if (!hub_dev->power_switched)
		return;

	/* Over-current condition is gone, it is safe to turn the ports on. */
	for (size_t port = 0; port < hub_dev->port_count; ++port) {
		const errno_t ret = usb_hub_set_port_feature(hub_dev, port,
		    USB_HUB_FEATURE_PORT_POWER);
		if (ret != EOK) {
			usb_log_warning("(%p-%u): HUB OVER-CURRENT GONE: Cannot"
			    " power on port: %s\n", hub_dev,
			    hub_dev->ports[port].port_number, str_error(ret));
		} else {
			if (!hub_dev->per_port_power)
				return;
		}
	}
}

/**
 * Set feature on the real hub port.
 *
 * @param port Port structure.
 * @param feature Feature selector.
 */
errno_t usb_hub_set_depth(const usb_hub_dev_t *hub)
{
	assert(hub);

	/* Slower hubs do not care about depth */
	if (hub->speed < USB_SPEED_SUPER)
		return EOK;

	const usb_device_request_setup_packet_t set_request = {
		.request_type = USB_HUB_REQ_TYPE_SET_HUB_DEPTH,
		.request = USB_HUB_REQUEST_SET_HUB_DEPTH,
		.value = uint16_host2usb(usb_device_get_depth(hub->usb_device) - 1),
		.index = 0,
		.length = 0,
	};
	return usb_pipe_control_write(hub->control_pipe, &set_request,
	    sizeof(set_request), NULL, 0);
}

/**
 * Set feature on the real hub port.
 *
 * @param port Port structure.
 * @param feature Feature selector.
 */
errno_t usb_hub_set_port_feature(const usb_hub_dev_t *hub, size_t port_number,
    usb_hub_class_feature_t feature)
{
	assert(hub);
	const usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE,
		.request = USB_DEVREQ_SET_FEATURE,
		.index = uint16_host2usb(port_number),
		.value = feature,
		.length = 0,
	};
	return usb_pipe_control_write(hub->control_pipe, &clear_request,
	    sizeof(clear_request), NULL, 0);
}

/**
 * Clear feature on the real hub port.
 *
 * @param port Port structure.
 * @param feature Feature selector.
 */
errno_t usb_hub_clear_port_feature(const usb_hub_dev_t *hub, size_t port_number,
    usb_hub_class_feature_t feature)
{
	assert(hub);
	const usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE,
		.request = USB_DEVREQ_CLEAR_FEATURE,
		.value = feature,
		.index = uint16_host2usb(port_number),
		.length = 0,
	};
	return usb_pipe_control_write(hub->control_pipe,
	    &clear_request, sizeof(clear_request), NULL, 0);
}

/**
 * Retrieve port status.
 *
 * @param[in] port Port structure
 * @param[out] status Where to store the port status.
 * @return Error code.
 */
errno_t usb_hub_get_port_status(const usb_hub_dev_t *hub, size_t port_number,
    usb_port_status_t *status)
{
	assert(hub);
	assert(status);

	/* USB hub specific GET_PORT_STATUS request. See USB Spec 11.16.2.6
	 * Generic GET_STATUS request cannot be used because of the difference
	 * in status data size (2B vs. 4B)*/
	const usb_device_request_setup_packet_t request = {
		.request_type = USB_HUB_REQ_TYPE_GET_PORT_STATUS,
		.request = USB_HUB_REQUEST_GET_STATUS,
		.value = 0,
		.index = uint16_host2usb(port_number),
		.length = sizeof(usb_port_status_t),
	};
	size_t recv_size;

	uint32_t buffer;
	const errno_t rc = usb_pipe_control_read(hub->control_pipe,
	    &request, sizeof(usb_device_request_setup_packet_t),
	    &buffer, sizeof(buffer), &recv_size);
	if (rc != EOK)
		return rc;

	if (recv_size != sizeof(*status))
		return ELIMIT;

	*status = uint32_usb2host(buffer);
	return EOK;
}

/**
 * Process hub interrupts.
 *
 * The change can be either in the over-current condition or local-power change.
 * @param hub_dev hub instance
 */
static void usb_hub_global_interrupt(const usb_hub_dev_t *hub_dev)
{
	assert(hub_dev);
	assert(hub_dev->usb_device);
	usb_log_debug("(%p): Global interrupt on th hub.",  hub_dev);
	usb_pipe_t *control_pipe =
	    usb_device_get_default_pipe(hub_dev->usb_device);

	usb_hub_status_t status;
	size_t rcvd_size;
	/* NOTE: We can't use standard USB GET_STATUS request, because
	 * hubs reply is 4byte instead of 2 */
	const errno_t opResult = usb_pipe_control_read(control_pipe,
	    &get_hub_status_request, sizeof(get_hub_status_request),
	    &status, sizeof(usb_hub_status_t), &rcvd_size);
	if (opResult != EOK) {
		usb_log_error("(%p): Could not get hub status: %s.", hub_dev,
		    str_error(opResult));
		return;
	}
	if (rcvd_size != sizeof(usb_hub_status_t)) {
		usb_log_error("(%p): Received status has incorrect size: "
		    "%zu != %zu", hub_dev, rcvd_size, sizeof(usb_hub_status_t));
		return;
	}

	/* Handle status changes */
	if (status & USB_HUB_STATUS_C_OVER_CURRENT) {
		usb_hub_over_current(hub_dev, status);
		/* Ack change in hub OC flag */
		const errno_t ret = usb_request_clear_feature(
		    control_pipe, USB_REQUEST_TYPE_CLASS,
		    USB_REQUEST_RECIPIENT_DEVICE,
		    USB_HUB_FEATURE_C_HUB_OVER_CURRENT, 0);
		if (ret != EOK) {
			usb_log_error("(%p): Failed to clear hub over-current "
			    "change flag: %s.\n", hub_dev, str_error(opResult));
		}
	}

	if (status & USB_HUB_STATUS_C_LOCAL_POWER) {
		/* NOTE: Handling this is more complicated.
		 * If the transition is from bus power to local power, all
		 * is good and we may signal the parent hub that we don't
		 * need the power.
		 * If the transition is from local power to bus power
		 * the hub should turn off all the ports and devices need
		 * to be reinitialized taking into account the limited power
		 * that is now available.
		 * There is no support for power distribution in HelenOS,
		 * (or other OSes/hub devices that I've seen) so this is not
		 * implemented.
		 * Just ACK the change.
		 */
		const errno_t ret = usb_request_clear_feature(
		    control_pipe, USB_REQUEST_TYPE_CLASS,
		    USB_REQUEST_RECIPIENT_DEVICE,
		    USB_HUB_FEATURE_C_HUB_LOCAL_POWER, 0);
		if (opResult != EOK) {
			usb_log_error("(%p): Failed to clear hub power change "
			    "flag: %s.\n", hub_dev, str_error(ret));
		}
	}
}

/**
 * Instead of just sleeping, we may as well sleep on a condition variable.
 * This has the advantage that we may instantly wait other hub from the polling
 * sleep, mitigating the delay of polling while still being synchronized with
 * other devices in need of the default address (there shall not be any).
 */
static FIBRIL_CONDVAR_INITIALIZE(global_hub_default_address_cv);
static FIBRIL_MUTEX_INITIALIZE(global_hub_default_address_guard);

/**
 * Reserve a default address for a port across all other devices connected to
 * the bus. We aggregate requests for ports to minimize delays between
 * connecting multiple devices from one hub - which happens e.g. when the hub
 * is connected with already attached devices.
 */
errno_t usb_hub_reserve_default_address(usb_hub_dev_t *hub, async_exch_t *exch,
    usb_port_t *port)
{
	assert(hub);
	assert(exch);
	assert(port);
	assert(fibril_mutex_is_locked(&port->guard));

	errno_t err = usbhc_reserve_default_address(exch);
	/*
	 * EINVAL signalls that its our hub (hopefully different port) that has
	 * this address reserved
	 */
	while (err == EAGAIN || err == EINVAL) {
		/* Drop the port guard, we're going to wait */
		fibril_mutex_unlock(&port->guard);

		/* This sleeping might be disturbed by other hub */
		fibril_mutex_lock(&global_hub_default_address_guard);
		fibril_condvar_wait_timeout(&global_hub_default_address_cv,
		    &global_hub_default_address_guard, 2000000);
		fibril_mutex_unlock(&global_hub_default_address_guard);

		fibril_mutex_lock(&port->guard);
		err = usbhc_reserve_default_address(exch);
	}

	if (err)
		return err;

	/*
	 * As we dropped the port guard, we need to check whether the device is
	 * still connected. If the release fails, we still hold the default
	 * address -- but then there is probably a bigger problem with the HC
	 * anyway.
	 */
	if (port->state != PORT_CONNECTING) {
		err = usb_hub_release_default_address(hub, exch);
		return err ? err : EINTR;
	}

	return EOK;
}

/**
 * Release the default address from a port.
 */
errno_t usb_hub_release_default_address(usb_hub_dev_t *hub, async_exch_t *exch)
{
	const errno_t ret = usbhc_release_default_address(exch);

	/*
	 * This is an optimistic optimization - it may wake
	 * one hub from polling sleep instantly.
	 */
	fibril_condvar_signal(&global_hub_default_address_cv);

	return ret;
}

/**
 * @}
 */
