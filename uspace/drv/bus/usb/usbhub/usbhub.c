/*
 * Copyright (c) 2010 Matus Dekanek
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
/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * @brief usb hub main functionality
 */

#include <ddf/driver.h>
#include <bool.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <stdio.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/dev/pipes.h>
#include <usb/classes/classes.h>
#include <usb/ddfiface.h>
#include <usb/descriptor.h>
#include <usb/dev/recognise.h>
#include <usb/dev/request.h>
#include <usb/classes/hub.h>
#include <usb/dev/poll.h>
#include <usb_iface.h>

#include "usbhub.h"
#include "status.h"

#define HUB_FNC_NAME "hub"

/** Standard get hub global status request */
static const usb_device_request_setup_packet_t get_hub_status_request = {
	.request_type = USB_HUB_REQ_TYPE_GET_HUB_STATUS,
	.request = USB_HUB_REQUEST_GET_STATUS,
	.index = 0,
	.value = 0,
	.length = sizeof(usb_hub_status_t),
};

static int usb_set_first_configuration(usb_device_t *usb_device);
static usb_hub_info_t * usb_hub_info_create(usb_device_t *usb_dev);
static int usb_hub_process_hub_specific_info(usb_hub_info_t *hub_info);
static void usb_hub_over_current(const usb_hub_info_t *hub_info,
    usb_hub_status_t status);
static void usb_hub_global_interrupt(const usb_hub_info_t *hub_info);
static void usb_hub_polling_terminated_callback(usb_device_t *device,
    bool was_error, void *data);

/**
 * Initialize hub device driver fibril
 *
 * Creates hub representation and fibril that periodically checks hub's status.
 * Hub representation is passed to the fibril.
 * @param usb_dev generic usb device information
 * @return error code
 */
int usb_hub_device_add(usb_device_t *usb_dev)
{
	assert(usb_dev);
	/* Create driver soft-state structure */
	usb_hub_info_t *hub_info = usb_hub_info_create(usb_dev);
	if (hub_info == NULL) {
		usb_log_error("Failed to create hun driver structure.\n");
		return ENOMEM;
	}

	/* Create hc connection */
	usb_log_debug("Initializing USB wire abstraction.\n");
	int opResult = usb_hc_connection_initialize_from_device(
	    &hub_info->connection, hub_info->usb_device->ddf_dev);
	if (opResult != EOK) {
		usb_log_error("Could not initialize connection to device: %s\n",
		    str_error(opResult));
		free(hub_info);
		return opResult;
	}

	/* Set hub's first configuration. (There should be only one) */
	opResult = usb_set_first_configuration(usb_dev);
	if (opResult != EOK) {
		usb_log_error("Could not set hub configuration: %s\n",
		    str_error(opResult));
		free(hub_info);
		return opResult;
	}

	//get port count and create attached_devs
	opResult = usb_hub_process_hub_specific_info(hub_info);
	if (opResult != EOK) {
		usb_log_error("Could process hub specific info, %s\n",
		    str_error(opResult));
		free(hub_info);
		return opResult;
	}

	usb_log_debug("Creating DDF function '" HUB_FNC_NAME "'.\n");
	ddf_fun_t *hub_fun = ddf_fun_create(hub_info->usb_device->ddf_dev,
	    fun_exposed, HUB_FNC_NAME);
	if (hub_fun == NULL) {
		usb_log_error("Failed to create hub function.\n");
		free(hub_info);
		return ENOMEM;
	}

	opResult = ddf_fun_bind(hub_fun);
	if (opResult != EOK) {
		usb_log_error("Failed to bind hub function: %s.\n",
		   str_error(opResult));
		free(hub_info);
		ddf_fun_destroy(hub_fun);
		return opResult;
	}

	opResult = usb_device_auto_poll(hub_info->usb_device, 0,
	    hub_port_changes_callback, ((hub_info->port_count + 1) / 8) + 1,
	    usb_hub_polling_terminated_callback, hub_info);
	if (opResult != EOK) {
		ddf_fun_destroy(hub_fun);
		free(hub_info);
		usb_log_error("Failed to create polling fibril: %s.\n",
		    str_error(opResult));
		return opResult;
	}
	usb_log_info("Controlling hub '%s' (%zu ports).\n",
	    hub_info->usb_device->ddf_dev->name, hub_info->port_count);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Callback for polling hub for changes.
 *
 * @param dev Device where the change occured.
 * @param change_bitmap Bitmap of changed ports.
 * @param change_bitmap_size Size of the bitmap in bytes.
 * @param arg Custom argument, points to @c usb_hub_info_t.
 * @return Whether to continue polling.
 */
bool hub_port_changes_callback(usb_device_t *dev,
    uint8_t *change_bitmap, size_t change_bitmap_size, void *arg)
{
	usb_log_debug("hub_port_changes_callback\n");
	usb_hub_info_t *hub = arg;
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

	/* N + 1 bit indicates change on port N */
	size_t port = 1;
	for (; port < hub->port_count + 1; port++) {
		const bool change = (change_bitmap[port / 8] >> (port % 8)) & 1;
		if (change) {
			usb_hub_port_process_interrupt(&hub->ports[port], hub);
		}
	}
	return true;
}
/*----------------------------------------------------------------------------*/
/**
 * create usb_hub_info_t structure
 *
 * Does only basic copying of known information into new structure.
 * @param usb_dev usb device structure
 * @return basic usb_hub_info_t structure
 */
static usb_hub_info_t * usb_hub_info_create(usb_device_t *usb_dev)
{
	assert(usb_dev);
	usb_hub_info_t *info = malloc(sizeof(usb_hub_info_t));
	if (!info)
	    return NULL;

	info->usb_device = usb_dev;

	info->ports = NULL;
	info->port_count = -1;
	info->pending_ops_count = 0;
	fibril_mutex_initialize(&info->pending_ops_mutex);
	fibril_condvar_initialize(&info->pending_ops_cv);

	return info;
}
/*----------------------------------------------------------------------------*/
/**
 * Load hub-specific information into hub_info structure and process if needed
 *
 * Read port count and initialize structures holding per port information.
 * If there are any non-removable devices, start initializing them.
 * This function is hub-specific and should be run only after the hub is
 * configured using usb_set_first_configuration function.
 * @param hub_info hub representation
 * @return error code
 */
static int usb_hub_process_hub_specific_info(usb_hub_info_t *hub_info)
{
	assert(hub_info);

	/* Get hub descriptor. */
	usb_log_debug("Retrieving descriptor\n");
	usb_pipe_t *control_pipe = &hub_info->usb_device->ctrl_pipe;

	usb_hub_descriptor_header_t descriptor;
	size_t received_size;
	int opResult = usb_request_get_descriptor(control_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_HUB, 0, 0, &descriptor,
	    sizeof(usb_hub_descriptor_header_t), &received_size);
	if (opResult != EOK) {
		usb_log_error("Failed to receive hub descriptor: %s.\n",
		    str_error(opResult));
		return opResult;
	}

	usb_log_debug("Setting port count to %d.\n", descriptor.port_count);
	hub_info->port_count = descriptor.port_count;

	// TODO: +1 hack is no longer necessary
	hub_info->ports =
	    calloc(hub_info->port_count + 1, sizeof(usb_hub_port_t));
	if (!hub_info->ports) {
		return ENOMEM;
	}

	for (size_t port = 1; port < hub_info->port_count + 1; ++port) {
		usb_hub_port_init(&hub_info->ports[port], port, control_pipe);
	}

	const bool is_power_switched =
	    !(descriptor.characteristics & HUB_CHAR_NO_POWER_SWITCH_FLAG);
	if (is_power_switched) {
		usb_log_debug("Hub power switched\n");
		const bool per_port_power = descriptor.characteristics
		    & HUB_CHAR_POWER_PER_PORT_FLAG;

		for (size_t port = 1; port <= hub_info->port_count; ++port) {
			usb_log_debug("Powering port %zu.\n", port);
			opResult = usb_hub_port_set_feature(
			    &hub_info->ports[port], USB_HUB_FEATURE_PORT_POWER);
			if (opResult != EOK) {
				usb_log_error("Cannot power on port %zu: %s.\n",
				    port, str_error(opResult));
			} else {
				if (!per_port_power) {
					usb_log_debug(
					    "Ganged power switching mode, "
					    "one port is enough.\n");
					break;
				}
			}
		}
	} else {
		usb_log_debug("Power not switched, ports always powered\n");
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * Set configuration of and USB device
 *
 * Check whether there is at least one configuration and sets the first one.
 * This function should be run prior to running any hub-specific action.
 * @param usb_device usb device representation
 * @return error code
 */
static int usb_set_first_configuration(usb_device_t *usb_device)
{
	assert(usb_device);
	/* Get number of possible configurations from device descriptor */
	const size_t configuration_count =
	    usb_device->descriptors.device.configuration_count;
	usb_log_debug("Hub has %zu configurations.\n", configuration_count);

	if (configuration_count < 1) {
		usb_log_error("There are no configurations available\n");
		return EINVAL;
	}

	if (usb_device->descriptors.configuration_size
	    < sizeof(usb_standard_configuration_descriptor_t)) {
	    usb_log_error("Configuration descriptor is not big enough"
	        " to fit standard configuration descriptor.\n");
	    return EOVERFLOW;
	}

	// TODO: Make sure that the cast is correct
	usb_standard_configuration_descriptor_t *config_descriptor
	    = (usb_standard_configuration_descriptor_t *)
	    usb_device->descriptors.configuration;

	/* Set configuration. Use the configuration that was in
	 * usb_device->descriptors.configuration i.e. The first one. */
	const int opResult = usb_request_set_configuration(
	    &usb_device->ctrl_pipe, config_descriptor->configuration_number);
	if (opResult != EOK) {
		usb_log_error("Failed to set hub configuration: %s.\n",
		    str_error(opResult));
	} else {
		usb_log_debug("\tUsed configuration %d\n",
		    config_descriptor->configuration_number);
	}
	return opResult;
}
/*----------------------------------------------------------------------------*/
/**
 * Process hub over current change
 *
 * This means either to power off the hub or power it on.
 * @param hub_info hub instance
 * @param status hub status bitmask
 * @return error code
 */
static void usb_hub_over_current(const usb_hub_info_t *hub_info,
    usb_hub_status_t status)
{
	if (status & USB_HUB_STATUS_OVER_CURRENT) {
		/* Hub should remove power from all ports if it detects OC */
		usb_log_warning("Detected hub over-current condition, "
		    "all ports should be powered off.");
	} else {
		/* Over-current condition is gone, it is safe to turn the
		 * ports on. */
		size_t port;
		for (port = 1; port <= hub_info->port_count; ++port) {
			const int opResult = usb_hub_port_set_feature(
			    &hub_info->ports[port], USB_HUB_FEATURE_PORT_POWER);
			if (opResult != EOK) {
				usb_log_warning(
				    "HUB OVER-CURRENT GONE: Cannot power on "
				    "port %zu;  %s\n",
				    port, str_error(opResult));
			}
		}
	}
	const int opResult = usb_request_clear_feature(
	    &hub_info->usb_device->ctrl_pipe, USB_REQUEST_TYPE_CLASS,
	    USB_REQUEST_RECIPIENT_DEVICE,
	    USB_HUB_FEATURE_C_HUB_LOCAL_POWER, 0);
	if (opResult != EOK) {
		usb_log_error(
		    "Failed to clear hub over-current change flag: %s.\n",
		    str_error(opResult));
	}
}
/*----------------------------------------------------------------------------*/
/**
 * Process hub interrupts.
 *
 * The change can be either in the over-current condition or local-power change.
 * @param hub_info hub instance
 */
static void usb_hub_global_interrupt(const usb_hub_info_t *hub_info)
{
	assert(hub_info);
	assert(hub_info->usb_device);
	usb_log_debug("Global interrupt on a hub\n");
	usb_pipe_t *control_pipe = &hub_info->usb_device->ctrl_pipe;

	usb_hub_status_t status;
	size_t rcvd_size;
	/* NOTE: We can't use standard USB GET_STATUS request, because
	 * hubs reply is 4byte instead of 2 */
	const int opResult = usb_pipe_control_read(control_pipe,
	    &get_hub_status_request, sizeof(get_hub_status_request),
	    &status, sizeof(usb_hub_status_t), &rcvd_size);
	if (opResult != EOK) {
		usb_log_error("Could not get hub status: %s\n",
		    str_error(opResult));
		return;
	}
	if (rcvd_size != sizeof(usb_hub_status_t)) {
		usb_log_error("Received status has incorrect size\n");
		return;
	}

	/* Handle status changes */
	if (status & USB_HUB_STATUS_C_OVER_CURRENT) {
		usb_hub_over_current(hub_info, status);
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
		const int opResult = usb_request_clear_feature(
		    control_pipe, USB_REQUEST_TYPE_CLASS,
		    USB_REQUEST_RECIPIENT_DEVICE,
		    USB_HUB_FEATURE_C_HUB_LOCAL_POWER, 0);
		if (opResult != EOK) {
			usb_log_error(
			    "Failed to clear hub power change flag: %s.\n",
			    str_error(opResult));
		}
	}
}
/*----------------------------------------------------------------------------*/
/**
 * callback called from hub polling fibril when the fibril terminates
 *
 * Should perform a cleanup - deletes hub_info.
 * @param device usb device afected
 * @param was_error indicates that the fibril is stoped due to an error
 * @param data pointer to usb_hub_info_t structure
 */
static void usb_hub_polling_terminated_callback(usb_device_t *device,
    bool was_error, void *data)
{
	usb_hub_info_t *hub = data;
	assert(hub);

	fibril_mutex_lock(&hub->pending_ops_mutex);

	/* The device is dead. However there might be some pending operations
	 * that we need to wait for.
	 * One of them is device adding in progress.
	 * The respective fibril is probably waiting for status change
	 * in port reset (port enable) callback.
	 * Such change would never come (otherwise we would not be here).
	 * Thus, we would flush all pending port resets.
	 */
	if (hub->pending_ops_count > 0) {
		size_t port;
		for (port = 0; port < hub->port_count; port++) {
			usb_hub_port_reset_fail(&hub->ports[port]);
		}
	}
	/* And now wait for them. */
	while (hub->pending_ops_count > 0) {
		fibril_condvar_wait(&hub->pending_ops_cv,
		    &hub->pending_ops_mutex);
	}
	fibril_mutex_unlock(&hub->pending_ops_mutex);

	usb_device_destroy(hub->usb_device);

	free(hub->ports);
	free(hub);
}
/**
 * @}
 */
