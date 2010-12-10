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


	printf("[usb_hub] phone to hc = %d\n", hc);
	if (hc < 0) {
		return result;
	}
	//get some hub info
	/// \TODO get correct params
	usb_address_t addr = usb_drv_get_my_address(hc, device);
	addr = 7;
	printf("[usb_hub] addres of newly created hub = %d\n", addr);
	/*if(addr<0){
		//return result;
		
	}*/

	result->device = usb_new(usb_hcd_attached_device_info_t);
	result->device->address = addr;

	// get hub descriptor
	usb_target_t target;
	target.address = addr;
	target.endpoint = 0;
	usb_device_request_setup_packet_t request;
	usb_hub_get_descriptor_request(&request);
	void * serialized_descriptor = malloc(USB_HUB_MAX_DESCRIPTOR_SIZE);
	usb_hub_descriptor_t * descriptor;
	size_t received_size;
	int opResult;

	opResult = usb_drv_sync_control_read(
			hc, target, &request, serialized_descriptor,
			USB_HUB_MAX_DESCRIPTOR_SIZE, &received_size);
	if (opResult != EOK) {
		printf("[usb_hub] failed when receiving hub descriptor \n");
	}
	descriptor = usb_deserialize_hub_desriptor(serialized_descriptor);
	result->port_count = descriptor->ports_count;
	free(serialized_descriptor);
	free(descriptor);

	//finish

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
	//get hc connection
	/// \TODO correct params
	int hc = usb_drv_hc_connect(NULL, 0);

	usb_hub_info_t * hub_info = usb_create_hub_info(dev, hc);
	int port;
	int opResult;
	usb_device_request_setup_packet_t request;
	usb_target_t target;
	target.address = hub_info->device->address;
	target.endpoint = 0;
	for (port = 0; port < hub_info->port_count; ++port) {
		usb_hub_set_power_port_request(&request, port);
		opResult = usb_drv_sync_control_write(hc, target, &request, NULL, 0);
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
	check_hub_changes();

	/// \TODO start the check loop, if not already started...

	//this is just a test for port status bitmap type
	usb_hub_test_port_status();

	printf("[usb_hub] hub dev added\n");

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
	opResult = usb_drv_reserve_default_address(hc);
	if (opResult != EOK) {
		printf("[usb_hub] cannot assign default address, it is probably used\n");
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
static void usb_hub_finalize_add_device(
		int hc, uint16_t port, usb_target_t target) {

	usb_device_request_setup_packet_t request;
	int opResult;
	printf("[usb_hub] finalizing add device\n");
	usb_address_t new_device_address =
			usb_drv_request_address(hc);
	usb_hub_set_set_address_request
			(&request, new_device_address);
	opResult = usb_drv_sync_control_write(
			hc, target,
			&request,
			NULL, 0
			);
	if (opResult != EOK) {
		printf("[usb_hub] could not set address for new device\n");
	}
	usb_drv_release_default_address(hc);


	/// \TODO driver work
}

/**
 * unregister device address in hc, close the port
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_removed_device(int hc, uint16_t port, usb_target_t target) {
	usb_device_request_setup_packet_t request;
	int opResult;
	//disable port
	usb_hub_set_disable_port_request(&request, port);
	opResult = usb_drv_sync_control_write(
			hc, target,
			&request,
			NULL, 0
			);
	if (opResult != EOK) {
		//continue;
		printf("[usb_hub] something went wrong when disabling a port\n");
	}
	//remove device
	//close address
	//

	///\TODO this code is not complete
}

/**
 * process interrupts on given hub port
 * @param hc
 * @param port
 * @param target
 */
static void usb_hub_process_interrupt(int hc, uint16_t port, usb_target_t target) {
	printf("[usb_hub] interrupt at port %d\n", port);
	//determine type of change
	usb_port_status_t status;
	size_t rcvd_size;
	usb_device_request_setup_packet_t request;
	int opResult;
	usb_hub_set_port_status_request(&request, port);

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
			usb_hub_removed_device(hc, port, target);
		}
	}
	//port reset
	if (usb_port_reset_completed(&status)) {
		printf("[usb_hub] finalizing add device\n");
		if (usb_port_enabled(&status)) {
			usb_hub_finalize_add_device(hc, port, target);
		} else {
			printf("[usb_hub] ERROR: port reset, but port still not enabled\n");
		}
	}

	usb_port_set_connect_change(&status, false);
	usb_port_set_reset(&status, false);
	usb_port_set_reset_completed(&status, false);
	usb_port_set_dev_connected(&status, false);
	if (status) {
		printf("[usb_hub]there was some unsupported change on port\n");
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
		//which one?
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

		if (opResult != EOK) {
			printf("[usb_hub] something went wrong while getting status of hub\n");
			continue;
		}
		unsigned int port;
		for (port = 0; port < port_count; ++port) {
			bool interrupt = (((uint8_t*) change_bitmap)[port / 8] >> (port % 8)) % 2;
			if (interrupt) {
				usb_hub_process_interrupt(hc, port, target);
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
