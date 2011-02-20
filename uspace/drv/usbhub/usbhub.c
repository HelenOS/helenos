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

#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/usbdrv.h>
#include <usb/descriptor.h>
#include <usb/recognise.h>
#include <usb/devreq.h>
#include <usb/request.h>
#include <usb/classes/hub.h>

#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"
#include "usb/usb.h"
#include "usb/pipes.h"

static device_ops_t hub_device_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface_hub_impl
};

//*********************************************
//
//  hub driver code, initialization
//
//*********************************************

usb_hub_info_t * usb_create_hub_info(device_t * device) {
	usb_hub_info_t* result = usb_new(usb_hub_info_t);
	usb_device_connection_initialize_from_device(&result->device_connection,
			device);
	usb_hc_connection_initialize_from_device(&result->connection,
			device);
	usb_endpoint_pipe_initialize_default_control(&result->endpoints.control,
            &result->device_connection);
	

	//result->device = device;
	result->port_count = -1;
	/// \TODO is this correct? is the device stored?
	result->device = device;

	//result->usb_device = usb_new(usb_hcd_attached_device_info_t);
	
	// get hub descriptor

	dprintf(USB_LOG_LEVEL_DEBUG, "creating serialized descripton");
	void * serialized_descriptor = malloc(USB_HUB_MAX_DESCRIPTOR_SIZE);
	usb_hub_descriptor_t * descriptor;
	size_t received_size;
	int opResult;
	dprintf(USB_LOG_LEVEL_DEBUG, "starting control transaction");
	usb_endpoint_pipe_start_session(&result->endpoints.control);
	opResult = usb_request_get_descriptor(&result->endpoints.control,
			USB_REQUEST_TYPE_CLASS,
			USB_DESCTYPE_HUB, 0, 0, serialized_descriptor,
			USB_HUB_MAX_DESCRIPTOR_SIZE, &received_size);
	usb_endpoint_pipe_end_session(&result->endpoints.control);

	/* Initialize the interrupt endpoint. 
	usb_endpoint_pipe_initalize(
		&hub_data->endpoints->status_change,
		&endpiont_descriptor, &hub_data->connection);

	 */ /// \TODO add this call

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "failed when receiving hub descriptor, badcode = %d",opResult);
		free(serialized_descriptor);
		return result;
	}
	dprintf(USB_LOG_LEVEL_DEBUG2, "deserializing descriptor");
	descriptor = usb_deserialize_hub_desriptor(serialized_descriptor);
	if(descriptor==NULL){
		dprintf(USB_LOG_LEVEL_WARNING, "could not deserialize descriptor ");
		result->port_count = 1;///\TODO this code is only for debug!!!
		return result;
	}
	dprintf(USB_LOG_LEVEL_INFO, "setting port count to %d",descriptor->ports_count);
	result->port_count = descriptor->ports_count;
	result->attached_devs = (usb_hc_attached_device_t*)
	    malloc((result->port_count+1) * sizeof(usb_hc_attached_device_t));
	int i;
	for(i=0;i<result->port_count+1;++i){
		result->attached_devs[i].handle=0;
		result->attached_devs[i].address=0;
	}
	dprintf(USB_LOG_LEVEL_DEBUG2, "freeing data");
	free(serialized_descriptor);
	free(descriptor->devices_removable);
	free(descriptor);

	//finish

	dprintf(USB_LOG_LEVEL_INFO, "hub info created");

	return result;
}

int usb_add_hub_device(device_t *dev) {
	dprintf(USB_LOG_LEVEL_INFO, "add_hub_device(handle=%d)", (int) dev->handle);

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */
	dev->ops = &hub_device_ops;


	usb_hub_info_t * hub_info = usb_create_hub_info(dev);
	usb_endpoint_pipe_start_session(&hub_info->endpoints.control);

	int port;
	int opResult;
	//usb_target_t target;
	//target.address = hub_info->usb_device->address;
	//target.endpoint = 0;

	//get configuration descriptor
	// this is not fully correct - there are more configurations
	// and all should be checked
	usb_standard_device_descriptor_t std_descriptor;
	opResult = usb_request_get_device_descriptor(&hub_info->endpoints.control,
	    &std_descriptor);
	if(opResult!=EOK){
		dprintf(USB_LOG_LEVEL_ERROR, "could not get device descriptor, %d",opResult);
		return opResult;
	}
	dprintf(USB_LOG_LEVEL_INFO, "hub has %d configurations",std_descriptor.configuration_count);
	if(std_descriptor.configuration_count<1){
		dprintf(USB_LOG_LEVEL_ERROR, "THERE ARE NO CONFIGURATIONS AVAILABLE");
		//shouldn`t I return?
	}
	/// \TODO check other configurations
	usb_standard_configuration_descriptor_t config_descriptor;
	opResult = usb_request_get_bare_configuration_descriptor(
	    &hub_info->endpoints.control, 0,
        &config_descriptor);
	if(opResult!=EOK){
		dprintf(USB_LOG_LEVEL_ERROR, "could not get configuration descriptor, %d",opResult);
		return opResult;
	}
	//set configuration
	opResult = usb_request_set_configuration(&hub_info->endpoints.control,
    config_descriptor.configuration_number);

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "something went wrong when setting hub`s configuration, %d", opResult);
	}

	usb_device_request_setup_packet_t request;
	for (port = 1; port < hub_info->port_count+1; ++port) {
		usb_hub_set_power_port_request(&request, port);
		opResult = usb_endpoint_pipe_control_write(&hub_info->endpoints.control,
				&request,sizeof(usb_device_request_setup_packet_t), NULL, 0);
		dprintf(USB_LOG_LEVEL_INFO, "powering port %d",port);
		if (opResult != EOK) {
			dprintf(USB_LOG_LEVEL_WARNING, "something went wrong when setting hub`s %dth port", port);
		}
	}
	//ports powered, hub seems to be enabled

	usb_endpoint_pipe_end_session(&hub_info->endpoints.control);
	//async_hangup(hc);

	//add the hub to list
	fibril_mutex_lock(&usb_hub_list_lock);
	usb_lst_append(&usb_hub_list, hub_info);
	fibril_mutex_unlock(&usb_hub_list_lock);

	dprintf(USB_LOG_LEVEL_DEBUG, "hub info added to list");
	//(void)hub_info;
	usb_hub_check_hub_changes();

	

	dprintf(USB_LOG_LEVEL_INFO, "hub dev added");
	//address is lost...
	dprintf(USB_LOG_LEVEL_DEBUG, "\taddress %d, has %d ports ",
			//hub_info->endpoints.control.,
			hub_info->port_count);
	dprintf(USB_LOG_LEVEL_DEBUG, "\tused configuration %d",config_descriptor.configuration_number);

	return EOK;
	//return ENOTSUP;
}


//*********************************************
//
//  hub driver code, main loop
//
//*********************************************

/**
 * Convenience function for releasing default address and writing debug info
 * (these few lines are used too often to be written again and again).
 * @param hc
 * @return
 */
inline static int usb_hub_release_default_address(int hc){
	int opResult;
	dprintf(USB_LOG_LEVEL_INFO, "releasing default address");
	opResult = usb_drv_release_default_address(hc);
	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_WARNING, "failed to release default address");
	}
	return opResult;
}

/**
 * Reset the port with new device and reserve the default address.
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_init_add_device(usb_hub_info_t * hub, uint16_t port) {
	usb_device_request_setup_packet_t request;
	int opResult;
	dprintf(USB_LOG_LEVEL_INFO, "some connection changed");
	assert(hub->endpoints.control.hc_phone);
	//get default address
	//opResult = usb_drv_reserve_default_address(hc);
	opResult = usb_hc_reserve_default_address(&hub->connection, USB_SPEED_LOW);

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_WARNING, "cannot assign default address, it is probably used");
		return;
	}
	//reset port
	usb_hub_set_reset_port_request(&request, port);
	opResult = usb_endpoint_pipe_control_write(
			&hub->endpoints.control,
			&request,sizeof(usb_device_request_setup_packet_t),
			NULL, 0
			);
	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "something went wrong when reseting a port");
		//usb_hub_release_default_address(hc);
		usb_hc_release_default_address(&hub->connection);
	}
}

/**
 * Finalize adding new device after port reset
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_finalize_add_device( usb_hub_info_t * hub,
		uint16_t port) {

	int opResult;
	dprintf(USB_LOG_LEVEL_INFO, "finalizing add device");
	opResult = usb_hub_clear_port_feature(&hub->endpoints.control,
	    port, USB_HUB_FEATURE_C_PORT_RESET);

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "failed to clear port reset feature");
		usb_hc_release_default_address(&hub->connection);
		return;
	}
	//create connection to device
	usb_endpoint_pipe_t new_device_pipe;
	usb_device_connection_t new_device_connection;
	usb_device_connection_initialize_on_default_address(
			&new_device_connection,
			&hub->connection
			);
	usb_endpoint_pipe_initialize_default_control(
			&new_device_pipe,
			&new_device_connection);
	/// \TODO get highspeed info





	/* Request address from host controller. */
	usb_address_t new_device_address = usb_hc_request_address(
			&hub->connection,
			USB_SPEED_LOW/// \TODO fullspeed??
			);
	if (new_device_address < 0) {
		dprintf(USB_LOG_LEVEL_ERROR, "failed to get free USB address");
		opResult = new_device_address;
		usb_hc_release_default_address(&hub->connection);
		return;
	}
	dprintf(USB_LOG_LEVEL_INFO, "setting new address %d",new_device_address);
	//opResult = usb_drv_req_set_address(hc, USB_ADDRESS_DEFAULT,
	//    new_device_address);
	opResult = usb_request_set_address(&new_device_pipe,new_device_address);

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "could not set address for new device");
		usb_hc_release_default_address(&hub->connection);
		return;
	}


	//opResult = usb_hub_release_default_address(hc);
	opResult = usb_hc_release_default_address(&hub->connection);
	if(opResult!=EOK){
		return;
	}

	devman_handle_t child_handle;
	//??
    opResult = usb_device_register_child_in_devman(new_device_address,
            hub->connection.hc_handle, hub->device, &child_handle);

	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "could not start driver for new device");
		return;
	}
	hub->attached_devs[port].handle = child_handle;
	hub->attached_devs[port].address = new_device_address;

	//opResult = usb_drv_bind_address(hc, new_device_address, child_handle);
	opResult = usb_hc_register_device(
			&hub->connection,
			&hub->attached_devs[port]);
	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "could not assign address of device in hcd");
		return;
	}
	dprintf(USB_LOG_LEVEL_INFO, "new device address %d, handle %zu",
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
	//usb_device_request_setup_packet_t request;
	int opResult;
	
	/** \TODO remove device from device manager - not yet implemented in
	 * devide manager
	 */
	
	//close address
	if(hub->attached_devs[port].address!=0){
		//opResult = usb_drv_release_address(hc,hub->attached_devs[port].address);
		opResult = usb_hc_unregister_device(
				&hub->connection, hub->attached_devs[port].address);
		if(opResult != EOK) {
			dprintf(USB_LOG_LEVEL_WARNING, "could not release address of " \
			    "removed device: %d", opResult);
		}
		hub->attached_devs[port].address = 0;
		hub->attached_devs[port].handle = 0;
	}else{
		dprintf(USB_LOG_LEVEL_WARNING, "this is strange, disconnected device had no address");
		//device was disconnected before it`s port was reset - return default address
		//usb_drv_release_default_address(hc);
		usb_hc_release_default_address(&hub->connection);
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
	dprintf(USB_LOG_LEVEL_DEBUG, "interrupt at port %d", port);
	//determine type of change
	usb_endpoint_pipe_t *pipe = &hub->endpoints.control;
	int opResult = usb_endpoint_pipe_start_session(pipe);
	
	if(opResult != EOK){
		dprintf(USB_LOG_LEVEL_ERROR, "cannot open pipe %d", opResult);
	}

	/*
	usb_target_t target;
	target.address=address;
	target.endpoint=0;
	*/

	usb_port_status_t status;
	size_t rcvd_size;
	usb_device_request_setup_packet_t request;
	//int opResult;
	usb_hub_set_port_status_request(&request, port);
	//endpoint 0

	opResult = usb_endpoint_pipe_control_read(
			pipe,
			&request, sizeof(usb_device_request_setup_packet_t),
			&status, 4, &rcvd_size
			);
	if (opResult != EOK) {
		dprintf(USB_LOG_LEVEL_ERROR, "ERROR: could not get port status");
		return;
	}
	if (rcvd_size != sizeof (usb_port_status_t)) {
		dprintf(USB_LOG_LEVEL_ERROR, "ERROR: received status has incorrect size");
		return;
	}
	//something connected/disconnected
	if (usb_port_connect_change(&status)) {
		opResult = usb_hub_clear_port_feature(pipe,
		    port, USB_HUB_FEATURE_C_PORT_CONNECTION);
		// TODO: check opResult
		if (usb_port_dev_connected(&status)) {
			dprintf(USB_LOG_LEVEL_INFO, "some connection changed");
			usb_hub_init_add_device(hub, port);
		} else {
			usb_hub_removed_device(hub, port);
		}
	}
	//port reset
	if (usb_port_reset_completed(&status)) {
		dprintf(USB_LOG_LEVEL_INFO, "port reset complete");
		if (usb_port_enabled(&status)) {
			usb_hub_finalize_add_device(hub, port);
		} else {
			dprintf(USB_LOG_LEVEL_WARNING, "ERROR: port reset, but port still not enabled");
		}
	}

	usb_port_set_connect_change(&status, false);
	usb_port_set_reset(&status, false);
	usb_port_set_reset_completed(&status, false);
	usb_port_set_dev_connected(&status, false);
	if (status>>16) {
		dprintf(USB_LOG_LEVEL_INFO, "there was some unsupported change on port %d: %X",port,status);

	}
	/// \TODO handle other changes
	/// \TODO debug log for various situations
	usb_endpoint_pipe_end_session(pipe);


}

/**
 * Check changes on all known hubs.
 */
void usb_hub_check_hub_changes(void) {
	/*
	 * Iterate through all hubs.
	 */
	usb_general_list_t * lst_item;
	fibril_mutex_lock(&usb_hub_list_lock);
	for (lst_item = usb_hub_list.next;
			lst_item != &usb_hub_list;
			lst_item = lst_item->next) {
		fibril_mutex_unlock(&usb_hub_list_lock);
		usb_hub_info_t * hub_info = ((usb_hub_info_t*)lst_item->data);
		int opResult;

		opResult = usb_endpoint_pipe_start_session(&hub_info->endpoints.status_change);
		if(opResult != EOK){
			continue;
		}
		/*
		 * Check status change pipe of this hub.
		 */
		/*
		usb_target_t target;
		target.address = hub_info->address;
		target.endpoint = 1;/// \TODO get from endpoint descriptor
		dprintf(USB_LOG_LEVEL_INFO, "checking changes for hub at addr %d",
		    target.address);
		*/
		size_t port_count = hub_info->port_count;

		/*
		 * Connect to respective HC.
		 *
		int hc = usb_drv_hc_connect_auto(hub_info->device, 0);
		if (hc < 0) {
			continue;
		}*/

		/// FIXME: count properly
		size_t byte_length = ((port_count+1) / 8) + 1;

		void *change_bitmap = malloc(byte_length);
		size_t actual_size;
		//usb_handle_t handle;

		/*
		 * Send the request.
		 */
		opResult = usb_endpoint_pipe_read(
				&hub_info->endpoints.status_change,
				change_bitmap, byte_length, &actual_size
				);

		//usb_drv_async_wait_for(handle);

		if (opResult != EOK) {
			free(change_bitmap);
			dprintf(USB_LOG_LEVEL_WARNING, "something went wrong while getting status of hub");
			continue;
		}
		unsigned int port;
		for (port = 1; port < port_count+1; ++port) {
			bool interrupt =
					(((uint8_t*) change_bitmap)[port / 8] >> (port % 8)) % 2;
			if (interrupt) {
				usb_hub_process_interrupt(
				        hub_info, port);
			}
		}
		usb_endpoint_pipe_end_session(&hub_info->endpoints.status_change);
		free(change_bitmap);
		

		//async_hangup(hc);
		fibril_mutex_lock(&usb_hub_list_lock);
	}
	fibril_mutex_unlock(&usb_hub_list_lock);
}





/**
 * @}
 */
