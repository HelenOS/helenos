/*
 * Copyright (c) 2010 Matus Dekanek
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

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/descriptor.h>
#include <usb/recognise.h>
#include <usb/request.h>
#include <usb/classes/hub.h>
#include <usb/devpoll.h>
#include <stdio.h>

#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"
#include "usb/usb.h"
#include "usb/pipes.h"
#include "usb/classes/classes.h"


static usb_hub_info_t * usb_hub_info_create(usb_device_t * usb_dev);

static int usb_hub_process_hub_specific_info(usb_hub_info_t * hub_info);

static int usb_hub_set_configuration(usb_hub_info_t * hub_info);

static int usb_hub_start_hub_fibril(usb_hub_info_t * hub_info);

static int usb_process_hub_over_current(usb_hub_info_t * hub_info,
    usb_hub_status_t status);

static int usb_process_hub_power_change(usb_hub_info_t * hub_info,
    usb_hub_status_t status);

static void usb_hub_process_global_interrupt(usb_hub_info_t * hub_info);


/// \TODO malloc checking

//*********************************************
//
//  hub driver code, initialization
//
//*********************************************

/**
 * Initialize hub device driver fibril
 *
 * Creates hub representation and fibril that periodically checks hub`s status.
 * Hub representation is passed to the fibril.
 * @param usb_dev generic usb device information
 * @return error code
 */
int usb_hub_add_device(usb_device_t * usb_dev) {
	if (!usb_dev) return EINVAL;
	usb_hub_info_t * hub_info = usb_hub_info_create(usb_dev);
	//create hc connection
	usb_log_debug("Initializing USB wire abstraction.\n");
	int opResult = usb_hc_connection_initialize_from_device(
	    &hub_info->connection,
	    hub_info->usb_device->ddf_dev);
	if (opResult != EOK) {
		usb_log_error("could not initialize connection to device, "
		    "errno %d\n",
		    opResult);
		free(hub_info);
		return opResult;
	}

	//usb_pipe_start_session(hub_info->control_pipe);
	//set hub configuration
	opResult = usb_hub_set_configuration(hub_info);
	if (opResult != EOK) {
		usb_log_error("could not set hub configuration, errno %d\n",
		    opResult);
		free(hub_info);
		return opResult;
	}
	//get port count and create attached_devs
	opResult = usb_hub_process_hub_specific_info(hub_info);
	if (opResult != EOK) {
		usb_log_error("could process hub specific info, errno %d\n",
		    opResult);
		free(hub_info);
		return opResult;
	}
	//usb_pipe_end_session(hub_info->control_pipe);

	/// \TODO what is this?
	usb_log_debug("Creating `hub' function.\n");
	ddf_fun_t *hub_fun = ddf_fun_create(hub_info->usb_device->ddf_dev,
	    fun_exposed, "hub");
	assert(hub_fun != NULL);
	hub_fun->ops = NULL;

	opResult = ddf_fun_bind(hub_fun);
	assert(opResult == EOK);
	opResult = ddf_fun_add_to_class(hub_fun, "hub");
	assert(opResult == EOK);

	opResult = usb_hub_start_hub_fibril(hub_info);
	if(opResult!=EOK)
		free(hub_info);
	return opResult;
}


/** Callback for polling hub for changes.
 *
 * @param dev Device where the change occured.
 * @param change_bitmap Bitmap of changed ports.
 * @param change_bitmap_size Size of the bitmap in bytes.
 * @param arg Custom argument, points to @c usb_hub_info_t.
 * @return Whether to continue polling.
 */
bool hub_port_changes_callback(usb_device_t *dev,
    uint8_t *change_bitmap, size_t change_bitmap_size, void *arg) {
	usb_log_debug("hub_port_changes_callback\n");
	usb_hub_info_t *hub = (usb_hub_info_t *) arg;

	/* FIXME: check that we received enough bytes. */
	if (change_bitmap_size == 0) {
		goto leave;
	}

	bool change;
	change = ((uint8_t*) change_bitmap)[0] & 1;
	if (change) {
		usb_hub_process_global_interrupt(hub);
	}

	size_t port;
	for (port = 1; port < hub->port_count + 1; port++) {
		bool change = (change_bitmap[port / 8] >> (port % 8)) % 2;
		if (change) {
			usb_hub_process_interrupt(hub, port);
		}
	}
leave:
	/* FIXME: proper interval. */
	async_usleep(1000 * 1000 * 10);

	return true;
}


//*********************************************
//
//  support functions
//
//*********************************************

/**
 * create usb_hub_info_t structure
 *
 * Does only basic copying of known information into new structure.
 * @param usb_dev usb device structure
 * @return basic usb_hub_info_t structure
 */
static usb_hub_info_t * usb_hub_info_create(usb_device_t * usb_dev) {
	usb_hub_info_t * result = malloc(sizeof(usb_hub_info_t));
	if (!result) return NULL;
	result->usb_device = usb_dev;
	result->status_change_pipe = usb_dev->pipes[0].pipe;
	result->control_pipe = &usb_dev->ctrl_pipe;
	result->is_default_address_used = false;
	return result;
}

/**
 * Load hub-specific information into hub_info structure and process if needed
 *
 * Particularly read port count and initialize structure holding port
 * information. If there are non-removable devices, start initializing them.
 * This function is hub-specific and should be run only after the hub is
 * configured using usb_hub_set_configuration function.
 * @param hub_info hub representation
 * @return error code
 */
static int usb_hub_process_hub_specific_info(usb_hub_info_t * hub_info) {
	// get hub descriptor
	usb_log_debug("creating serialized descriptor\n");
	//void * serialized_descriptor = malloc(USB_HUB_MAX_DESCRIPTOR_SIZE);
	uint8_t serialized_descriptor[USB_HUB_MAX_DESCRIPTOR_SIZE];
	usb_hub_descriptor_t * descriptor;
	int opResult;

	size_t received_size;
	opResult = usb_request_get_descriptor(hub_info->control_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_HUB, 0, 0, serialized_descriptor,
	    USB_HUB_MAX_DESCRIPTOR_SIZE, &received_size);

	if (opResult != EOK) {
		usb_log_error("failed when receiving hub descriptor, "
		    "badcode = %d\n",
		    opResult);
		free(serialized_descriptor);
		return opResult;
	}
	usb_log_debug2("deserializing descriptor\n");
	descriptor = usb_create_deserialized_hub_desriptor(
	    serialized_descriptor);
	if (descriptor == NULL) {
		usb_log_warning("could not deserialize descriptor \n");
		return ENOMEM;
	}
	usb_log_debug("setting port count to %d\n", descriptor->ports_count);
	hub_info->port_count = descriptor->ports_count;
	/// \TODO this is not semantically correct
	bool is_power_switched =
	    ((descriptor->hub_characteristics & 1) ==0);
	bool has_individual_port_powering =
	    ((descriptor->hub_characteristics & 1) !=0);
	hub_info->ports = malloc(
	    sizeof (usb_hub_port_t) * (hub_info->port_count + 1));
	size_t port;
	for (port = 0; port < hub_info->port_count + 1; port++) {
		usb_hub_port_init(&hub_info->ports[port]);
	}
	if(is_power_switched){
		usb_log_debug("is_power_switched\n");
		if(has_individual_port_powering){
			usb_log_debug("has_individual_port_powering\n");
			for (port = 0; port < hub_info->port_count; port++) {
				opResult = usb_hub_set_port_feature(hub_info->control_pipe,
				    port+1, USB_HUB_FEATURE_PORT_POWER);
				if (opResult != EOK) {
					usb_log_error("cannot power on port %d;  %d\n",
					    port+1, opResult);
				}
			}
		}else{
			usb_log_debug("!has_individual_port_powering\n");
			opResult = usb_hub_set_feature(hub_info->control_pipe,
			    USB_HUB_FEATURE_C_HUB_LOCAL_POWER);
			if (opResult != EOK) {
				usb_log_error("cannot power hub;  %d\n",
				  opResult);
			}
		}
	}else{
		usb_log_debug("!is_power_switched\n");
	}
	usb_log_debug2("freeing data\n");
	//free(serialized_descriptor);
	//free(descriptor->devices_removable);
	free(descriptor);
	return EOK;
}

/**
 * Set configuration of hub
 *
 * Check whether there is at least one configuration and sets the first one.
 * This function should be run prior to running any hub-specific action.
 * @param hub_info hub representation
 * @return error code
 */
static int usb_hub_set_configuration(usb_hub_info_t * hub_info) {
	//device descriptor
	usb_standard_device_descriptor_t *std_descriptor
	    = &hub_info->usb_device->descriptors.device;
	usb_log_debug("hub has %d configurations\n",
	    std_descriptor->configuration_count);
	if (std_descriptor->configuration_count < 1) {
		usb_log_error("there are no configurations available\n");
		return EINVAL;
	}

	usb_standard_configuration_descriptor_t *config_descriptor
	    = (usb_standard_configuration_descriptor_t *)
	    hub_info->usb_device->descriptors.configuration;

	/* Set configuration. */
	int opResult = usb_request_set_configuration(
	    &hub_info->usb_device->ctrl_pipe,
	    config_descriptor->configuration_number);

	if (opResult != EOK) {
		usb_log_error("Failed to set hub configuration: %s.\n",
		    str_error(opResult));
		return opResult;
	}
	usb_log_debug("\tused configuration %d\n",
	    config_descriptor->configuration_number);

	return EOK;
}

/**
 * create and start fibril with hub control loop
 *
 * Before the fibril is started, the control pipe and host controller
 * connection of the hub is open.
 *
 * @param hub_info hub representing structure
 * @return error code
 */
static int usb_hub_start_hub_fibril(usb_hub_info_t * hub_info){
	/*
	 * The processing will require opened control pipe and connection
	 * to the host controller.
	 * It is waste of resources but let's hope there will be less
	 * hubs than the phone limit.
	 * FIXME: with some proper locking over pipes and session
	 * auto destruction, this could work better.
	 */
	int rc = usb_hc_connection_open(&hub_info->connection);
	if (rc != EOK) {
		//usb_pipe_end_session(hub_info->control_pipe);
		usb_log_error("Failed to open connection to HC: %s.\n",
		    str_error(rc));
		return rc;
	}

	rc = usb_device_auto_poll(hub_info->usb_device, 0,
	    hub_port_changes_callback, ((hub_info->port_count + 1) / 8) + 1,
	    NULL, hub_info);
	if (rc != EOK) {
		usb_log_error("Failed to create polling fibril: %s.\n",
		    str_error(rc));
		free(hub_info);
		return rc;
	}

	usb_log_info("Controlling hub `%s' (%d ports).\n",
	    hub_info->usb_device->ddf_dev->name, hub_info->port_count);
	return EOK;
}

//*********************************************
//
//  change handling functions
//
//*********************************************


/**
 * process hub over current change
 *
 * This means either to power off the hub or power it on.
 * @param hub_info hub instance
 * @param status hub status bitmask
 * @return error code
 */
static int usb_process_hub_over_current(usb_hub_info_t * hub_info,
    usb_hub_status_t status) {
	int opResult;
	if (usb_hub_is_status(status,USB_HUB_FEATURE_HUB_OVER_CURRENT)){
		opResult = usb_hub_clear_feature(hub_info->control_pipe,
		    USB_HUB_FEATURE_HUB_LOCAL_POWER);
		if (opResult != EOK) {
			usb_log_error("cannot power off hub: %d\n",
			    opResult);
		}
	} else {
		opResult = usb_hub_set_feature(hub_info->control_pipe,
		    USB_HUB_FEATURE_HUB_LOCAL_POWER);
		if (opResult != EOK) {
			usb_log_error("cannot power on hub: %d\n",
			    opResult);
		}
	}
	return opResult;
}

/**
 * process hub power change
 *
 * If the power has been lost, reestablish it.
 * If it was reestablished, re-power all ports.
 * @param hub_info hub instance
 * @param status hub status bitmask
 * @return error code
 */
static int usb_process_hub_power_change(usb_hub_info_t * hub_info,
    usb_hub_status_t status) {
	int opResult;
	if (usb_hub_is_status(status,USB_HUB_FEATURE_HUB_LOCAL_POWER)) {
		//restart power on hub
		opResult = usb_hub_set_feature(hub_info->control_pipe,
		    USB_HUB_FEATURE_HUB_LOCAL_POWER);
		if (opResult != EOK) {
			usb_log_error("cannot power on hub: %d\n",
			    opResult);
		}
	} else {//power reestablished on hub- restart ports
		size_t port;
		for (port = 0; port < hub_info->port_count; ++port) {
			opResult = usb_hub_set_port_feature(
			    hub_info->control_pipe,
			    port, USB_HUB_FEATURE_PORT_POWER);
			if (opResult != EOK) {
				usb_log_error("cannot power on port %d;  %d\n",
				    port, opResult);
			}
		}
	}
	return opResult;
}

/**
 * process hub interrupts
 *
 * The change can be either in the over-current condition or
 * local-power lost condition.
 * @param hub_info hub instance
 */
static void usb_hub_process_global_interrupt(usb_hub_info_t * hub_info) {
	usb_log_debug("global interrupt on a hub\n");
	usb_pipe_t *pipe = hub_info->control_pipe;
	int opResult;

	usb_port_status_t status;
	size_t rcvd_size;
	usb_device_request_setup_packet_t request;
	//int opResult;
	usb_hub_set_hub_status_request(&request);
	//endpoint 0

	opResult = usb_pipe_control_read(
	    pipe,
	    &request, sizeof (usb_device_request_setup_packet_t),
	    &status, 4, &rcvd_size
	    );
	if (opResult != EOK) {
		usb_log_error("could not get hub status\n");
		return;
	}
	if (rcvd_size != sizeof (usb_port_status_t)) {
		usb_log_error("received status has incorrect size\n");
		return;
	}
	//port reset
	if (
	    usb_hub_is_status(status,16+USB_HUB_FEATURE_C_HUB_OVER_CURRENT)) {
		usb_process_hub_over_current(hub_info, status);
	}
	if (
	    usb_hub_is_status(status,16+USB_HUB_FEATURE_C_HUB_LOCAL_POWER)) {
		usb_process_hub_power_change(hub_info, status);
	}
}

/**
 * @}
 */
