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
#include <usb/hcdhubd.h>
#include <usb/devreq.h>
#include <usbhc_iface.h>
#include <usb/descriptor.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <usb/classes/hub.h>
#include "hcdhubd_private.h"

static void check_hub_changes(void);

size_t USB_HUB_MAX_DESCRIPTOR_SIZE = 71;

//*********************************************
//
//  various utils
//
//*********************************************

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
	usb_hub_descriptor_t * result = (usb_hub_descriptor_t*) malloc(sizeof (usb_hub_descriptor_t));
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


//*********************************************
//
//  hub driver code
//
//*********************************************

static void set_hub_address(usb_hc_device_t *hc, usb_address_t address);

usb_hcd_hub_info_t * usb_create_hub_info(device_t * device) {
	usb_hcd_hub_info_t* result = (usb_hcd_hub_info_t*) malloc(sizeof (usb_hcd_hub_info_t));
	//get parent device
	device_t * my_hcd = device;
	while (my_hcd->parent)
		my_hcd = my_hcd->parent;
	//dev->
	printf("[hcdhubd]%s: owner hcd found: %s\n", hc_driver->name, my_hcd->name);
	//we add the hub into the first hc
	//link_t *link_hc = hc_list.next;
	//usb_hc_device_t *hc = list_get_instance(link_hc,
	//		usb_hc_device_t, link);
	//must get generic device info


	return result;
}

/** Callback when new hub device is detected.
 *
 * @param dev New device.
 * @return Error code.
 */
int usb_add_hub_device(device_t *dev) {
	//usb_hc_device_t *hc = list_get_instance(hc_list.next, usb_hc_device_t, link);
	assert(dev->parent);
	usb_hc_device_t *hc = (usb_hc_device_t*)dev->parent->driver_data;
	usb_address_t addr =usb_use_free_address(hc);
	if(addr<0){
		printf("[hcdhubd] ERROR: cannot find an address \n");
	}
	set_hub_address(hc, addr);

	check_hub_changes();

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */
	//insert hub into list
	//find owner hcd
	device_t * my_hcd = dev;
	while (my_hcd->parent)
		my_hcd = my_hcd->parent;
	//dev->
	printf("[hcdhubd]%s: owner hcd found: %s\n", hc_driver->name, my_hcd->name);
	my_hcd = dev;
	while (my_hcd->parent)
		my_hcd = my_hcd->parent;
	//dev->

	printf("[hcdhubd]%s: owner hcd found: %s\n", hc_driver->name, my_hcd->name);

	//create the hub structure
	usb_hcd_hub_info_t * hub_info = usb_create_hub_info(dev);


	//append into the list
	//we add the hub into the first hc
	list_append(&hub_info->link, &hc->hubs);



	return EOK;
	//return ENOTSUP;
}

/** Sample usage of usb_hc_async functions.
 * This function sets hub address using standard SET_ADDRESS request.
 *
 * @warning This function shall be removed once you are familiar with
 * the usb_hc_ API.
 *
 * @param hc Host controller the hub belongs to.
 * @param address New hub address.
 */
static void set_hub_address(usb_hc_device_t *hc, usb_address_t address) {
	printf("[hcdhubd]%s: setting hub address to %d\n", hc->generic->name, address);
	usb_target_t target = {0, 0};
	usb_handle_t handle;
	int rc;

	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		.index = 0,
		.length = 0,
	};
	setup_packet.value = address;

	rc = usb_hc_async_control_write_setup(hc, target,
			&setup_packet, sizeof (setup_packet), &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_control_write_status(hc, target, &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_hc_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	printf("[hcdhubd]%s: hub address changed successfully to %d\n",
			hc->generic->name, address);
}

/** Check changes on all known hubs.
 */
static void check_hub_changes(void) {
	/*
	 * Iterate through all HCs.
	 */
	link_t *link_hc;
	for (link_hc = hc_list.next;
			link_hc != &hc_list;
			link_hc = link_hc->next) {
		usb_hc_device_t *hc = list_get_instance(link_hc,
				usb_hc_device_t, link);
		/*
		 * Iterate through all their hubs.
		 */
		link_t *link_hub;
		for (link_hub = hc->hubs.next;
				link_hub != &hc->hubs;
				link_hub = link_hub->next) {
			usb_hcd_hub_info_t *hub = list_get_instance(link_hub,
					usb_hcd_hub_info_t, link);

			/*
			 * Check status change pipe of this hub.
			 */
			usb_target_t target = {
				.address = hub->device->address,
				.endpoint = 1
			};

			// FIXME: count properly
			size_t byte_length = (hub->port_count / 8) + 1;

			void *change_bitmap = malloc(byte_length);
			size_t actual_size;
			usb_handle_t handle;

			/*
			 * Send the request.
			 * FIXME: check returned value for possible errors
			 */
			usb_hc_async_interrupt_in(hc, target,
					change_bitmap, byte_length, &actual_size,
					&handle);

			usb_hc_async_wait_for(handle);

			/*
			 * TODO: handle the changes.
			 */
		}
	}
}

/**
 * @}
 */
