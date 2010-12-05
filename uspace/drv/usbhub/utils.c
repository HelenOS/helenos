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
#include <usb/devreq.h>
#include <usbhc_iface.h>
#include <usb/usbdrv.h>
#include <usb/descriptor.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <usb/classes/hub.h>
#include "usbhub.h"
#include "usbhub_private.h"
#include "port_status.h"
#include <usb/devreq.h>

static void check_hub_changes(void);

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
	if (sdescriptor[1] != USB_DESCTYPE_HUB) return NULL;
	usb_hub_descriptor_t * result = usb_new(usb_hub_descriptor_t);
	//uint8_t size = sdescriptor[0];
	result->ports_count = sdescriptor[2];
	/// @fixme handling of endianness??
	result->hub_characteristics = sdescriptor[4] + 256 * sdescriptor[3];
	result->pwr_on_2_good_time = sdescriptor[5];
	result->current_requirement = sdescriptor[6];
	size_t var_size = result->ports_count / 8 + ((result->ports_count % 8 > 0) ? 1 : 0);
	result->devices_removable = (uint8_t*) malloc(var_size);

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
){
	usb_handle_t handle;
	int opResult;
	//setup
	opResult = usb_drv_async_control_read_setup(phone, target,
        request, sizeof(usb_device_request_setup_packet_t),
        &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	//read
	opResult = usb_drv_async_control_read_data(phone, target,
	    rcvd_buffer,  rcvd_size, actual_size,
	    &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	//finalize
	opResult = usb_drv_async_control_read_status(phone, target,
	    &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	return EOK;
}


int usb_drv_sync_control_write(
    int phone, usb_target_t target,
    usb_device_request_setup_packet_t * request,
    void * sent_buffer, size_t sent_size
){
	usb_handle_t handle;
	int opResult;
	//setup
	opResult = usb_drv_async_control_write_setup(phone, target,
        request, sizeof(usb_device_request_setup_packet_t),
        &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	//write
	opResult = usb_drv_async_control_write_data(phone, target,
        sent_buffer, sent_size,
        &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	//finalize
	opResult = usb_drv_async_control_write_status(phone, target,
        &handle);
	if(opResult!=EOK){
		return opResult;
	}
	opResult = usb_drv_async_wait_for(handle);
	if(opResult!=EOK){
		return opResult;
	}
	return EOK;
}

//list implementation

usb_general_list_t * usb_lst_create(void){
	usb_general_list_t* result = usb_new(usb_general_list_t);
	usb_lst_init(result);
	return result;
}

void usb_lst_init(usb_general_list_t * lst){
	lst->prev = lst;
	lst->next = lst;
	lst->data = NULL;
}

void usb_lst_prepend(usb_general_list_t* item, void* data){
	usb_general_list_t* appended = usb_new(usb_general_list_t);
	appended->data=data;
	appended->next=item;
	appended->prev=item->prev;
	item->prev->next = appended;
	item->prev = appended;
}

void usb_lst_append(usb_general_list_t* item, void* data){
	usb_general_list_t* appended =usb_new(usb_general_list_t);
	appended->data=data;
	appended->next=item->next;
	appended->prev=item;
	item->next->prev = appended;
	item->next = appended;
}


void usb_lst_remove(usb_general_list_t* item){
	item->next->prev = item->prev;
	item->prev->next = item->next;
}



//*********************************************
//
//  hub driver code
//
//*********************************************

usb_hub_info_t * usb_create_hub_info(device_t * device) {
	usb_hub_info_t* result = usb_new(usb_hub_info_t);
	//result->device = device;
	result->port_count = -1;

	//get hc connection
	int hc = usb_drv_hc_connect(NULL, 0);
	printf("[usb_hub] phone to hc = %d\n",hc);
	if (hc < 0) {
		return result;
	}
	//get some hub info

	usb_address_t addr = usb_drv_get_my_address(hc,device);
	printf("[usb_hub] addres of newly created hub = %d\n",addr);
	/*if(addr<0){
		//return result;
		
	}*/
	result->device = usb_new(usb_hcd_attached_device_info_t);
	result->device->address=addr;
	//hub configuration?
	printf("[usb_hub] hub info created\n");

	
	
	return result;
}

/** Callback when new hub device is detected.
 *
 * @param dev New device.
 * @return Error code.
 */
int usb_add_hub_device(device_t *dev) {
	printf(NAME ": add_hub_device(handle=%d)\n", (int) dev->handle);
	printf("[usb_hub] hub device\n");

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */

	//create the hub structure
	usb_hub_info_t * hub_info = usb_create_hub_info(dev);
	usb_lst_append(&usb_hub_list, hub_info);
	printf("[usb_hub] hub info added to list\n");
	//(void)hub_info;
	check_hub_changes();
	printf("[usb_hub] hub dev added\n");
	//test port status type...

	return EOK;
	//return ENOTSUP;
}




/** Check changes on all known hubs.
 */
static void check_hub_changes(void) {
	/*
	 * Iterate through all hubs.
	 */
	usb_general_list_t * lst_item;
	for (lst_item = usb_hub_list.next;
	     lst_item != &usb_hub_list;
	     lst_item = lst_item->next) {
		printf("[usb_hub] checking hub changes\n");
		/*
		 * Check status change pipe of this hub.
		 */
		
		usb_target_t target = {
			.address = 5,
			.endpoint = 1
		};
		/// \TODO uncomment once it works correctly
		//target.address = usb_create_hub_info(lst_item)->device->address;

		size_t port_count = 7;

		/*
		 * Connect to respective HC.
		 */
		/// \FIXME this is incorrect code: here
		/// must be used particular device instead of NULL
		int hc = usb_drv_hc_connect(NULL, 0);
		if (hc < 0) {
			continue;
		}

		// FIXME: count properly
		size_t byte_length = (port_count / 8) + 1;

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

		if(opResult!=EOK){
			printf("[usb_hub] something went wrong while getting status of hub\n");
			continue;
		}
		unsigned int port;
		for(port=0;port<port_count;++port){
			bool interrupt = (((uint8_t*)change_bitmap)[port/8]>>(port%8))%2;
			if(interrupt){
				printf("[usb_hub] interrupt at port %d\n",port);
				//determine type of change
				usb_port_status_t status;
				size_t rcvd_size;
				usb_device_request_setup_packet_t request;
				usb_hub_set_port_status_request(&request,port);

				opResult = usb_drv_sync_control_read(
				    hc, target,
				    &request,
				    &status, 4, &rcvd_size
				);
				if(opResult!=EOK){
					continue;
				}
				if(rcvd_size!=sizeof(usb_port_status_t)){
					continue;
				}
				
				if(usb_port_connect_change(&status)){
					printf("[usb_hub] some connectionchanged\n");
					usb_drv_reserve_default_address(hc);
					//enable port
					usb_hub_set_enable_port_request(&request,port);
					opResult = usb_drv_sync_control_write(
						hc, target,
						&request,
						NULL, 0
					);
					if(opResult!=EOK){
						continue;
					}
					//set address
					usb_address_t new_device_address =
							usb_drv_request_address(hc);
					usb_hub_set_set_address_request
							(&request,new_device_address);
					opResult = usb_drv_sync_control_write(
						hc, target,
						&request,
						NULL, 0
					);
					//some other work with drivers
					/// \TODO do the work with drivers


					usb_drv_release_default_address(hc);
				}else{
					printf("[usb_hub] no supported event occured\n");
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
