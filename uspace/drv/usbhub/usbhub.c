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

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/descriptor.h>
#include <usb/recognise.h>
#include <usb/request.h>
#include <usb/classes/hub.h>
#include <stdio.h>

#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"
#include "usb/usb.h"
#include "usb/pipes.h"
#include "usb/classes/classes.h"

int usb_hub_control_loop(void * hub_info_param){
	usb_hub_info_t * hub_info = (usb_hub_info_t*)hub_info_param;
	int errorCode = EOK;

	while(errorCode == EOK){
		errorCode = usb_hub_check_hub_changes(hub_info);
		async_usleep(1000 * 1000 );/// \TODO proper number once
	}
	usb_log_error("something in ctrl loop went wrong, errno %d\n",errorCode);

	return 0;
}


//*********************************************
//
//  hub driver code, initialization
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
	usb_hub_info_t * result = usb_new(usb_hub_info_t);
	if(!result) return NULL;
	result->usb_device = usb_dev;
	result->status_change_pipe = usb_dev->pipes[0].pipe;
	result->control_pipe = &usb_dev->ctrl_pipe;
	result->is_default_address_used = false;
	return result;
}

/**
 * Load hub-specific information into hub_info structure.
 *
 * Particularly read port count and initialize structure holding port
 * information.
 * This function is hub-specific and should be run only after the hub is
 * configured using usb_hub_set_configuration function.
 * @param hub_info pointer to structure with usb hub data
 * @return error code
 */
static int usb_hub_get_hub_specific_info(usb_hub_info_t * hub_info){
	// get hub descriptor
	usb_log_debug("creating serialized descriptor\n");
	void * serialized_descriptor = malloc(USB_HUB_MAX_DESCRIPTOR_SIZE);
	usb_hub_descriptor_t * descriptor;

	/* this was one fix of some bug, should not be needed anymore
	int opResult = usb_request_set_configuration(&result->endpoints.control, 1);
	if(opResult!=EOK){
		usb_log_error("could not set default configuration, errno %d",opResult);
		return opResult;
	}
	 */
	size_t received_size;
	int opResult = usb_request_get_descriptor(&hub_info->usb_device->ctrl_pipe,
			USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_DEVICE,
			USB_DESCTYPE_HUB,
			0, 0, serialized_descriptor,
			USB_HUB_MAX_DESCRIPTOR_SIZE, &received_size);

	if (opResult != EOK) {
		usb_log_error("failed when receiving hub descriptor, badcode = %d\n",
				opResult);
		free(serialized_descriptor);
		return opResult;
	}
	usb_log_debug2("deserializing descriptor\n");
	descriptor = usb_deserialize_hub_desriptor(serialized_descriptor);
	if(descriptor==NULL){
		usb_log_warning("could not deserialize descriptor \n");
		return opResult;
	}
	usb_log_debug("setting port count to %d\n",descriptor->ports_count);
	hub_info->port_count = descriptor->ports_count;
	hub_info->attached_devs = (usb_hc_attached_device_t*)
	    malloc((hub_info->port_count+1) * sizeof(usb_hc_attached_device_t));
	int i;
	for(i=0;i<hub_info->port_count+1;++i){
		hub_info->attached_devs[i].handle=0;
		hub_info->attached_devs[i].address=0;
	}
	usb_log_debug2("freeing data\n");
	free(serialized_descriptor);
	free(descriptor->devices_removable);
	free(descriptor);
	return EOK;
}
/**
 * Set configuration of hub
 *
 * Check whether there is at least one configuration and sets the first one.
 * This function should be run prior to running any hub-specific action.
 * @param hub_info
 * @return
 */
static int usb_hub_set_configuration(usb_hub_info_t * hub_info){
	//device descriptor
	usb_standard_device_descriptor_t *std_descriptor
	    = &hub_info->usb_device->descriptors.device;
	usb_log_debug("hub has %d configurations\n",
	    std_descriptor->configuration_count);
	if(std_descriptor->configuration_count<1){
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
 * Initialize hub device driver fibril
 *
 * Creates hub representation and fibril that periodically checks hub`s status.
 * Hub representation is passed to the fibril.
 * @param usb_dev generic usb device information
 * @return error code
 */
int usb_hub_add_device(usb_device_t * usb_dev){
	if(!usb_dev) return EINVAL;
	usb_hub_info_t * hub_info = usb_hub_info_create(usb_dev);
	//create hc connection
	usb_log_debug("Initializing USB wire abstraction.\n");
	int opResult = usb_hc_connection_initialize_from_device(
			&hub_info->connection,
			hub_info->usb_device->ddf_dev);
	if(opResult != EOK){
		usb_log_error("could not initialize connection to device, errno %d\n",
				opResult);
		free(hub_info);
		return opResult;
	}
	
	usb_pipe_start_session(hub_info->control_pipe);
	//set hub configuration
	opResult = usb_hub_set_configuration(hub_info);
	if(opResult!=EOK){
		usb_log_error("could not set hub configuration, errno %d\n",opResult);
		free(hub_info);
		return opResult;
	}
	//get port count and create attached_devs
	opResult = usb_hub_get_hub_specific_info(hub_info);
	if(opResult!=EOK){
		usb_log_error("could not set hub configuration, errno %d\n",opResult);
		free(hub_info);
		return opResult;
	}
	usb_pipe_end_session(hub_info->control_pipe);


	/// \TODO what is this?
	usb_log_debug("Creating `hub' function.\n");
	ddf_fun_t *hub_fun = ddf_fun_create(hub_info->usb_device->ddf_dev,
			fun_exposed, "hub");
	assert(hub_fun != NULL);
	hub_fun->ops = NULL;

	int rc = ddf_fun_bind(hub_fun);
	assert(rc == EOK);
	rc = ddf_fun_add_to_class(hub_fun, "hub");
	assert(rc == EOK);

	//create fibril for the hub control loop
	fid_t fid = fibril_create(usb_hub_control_loop, hub_info);
	if (fid == 0) {
		usb_log_error("failed to start monitoring fibril for new hub.\n");
		return ENOMEM;
	}
	fibril_add_ready(fid);
	usb_log_debug("Hub fibril created.\n");

	usb_log_info("Controlling hub `%s' (%d ports).\n",
	    hub_info->usb_device->ddf_dev->name, hub_info->port_count);
	return EOK;
}


//*********************************************
//
//  hub driver code, main loop
//
//*********************************************

/**
 * release default address used by given hub
 *
 * Also unsets hub->is_default_address_used. Convenience wrapper function.
 * @note hub->connection MUST be open for communication
 * @param hub hub representation
 * @return error code
 */
static int usb_hub_release_default_address(usb_hub_info_t * hub){
	int opResult = usb_hc_release_default_address(&hub->connection);
	if(opResult!=EOK){
		usb_log_error("could not release default address, errno %d\n",opResult);
		return opResult;
	}
	hub->is_default_address_used = false;
	return EOK;
}

/**
 * Reset the port with new device and reserve the default address.
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_init_add_device(usb_hub_info_t * hub, uint16_t port,
		usb_speed_t speed) {
	//if this hub already uses default address, it cannot request it once more
	if(hub->is_default_address_used) return;
	usb_log_debug("some connection changed\n");
	assert(hub->control_pipe->hc_phone);
	int opResult = usb_hub_clear_port_feature(hub->control_pipe,
				port, USB_HUB_FEATURE_C_PORT_CONNECTION);
	if(opResult != EOK){
		usb_log_warning("could not clear port-change-connection flag\n");
	}
	usb_device_request_setup_packet_t request;
	
	//get default address
	opResult = usb_hc_reserve_default_address(&hub->connection, speed);
	
	if (opResult != EOK) {
		usb_log_warning("cannot assign default address, it is probably used %d\n",
				opResult);
		return;
	}
	hub->is_default_address_used = true;
	//reset port
	usb_hub_set_reset_port_request(&request, port);
	opResult = usb_pipe_control_write(
			hub->control_pipe,
			&request,sizeof(usb_device_request_setup_packet_t),
			NULL, 0
			);
	if (opResult != EOK) {
		usb_log_error("something went wrong when reseting a port %d\n",opResult);
		//usb_hub_release_default_address(hc);
		usb_hub_release_default_address(hub);
	}
	return;
}

/**
 * Finalize adding new device after port reset
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_finalize_add_device( usb_hub_info_t * hub,
		uint16_t port, usb_speed_t speed) {

	int opResult;
	usb_log_debug("finalizing add device\n");
	opResult = usb_hub_clear_port_feature(hub->control_pipe,
	    port, USB_HUB_FEATURE_C_PORT_RESET);

	if (opResult != EOK) {
		usb_log_error("failed to clear port reset feature\n");
		usb_hub_release_default_address(hub);
		return;
	}
	//create connection to device
	usb_pipe_t new_device_pipe;
	usb_device_connection_t new_device_connection;
	usb_device_connection_initialize_on_default_address(
			&new_device_connection,
			&hub->connection
			);
	usb_pipe_initialize_default_control(
			&new_device_pipe,
			&new_device_connection);
	usb_pipe_probe_default_control(&new_device_pipe);

	/* Request address from host controller. */
	usb_address_t new_device_address = usb_hc_request_address(
			&hub->connection,
			speed
			);
	if (new_device_address < 0) {
		usb_log_error("failed to get free USB address\n");
		opResult = new_device_address;
		usb_hub_release_default_address(hub);
		return;
	}
	usb_log_debug("setting new address %d\n",new_device_address);
	//opResult = usb_drv_req_set_address(hc, USB_ADDRESS_DEFAULT,
	//    new_device_address);
	usb_pipe_start_session(&new_device_pipe);
	opResult = usb_request_set_address(&new_device_pipe,new_device_address);
	usb_pipe_end_session(&new_device_pipe);
	if (opResult != EOK) {
		usb_log_error("could not set address for new device %d\n",opResult);
		usb_hub_release_default_address(hub);
		return;
	}


	//opResult = usb_hub_release_default_address(hc);
	opResult = usb_hub_release_default_address(hub);
	if(opResult!=EOK){
		return;
	}

	devman_handle_t child_handle;
	//??
    opResult = usb_device_register_child_in_devman(new_device_address,
            hub->connection.hc_handle, hub->usb_device->ddf_dev, &child_handle,
            NULL, NULL, NULL);

	if (opResult != EOK) {
		usb_log_error("could not start driver for new device %d\n",opResult);
		return;
	}
	hub->attached_devs[port].handle = child_handle;
	hub->attached_devs[port].address = new_device_address;

	//opResult = usb_drv_bind_address(hc, new_device_address, child_handle);
	opResult = usb_hc_register_device(
			&hub->connection,
			&hub->attached_devs[port]);
	if (opResult != EOK) {
		usb_log_error("could not assign address of device in hcd %d\n",opResult);
		return;
	}
	usb_log_info("Detected new device on `%s' (port %d), " \
	    "address %d (handle %llu).\n",
	    hub->usb_device->ddf_dev->name, (int) port,
	    new_device_address, child_handle);
}

/**
 * Unregister device address in hc
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_removed_device(
    usb_hub_info_t * hub,uint16_t port) {

	int opResult = usb_hub_clear_port_feature(hub->control_pipe,
				port, USB_HUB_FEATURE_C_PORT_CONNECTION);
	if(opResult != EOK){
		usb_log_warning("could not clear port-change-connection flag\n");
	}
	/** \TODO remove device from device manager - not yet implemented in
	 * devide manager
	 */
	
	//close address
	if(hub->attached_devs[port].address!=0){
		/*uncomment this code to use it when DDF allows device removal
		opResult = usb_hc_unregister_device(
				&hub->connection, hub->attached_devs[port].address);
		if(opResult != EOK) {
			dprintf(USB_LOG_LEVEL_WARNING, "could not release address of " \
			    "removed device: %d", opResult);
		}
		hub->attached_devs[port].address = 0;
		hub->attached_devs[port].handle = 0;
		 */
	}else{
		usb_log_warning("this is strange, disconnected device had no address\n");
		//device was disconnected before it`s port was reset - return default address
		usb_hub_release_default_address(hub);
	}
}


/**
 * Process over current condition on port.
 * 
 * Turn off the power on the port.
 *
 * @param hub
 * @param port
 */
static void usb_hub_over_current( usb_hub_info_t * hub,
		uint16_t port){
	int opResult;
	opResult = usb_hub_clear_port_feature(hub->control_pipe,
	    port, USB_HUB_FEATURE_PORT_POWER);
	if(opResult!=EOK){
		usb_log_error("cannot power off port %d;  %d\n",
				port, opResult);
	}
}

/**
 * Process interrupts on given hub port
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_process_interrupt(usb_hub_info_t * hub, 
        uint16_t port) {
	usb_log_debug("interrupt at port %d\n", port);
	//determine type of change
	usb_pipe_t *pipe = hub->control_pipe;
	
	int opResult;

	usb_port_status_t status;
	size_t rcvd_size;
	usb_device_request_setup_packet_t request;
	//int opResult;
	usb_hub_set_port_status_request(&request, port);
	//endpoint 0

	opResult = usb_pipe_control_read(
			pipe,
			&request, sizeof(usb_device_request_setup_packet_t),
			&status, 4, &rcvd_size
			);
	if (opResult != EOK) {
		usb_log_error("could not get port status\n");
		return;
	}
	if (rcvd_size != sizeof (usb_port_status_t)) {
		usb_log_error("received status has incorrect size\n");
		return;
	}
	//something connected/disconnected
	if (usb_port_connect_change(&status)) {
		if (usb_port_dev_connected(&status)) {
			usb_log_debug("some connection changed\n");
			usb_hub_init_add_device(hub, port, usb_port_speed(&status));
		} else {
			usb_hub_removed_device(hub, port);
		}
	}
	//over current
	if (usb_port_overcurrent_change(&status)) {
		//check if it was not auto-resolved
		if(usb_port_over_current(&status)){
			usb_hub_over_current(hub,port);
		}else{
			usb_log_debug("over current condition was auto-resolved on port %d\n",
					port);
		}
	}
	//port reset
	if (usb_port_reset_completed(&status)) {
		usb_log_debug("port reset complete\n");
		if (usb_port_enabled(&status)) {
			usb_hub_finalize_add_device(hub, port, usb_port_speed(&status));
		} else {
			usb_log_warning("port reset, but port still not enabled\n");
		}
	}

	usb_port_set_connect_change(&status, false);
	usb_port_set_reset(&status, false);
	usb_port_set_reset_completed(&status, false);
	usb_port_set_dev_connected(&status, false);
	if (status>>16) {
		usb_log_info("there was some unsupported change on port %d: %X\n",
				port,status);

	}
}

/**
 * Check changes on particular hub
 * @param hub_info_param pointer to usb_hub_info_t structure
 * @return error code if there is problem when initializing communication with
 * hub, EOK otherwise
 */
int usb_hub_check_hub_changes(usb_hub_info_t * hub_info){
	int opResult;
	opResult = usb_pipe_start_session(
			hub_info->status_change_pipe);
	if(opResult != EOK){
		usb_log_error("could not initialize communication for hub; %d\n",
				opResult);
		return opResult;
	}

	size_t port_count = hub_info->port_count;

	/// FIXME: count properly
	size_t byte_length = ((port_count+1) / 8) + 1;
		void *change_bitmap = malloc(byte_length);
	size_t actual_size;

	/*
	 * Send the request.
	 */
	opResult = usb_pipe_read(
			hub_info->status_change_pipe,
			change_bitmap, byte_length, &actual_size
			);

	if (opResult != EOK) {
		free(change_bitmap);
		usb_log_warning("something went wrong while getting status of hub\n");
		usb_pipe_end_session(hub_info->status_change_pipe);
		return opResult;
	}
	unsigned int port;
	opResult = usb_pipe_start_session(hub_info->control_pipe);
	if(opResult!=EOK){
		usb_log_error("could not start control pipe session %d\n", opResult);
		usb_pipe_end_session(hub_info->status_change_pipe);
		return opResult;
	}
	opResult = usb_hc_connection_open(&hub_info->connection);
	if(opResult!=EOK){
		usb_log_error("could not start host controller session %d\n",
				opResult);
		usb_pipe_end_session(hub_info->control_pipe);
		usb_pipe_end_session(hub_info->status_change_pipe);
		return opResult;
	}

	///todo, opresult check, pre obe konekce
	for (port = 1; port < port_count+1; ++port) {
		bool interrupt =
				(((uint8_t*) change_bitmap)[port / 8] >> (port % 8)) % 2;
		if (interrupt) {
			usb_hub_process_interrupt(
			        hub_info, port);
		}
	}
	usb_hc_connection_close(&hub_info->connection);
	usb_pipe_end_session(hub_info->control_pipe);
	usb_pipe_end_session(hub_info->status_change_pipe);
	free(change_bitmap);
	return EOK;
}



/**
 * @}
 */
