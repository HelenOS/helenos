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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Hub driver.
 */
#include <driver.h>
#include <bool.h>
#include <errno.h>

#include <usbhc_iface.h>
#include <usb/usbdrv.h>
#include <usb/descriptor.h>
#include <usb/devreq.h>
#include <usb/classes/hub.h>

#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"


size_t USB_HUB_MAX_DESCRIPTOR_SIZE = 71;

//*********************************************
//
//  various utils
//
//*********************************************

//hub descriptor utils

void * usb_serialize_hub_descriptor(usb_hub_descriptor_t * descriptor) {
	//base size
	size_t size = 7;
	//variable size according to port count
	size_t var_size = descriptor->ports_count / 8 + ((descriptor->ports_count % 8 > 0) ? 1 : 0);
	size += 2 * var_size;
	uint8_t * result = (uint8_t*) malloc(size);
	//size
	result[0] = size;
	//descriptor type
	result[1] = USB_DESCTYPE_HUB;
	result[2] = descriptor->ports_count;
	/// @fixme handling of endianness??
	result[3] = descriptor->hub_characteristics / 256;
	result[4] = descriptor->hub_characteristics % 256;
	result[5] = descriptor->pwr_on_2_good_time;
	result[6] = descriptor->current_requirement;

	size_t i;
	for (i = 0; i < var_size; ++i) {
		result[7 + i] = descriptor->devices_removable[i];
	}
	for (i = 0; i < var_size; ++i) {
		result[7 + var_size + i] = 255;
	}
	return result;
}

usb_hub_descriptor_t * usb_deserialize_hub_desriptor(void * serialized_descriptor) {
	uint8_t * sdescriptor = (uint8_t*) serialized_descriptor;

	if (sdescriptor[1] != USB_DESCTYPE_HUB) {
		printf("[usb_hub] wrong descriptor %x\n",sdescriptor[1]);
		return NULL;
	}

	usb_hub_descriptor_t * result = usb_new(usb_hub_descriptor_t);
	

	result->ports_count = sdescriptor[2];
	/// @fixme handling of endianness??
	result->hub_characteristics = sdescriptor[4] + 256 * sdescriptor[3];
	result->pwr_on_2_good_time = sdescriptor[5];
	result->current_requirement = sdescriptor[6];
	size_t var_size = result->ports_count / 8 + ((result->ports_count % 8 > 0) ? 1 : 0);
	result->devices_removable = (uint8_t*) malloc(var_size);
	//printf("[usb_hub] getting removable devices data \n");
	size_t i;
	for (i = 0; i < var_size; ++i) {
		result->devices_removable[i] = sdescriptor[7 + i];
	}
	return result;
}

//control transactions

int usb_drv_sync_control_read(
		int phone, usb_target_t target,
		usb_device_request_setup_packet_t * request,
		void * rcvd_buffer, size_t rcvd_size, size_t * actual_size
		) {
	usb_handle_t handle;
	int opResult;
	//setup
	opResult = usb_drv_async_control_read_setup(phone, target,
			request, sizeof (usb_device_request_setup_packet_t),
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	//read
	opResult = usb_drv_async_control_read_data(phone, target,
			rcvd_buffer, rcvd_size, actual_size,
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	//finalize
	opResult = usb_drv_async_control_read_status(phone, target,
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	return EOK;
}

int usb_drv_sync_control_write(
		int phone, usb_target_t target,
		usb_device_request_setup_packet_t * request,
		void * sent_buffer, size_t sent_size
		) {
	usb_handle_t handle;
	int opResult;
	//setup
	opResult = usb_drv_async_control_write_setup(phone, target,
			request, sizeof (usb_device_request_setup_packet_t),
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	//write
	opResult = usb_drv_async_control_write_data(phone, target,
			sent_buffer, sent_size,
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	//finalize
	opResult = usb_drv_async_control_write_status(phone, target,
			&handle);
	if (opResult != EOK) {
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if (opResult != EOK) {
		return opResult;
	}
	return EOK;
}

//list implementation

usb_general_list_t * usb_lst_create(void) {
	usb_general_list_t* result = usb_new(usb_general_list_t);
	usb_lst_init(result);
	return result;
}

void usb_lst_init(usb_general_list_t * lst) {
	lst->prev = lst;
	lst->next = lst;
	lst->data = NULL;
}

void usb_lst_prepend(usb_general_list_t* item, void* data) {
	usb_general_list_t* appended = usb_new(usb_general_list_t);
	appended->data = data;
	appended->next = item;
	appended->prev = item->prev;
	item->prev->next = appended;
	item->prev = appended;
}

void usb_lst_append(usb_general_list_t* item, void* data) {
	usb_general_list_t* appended = usb_new(usb_general_list_t);
	appended->data = data;
	appended->next = item->next;
	appended->prev = item;
	item->next->prev = appended;
	item->next = appended;
}

void usb_lst_remove(usb_general_list_t* item) {
	item->next->prev = item->prev;
	item->prev->next = item->next;
}

static void usb_hub_test_port_status(void) {
	printf("[usb_hub] -------------port status test---------\n");
	usb_port_status_t status = 0;

	//printf("original status %d (should be 0)\n",(uint32_t)status);
	usb_port_set_bit(&status, 1, 1);
	//printf("%d =?= 2\n",(uint32_t)status);
	if (status != 2) {
		printf("[usb_port_status] test failed: wrong set of bit 1\n");
		return;
	}
	usb_port_set_bit(&status, 3, 1);
	if (status != 10) {
		printf("[usb_port_status] test failed: wrong set of bit 3\n");
		return;
	}

	usb_port_set_bit(&status, 15, 1);
	if (status != 10 + (1 << 15)) {
		printf("[usb_port_status] test failed: wrong set of bit 15\n");
		return;
	}
	usb_port_set_bit(&status, 1, 0);
	if (status != 8 + (1 << 15)) {
		printf("[usb_port_status] test failed: wrong unset of bit 1\n");
		return;
	}
	int i;
	for (i = 0; i < 32; ++i) {
		if (i == 3 || i == 15) {
			if (!usb_port_get_bit(&status, i)) {
				printf("[usb_port_status] test failed: wrong bit at %d\n", i);
			}
		} else {
			if (usb_port_get_bit(&status, i)) {
				printf("[usb_port_status] test failed: wrong bit at %d\n", i);
			}
		}
	}

	printf("test ok\n");


	//printf("%d =?= 10\n",(uint32_t)status);

	//printf("this should be 0: %d \n",usb_port_get_bit(&status,0));
	//printf("this should be 1: %d \n",usb_port_get_bit(&status,1));
	//printf("this should be 0: %d \n",usb_port_get_bit(&status,2));
	//printf("this should be 1: %d \n",usb_port_get_bit(&status,3));
	//printf("this should be 0: %d \n",usb_port_get_bit(&status,4));




}

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
	printf("[usb_hub] addres of newly created hub = %d\n", addr);
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
		printf("[usb_hub] failed when receiving hub descriptor, badcode = %d\n",opResult);
		///\TODO memory leak will occur here!
		return result;
	}
	//printf("[usb_hub] deserializing descriptor\n");
	descriptor = usb_deserialize_hub_desriptor(serialized_descriptor);
	if(descriptor==NULL){
		printf("[usb_hub] could not deserialize descriptor \n");
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

	printf("[usb_hub] hub info created\n");

	return result;
}

int usb_add_hub_device(device_t *dev) {
	printf(NAME ": add_hub_device(handle=%d)\n", (int) dev->handle);
	printf("[usb_hub] hub device\n");

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */

	//create the hub structure
	//get hc connection
	/// \TODO correct params
	int hc = usb_drv_hc_connect(dev, 0);

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
		printf("[usb_hub] could not get device descriptor, %d\n",opResult);
		return 1;///\TODO some proper error code needed
	}
	printf("[usb_hub] hub has %d configurations\n",std_descriptor.configuration_count);
	if(std_descriptor.configuration_count<1){
		printf("[usb_hub] THERE ARE NO CONFIGURATIONS AVAILABLE\n");
	}
	usb_standard_configuration_descriptor_t config_descriptor;
	opResult = usb_drv_req_get_bare_configuration_descriptor(hc,
        target.address, 0,
        &config_descriptor);
	if(opResult!=EOK){
		printf("[usb_hub] could not get configuration descriptor, %d\n",opResult);
		return 1;///\TODO some proper error code needed
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
		printf("[usb_hub]something went wrong when setting hub`s configuration, %d\n", opResult);
	}


	for (port = 1; port < hub_info->port_count+1; ++port) {
		usb_hub_set_power_port_request(&request, port);
		opResult = usb_drv_sync_control_write(hc, target, &request, NULL, 0);
		printf("[usb_hub] powering port %d\n",port);
		if (opResult != EOK) {
			printf("[usb_hub]something went wrong when setting hub`s %dth port\n", port);
		}
	}
	//ports powered, hub seems to be enabled
	

	ipc_hangup(hc);

	//add the hub to list
	usb_lst_append(&usb_hub_list, hub_info);
	printf("[usb_hub] hub info added to list\n");
	//(void)hub_info;
	usb_hub_check_hub_changes();

	/// \TODO start the check loop, if not already started...

	//this is just a test for port status bitmap type
	usb_hub_test_port_status();

	printf("[usb_hub] hub dev added\n");
	printf("\taddress %d, has %d ports \n",
			hub_info->usb_device->address,
			hub_info->port_count);
	printf("\tused configuration %d\n",config_descriptor.configuration_number);

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
	printf("[usb_hub] some connection changed\n");
	//get default address
	opResult = usb_drv_reserve_default_address(hc);
	if (opResult != EOK) {
		printf("[usb_hub] cannot assign default address, it is probably used\n");
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
		//continue;
		printf("[usb_hub] something went wrong when reseting a port\n");
	}
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
	printf("[usb_hub] finalizing add device\n");

	/* Request address at from host controller. */
	usb_address_t new_device_address = usb_drv_request_address(hc);
	if (new_device_address < 0) {
		printf("[usb_hub] failed to get free USB address\n");
		opResult = new_device_address;
		goto release;
	}
	printf("[usb_hub] setting new address\n");
	opResult = usb_drv_req_set_address(hc, USB_ADDRESS_DEFAULT,
	    new_device_address);

	if (opResult != EOK) {
		printf("[usb_hub] could not set address for new device\n");
		goto release;
	}

release:
	printf("[usb_hub] releasing default address\n");
	usb_drv_release_default_address(hc);
	if (opResult != EOK) {
		return;
	}

	devman_handle_t child_handle;
	opResult = usb_drv_register_child_in_devman(hc, hub->device,
            new_device_address, &child_handle);
	if (opResult != EOK) {
		printf("[usb_hub] could not start driver for new device \n");
		return;
	}
	hub->attached_devs[port].devman_handle = child_handle;
	hub->attached_devs[port].address = new_device_address;

	opResult = usb_drv_bind_address(hc, new_device_address, child_handle);
	if (opResult != EOK) {
		printf("[usb_hub] could not assign address of device in hcd \n");
		return;
	}
	printf("[usb_hub] new device address %d, handle %d\n",
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
			printf("[usb_hub] could not release address of removed device: %d\n",opResult);
		}
		hub->attached_devs[port].address = 0;
	}else{
		printf("[usb_hub] this is strange, disconnected device had no address\n");
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
	printf("[usb_hub] interrupt at port %d\n", port);
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
		printf("[usb_hub] ERROR: could not get port status\n");
		return;
	}
	if (rcvd_size != sizeof (usb_port_status_t)) {
		printf("[usb_hub] ERROR: received status has incorrect size\n");
		return;
	}
	//something connected/disconnected
	if (usb_port_connect_change(&status)) {
		if (usb_port_dev_connected(&status)) {
			printf("[usb_hub] some connection changed\n");
			usb_hub_init_add_device(hc, port, target);
		} else {
			usb_hub_removed_device(hub, hc, port, target);
		}
	}
	//port reset
	if (usb_port_reset_completed(&status)) {
		printf("[usb_hub] port reset complete\n");
		if (usb_port_enabled(&status)) {
			usb_hub_finalize_add_device(hub, hc, port, target);
		} else {
			printf("[usb_hub] ERROR: port reset, but port still not enabled\n");
		}
	}

	usb_port_set_connect_change(&status, false);
	usb_port_set_reset(&status, false);
	usb_port_set_reset_completed(&status, false);
	usb_port_set_dev_connected(&status, false);
	if (status) {
		printf("[usb_hub]there was some unsupported change on port %d\n",port);
	}
	/// \TODO handle other changes
	/// \TODO debug log for various situations



	/*
	//configure device
	usb_drv_reserve_default_address(hc);

	usb_address_t new_device_address = usb_drv_request_address(hc);


	usb_drv_release_default_address(hc);
	 * */
}

/** Check changes on all known hubs.
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
		printf("[usb_hub] checking changes for hub at addr %d\n",
		    target.address);

		size_t port_count = hub_info->port_count;

		/*
		 * Connect to respective HC.
		 */
		int hc = usb_drv_hc_connect(hub_info->device, 0);
		if (hc < 0) {
			continue;
		}

		// FIXME: count properly
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
			printf("[usb_hub] something went wrong while getting status of hub\n");
			continue;
		}
		unsigned int port;
		for (port = 1; port < port_count+1; ++port) {
			bool interrupt = (((uint8_t*) change_bitmap)[port / 8] >> (port % 8)) % 2;
			if (interrupt) {
				usb_hub_process_interrupt(
				        hub_info, hc, port, hub_info->usb_device->address);
			}
		}


		/*
		 * TODO: handle the changes.
		 */

		/*
		 * WARNING: sample code, will not work out of the box.
		 * And does not contain code for checking for errors.
		 */
#if 0
		/*
		 * Before opening the port, we must acquire the default
		 * address.
		 */
		usb_drv_reserve_default_address(hc);

		usb_address_t new_device_address = usb_drv_request_address(hc);

		// TODO: open the port

		// TODO: send request for setting address to new_device_address

		/*
		 * Once new address is set, we can release the default
		 * address.
		 */
		usb_drv_release_default_address(hc);

		/*
		 * Obtain descriptors and create match ids for devman.
		 */

		// TODO: get device descriptors

		// TODO: create match ids

		// TODO: add child device

		// child_device_register sets the device handle
		// TODO: store it here
		devman_handle_t new_device_handle = 0;

		/*
		 * Inform the HC that the new device has devman handle
		 * assigned.
		 */
		usb_drv_bind_address(hc, new_device_address, new_device_handle);

		/*
		 * That's all.
		 */
#endif


		/*
		 * Hang-up the HC-connected phone.
		 */
		ipc_hangup(hc);
	}
}

/**
 * @}
 */
