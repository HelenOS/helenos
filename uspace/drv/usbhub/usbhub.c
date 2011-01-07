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
/** @addtogroup usb hub driver
 * @{
 */
/** @file
 * @brief usb hub main functionality
 */

#include <driver.h>
#include <bool.h>
#include <errno.h>

#include <usb_iface.h>
#include <usb/usbdrv.h>
#include <usb/descriptor.h>
#include <usb/devreq.h>
#include <usb/classes/hub.h>

#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"

static usb_iface_t hub_usb_iface = {
	.get_hc_handle = usb_drv_find_hc
};

static device_ops_t hub_device_ops = {
	.interfaces[USB_DEV_IFACE] = &hub_usb_iface
};

//*********************************************
//
//  hub driver code, initialization
//
//*********************************************

usb_hub_info_t * usb_create_hub_info(device_t * device, int hc) {
	usb_hub_info_t* result = usb_new(usb_hub_info_t);
	//result->device = device;
	result->port_count = -1;
	/// \TODO is this correct? is the device stored?
	result->device = device;


	//printf("[usb_hub] phone to hc = %d\n", hc);
	if (hc < 0) {
		return result;
	}
	//get some hub info
	usb_address_t addr = usb_drv_get_my_address(hc, device);
	dprintf(1,"[usb_hub] addres of newly created hub = %d", addr);
	/*if(addr<0){
		//return result;

	}*/

	result->usb_device = usb_new(usb_hcd_attached_device_info_t);
	result->usb_device->address = addr;

	// get hub descriptor
	usb_target_t target;
	target.address = addr;
	target.endpoint = 0;
	usb_device_request_setup_packet_t request;
	//printf("[usb_hub] creating descriptor request\n");
	usb_hub_set_descriptor_request(&request);

	//printf("[usb_hub] creating serialized descriptor\n");
	void * serialized_descriptor = malloc(USB_HUB_MAX_DESCRIPTOR_SIZE);
	usb_hub_descriptor_t * descriptor;
	size_t received_size;
	int opResult;
	//printf("[usb_hub] starting control transaction\n");
	opResult = usb_drv_sync_control_read(
			hc, target, &request, serialized_descriptor,
			USB_HUB_MAX_DESCRIPTOR_SIZE, &received_size);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] failed when receiving hub descriptor, badcode = %d",opResult);
		free(serialized_descriptor);
		return result;
	}
	//printf("[usb_hub] deserializing descriptor\n");
	descriptor = usb_deserialize_hub_desriptor(serialized_descriptor);
	if(descriptor==NULL){
		dprintf(1,"[usb_hub] could not deserialize descriptor ");
		result->port_count = 1;///\TODO this code is only for debug!!!
		return result;
	}
	//printf("[usb_hub] setting port count to %d\n",descriptor->ports_count);
	result->port_count = descriptor->ports_count;
	result->attached_devs = (usb_hub_attached_device_t*)
	    malloc((result->port_count+1) * sizeof(usb_hub_attached_device_t));
	int i;
	for(i=0;i<result->port_count+1;++i){
		result->attached_devs[i].devman_handle=0;
		result->attached_devs[i].address=0;
	}
	//printf("[usb_hub] freeing data\n");
	free(serialized_descriptor);
	free(descriptor->devices_removable);
	free(descriptor);

	//finish

	dprintf(1,"[usb_hub] hub info created");

	return result;
}

int usb_add_hub_device(device_t *dev) {
	dprintf(1, "add_hub_device(handle=%d)", (int) dev->handle);
	dprintf(1,"[usb_hub] hub device");

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */
	dev->ops = &hub_device_ops;

	//create the hub structure
	//get hc connection
	int hc = usb_drv_hc_connect_auto(dev, 0);
	if (hc < 0) {
		return hc;
	}

	usb_hub_info_t * hub_info = usb_create_hub_info(dev, hc);
	int port;
	int opResult;
	usb_device_request_setup_packet_t request;
	usb_target_t target;
	target.address = hub_info->usb_device->address;
	target.endpoint = 0;

	//get configuration descriptor
	// this is not fully correct - there are more configurations
	// and all should be checked
	usb_standard_device_descriptor_t std_descriptor;
	opResult = usb_drv_req_get_device_descriptor(hc, target.address,
    &std_descriptor);
	if(opResult!=EOK){
		dprintf(1,"[usb_hub] could not get device descriptor, %d",opResult);
		return opResult;
	}
	dprintf(1,"[usb_hub] hub has %d configurations",std_descriptor.configuration_count);
	if(std_descriptor.configuration_count<1){
		dprintf(1,"[usb_hub] THERE ARE NO CONFIGURATIONS AVAILABLE");
	}
	usb_standard_configuration_descriptor_t config_descriptor;
	opResult = usb_drv_req_get_bare_configuration_descriptor(hc,
        target.address, 0,
        &config_descriptor);
	if(opResult!=EOK){
		dprintf(1,"[usb_hub] could not get configuration descriptor, %d",opResult);
		return opResult;
	}
	//set configuration
	request.request_type = 0;
	request.request = USB_DEVREQ_SET_CONFIGURATION;
	request.index=0;
	request.length=0;
	request.value_high=0;
	request.value_low = config_descriptor.configuration_number;
	opResult = usb_drv_sync_control_write(hc, target, &request, NULL, 0);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub]something went wrong when setting hub`s configuration, %d", opResult);
	}


	for (port = 1; port < hub_info->port_count+1; ++port) {
		usb_hub_set_power_port_request(&request, port);
		opResult = usb_drv_sync_control_write(hc, target, &request, NULL, 0);
		dprintf(1,"[usb_hub] powering port %d",port);
		if (opResult != EOK) {
			dprintf(1,"[usb_hub]something went wrong when setting hub`s %dth port", port);
		}
	}
	//ports powered, hub seems to be enabled


	ipc_hangup(hc);

	//add the hub to list
	usb_lst_append(&usb_hub_list, hub_info);
	dprintf(1,"[usb_hub] hub info added to list");
	//(void)hub_info;
	usb_hub_check_hub_changes();

	

	dprintf(1,"[usb_hub] hub dev added");
	dprintf(1,"\taddress %d, has %d ports ",
			hub_info->usb_device->address,
			hub_info->port_count);
	dprintf(1,"\tused configuration %d",config_descriptor.configuration_number);

	return EOK;
	//return ENOTSUP;
}



//*********************************************
//
//  hub driver code, main loop
//
//*********************************************

/**
 * reset the port with new device and reserve the default address
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_init_add_device(int hc, uint16_t port, usb_target_t target) {
	usb_device_request_setup_packet_t request;
	int opResult;
	dprintf(1,"[usb_hub] some connection changed");
	//get default address
	opResult = usb_drv_reserve_default_address(hc);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] cannot assign default address, it is probably used");
		return;
	}
	//reset port
	usb_hub_set_reset_port_request(&request, port);
	opResult = usb_drv_sync_control_write(
			hc, target,
			&request,
			NULL, 0
			);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] something went wrong when reseting a port");
	}
}

/**
 * convenience function for releasing default address and writing debug info
 * (these few lines are used too often to be written again and again)
 * @param hc
 * @return
 */
inline static int usb_hub_release_default_address(int hc){
	int opResult;
	dprintf(1,"[usb_hub] releasing default address");
	opResult = usb_drv_release_default_address(hc);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] failed to release default address");
	}
	return opResult;
}


/**
 * finalize adding new device after port reset
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_finalize_add_device( usb_hub_info_t * hub,
		int hc, uint16_t port, usb_target_t target) {

	int opResult;
	dprintf(1,"[usb_hub] finalizing add device");
	opResult = usb_hub_clear_port_feature(hc, target.address,
	    port, USB_HUB_FEATURE_C_PORT_RESET);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] failed to clear port reset feature");
		usb_hub_release_default_address(hc);
		return;
	}

	/* Request address at from host controller. */
	usb_address_t new_device_address = usb_drv_request_address(hc);
	if (new_device_address < 0) {
		dprintf(1,"[usb_hub] failed to get free USB address");
		opResult = new_device_address;
		usb_hub_release_default_address(hc);
		return;
	}
	dprintf(1,"[usb_hub] setting new address");
	opResult = usb_drv_req_set_address(hc, USB_ADDRESS_DEFAULT,
	    new_device_address);

	if (opResult != EOK) {
		dprintf(1,"[usb_hub] could not set address for new device");
		usb_hub_release_default_address(hc);
		return;
	}


	opResult = usb_hub_release_default_address(hc);
	if(opResult!=EOK){
		return;
	}

	devman_handle_t child_handle;
	opResult = usb_drv_register_child_in_devman(hc, hub->device,
            new_device_address, &child_handle);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] could not start driver for new device");
		return;
	}
	hub->attached_devs[port].devman_handle = child_handle;
	hub->attached_devs[port].address = new_device_address;

	opResult = usb_drv_bind_address(hc, new_device_address, child_handle);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] could not assign address of device in hcd");
		return;
	}
	dprintf(1,"[usb_hub] new device address %d, handle %zu",
	    new_device_address, child_handle);

}

/**
 * unregister device address in hc, close the port
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_removed_device(
    usb_hub_info_t * hub, int hc, uint16_t port, usb_target_t target) {
	//usb_device_request_setup_packet_t request;
	int opResult;
	//disable port
	/*usb_hub_set_disable_port_request(&request, port);
	opResult = usb_drv_sync_control_write(
			hc, target,
			&request,
			NULL, 0
			);
	if (opResult != EOK) {
		//continue;
		printf("[usb_hub] something went wrong when disabling a port\n");
	}*/
	/// \TODO remove device

	hub->attached_devs[port].devman_handle=0;
	//close address
	if(hub->attached_devs[port].address!=0){
		opResult = usb_drv_release_address(hc,hub->attached_devs[port].address);
		if(opResult != EOK) {
			dprintf(1,
					"[usb_hub] could not release address of removed device: %d"
					,opResult);
		}
		hub->attached_devs[port].address = 0;
	}else{
		dprintf(1,
				"[usb_hub] this is strange, disconnected device had no address");
		//device was disconnected before it`s port was reset - return default address
		usb_drv_release_default_address(hc);
	}
}

/**
 * process interrupts on given hub port
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_process_interrupt(usb_hub_info_t * hub, int hc,
        uint16_t port, usb_address_t address) {
	dprintf(1,"[usb_hub] interrupt at port %d", port);
	//determine type of change
	usb_target_t target;
	target.address=address;
	target.endpoint=0;
	usb_port_status_t status;
	size_t rcvd_size;
	usb_device_request_setup_packet_t request;
	int opResult;
	usb_hub_set_port_status_request(&request, port);
	//endpoint 0

	opResult = usb_drv_sync_control_read(
			hc, target,
			&request,
			&status, 4, &rcvd_size
			);
	if (opResult != EOK) {
		dprintf(1,"[usb_hub] ERROR: could not get port status");
		return;
	}
	if (rcvd_size != sizeof (usb_port_status_t)) {
		dprintf(1,"[usb_hub] ERROR: received status has incorrect size");
		return;
	}
	//something connected/disconnected
	if (usb_port_connect_change(&status)) {
		opResult = usb_hub_clear_port_feature(hc, target.address,
		    port, USB_HUB_FEATURE_C_PORT_CONNECTION);
		// TODO: check opResult
		if (usb_port_dev_connected(&status)) {
			dprintf(1,"[usb_hub] some connection changed");
			usb_hub_init_add_device(hc, port, target);
		} else {
			usb_hub_removed_device(hub, hc, port, target);
		}
	}
	//port reset
	if (usb_port_reset_completed(&status)) {
		dprintf(1,"[usb_hub] port reset complete");
		if (usb_port_enabled(&status)) {
			usb_hub_finalize_add_device(hub, hc, port, target);
		} else {
			dprintf(1,"[usb_hub] ERROR: port reset, but port still not enabled");
		}
	}

	usb_port_set_connect_change(&status, false);
	usb_port_set_reset(&status, false);
	usb_port_set_reset_completed(&status, false);
	usb_port_set_dev_connected(&status, false);
	if (status) {
		dprintf(1,"[usb_hub]there was some unsupported change on port %d",port);
	}
	/// \TODO handle other changes
	/// \TODO debug log for various situations

}

/* Check changes on all known hubs.
 */
void usb_hub_check_hub_changes(void) {
	/*
	 * Iterate through all hubs.
	 */
	usb_general_list_t * lst_item;
	for (lst_item = usb_hub_list.next;
			lst_item != &usb_hub_list;
			lst_item = lst_item->next) {
		usb_hub_info_t * hub_info = ((usb_hub_info_t*)lst_item->data);
		/*
		 * Check status change pipe of this hub.
		 */

		usb_target_t target;
		target.address = hub_info->usb_device->address;
		target.endpoint = 1;/// \TODO get from endpoint descriptor
		dprintf(1,"[usb_hub] checking changes for hub at addr %d",
		    target.address);

		size_t port_count = hub_info->port_count;

		/*
		 * Connect to respective HC.
		 */
		int hc = usb_drv_hc_connect_auto(hub_info->device, 0);
		if (hc < 0) {
			continue;
		}

		/// FIXME: count properly
		size_t byte_length = ((port_count+1) / 8) + 1;

		void *change_bitmap = malloc(byte_length);
		size_t actual_size;
		usb_handle_t handle;

		/*
		 * Send the request.
		 */
		int opResult = usb_drv_async_interrupt_in(hc, target,
				change_bitmap, byte_length, &actual_size,
				&handle);

		usb_drv_async_wait_for(handle);

		if (opResult != EOK) {
			dprintf(1,"[usb_hub] something went wrong while getting status of hub");
			continue;
		}
		unsigned int port;
		for (port = 1; port < port_count+1; ++port) {
			bool interrupt =
					(((uint8_t*) change_bitmap)[port / 8] >> (port % 8)) % 2;
			if (interrupt) {
				usb_hub_process_interrupt(
				        hub_info, hc, port, hub_info->usb_device->address);
			}
		}

		ipc_hangup(hc);
	}
}





/**
 * @}
 */
