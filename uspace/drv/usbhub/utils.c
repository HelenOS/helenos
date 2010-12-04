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

static void set_hub_address(device_t *dev, usb_address_t address);

usb_hcd_hub_info_t * usb_create_hub_info(device_t * device) {
	usb_hcd_hub_info_t* result = (usb_hcd_hub_info_t*) malloc(sizeof (usb_hcd_hub_info_t));

	return result;
}

/** Callback when new hub device is detected.
 *
 * @param dev New device.
 * @return Error code.
 */
int usb_add_hub_device(device_t *dev) {
	printf(NAME ": add_hub_device(handle=%d)\n", (int) dev->handle);
	set_hub_address(dev, 5);

	check_hub_changes();

	/*
	 * We are some (probably deeply nested) hub.
	 * Thus, assign our own operations and explore already
	 * connected devices.
	 */

	//create the hub structure
	usb_hcd_hub_info_t * hub_info = usb_create_hub_info(dev);
	(void)hub_info;

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
static void set_hub_address(device_t *dev, usb_address_t address) {
	printf(NAME ": setting hub address to %d\n", address);
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

	int hc = usb_drv_hc_connect(dev, 0);

	rc = usb_drv_async_control_write_setup(hc, target,
			&setup_packet, sizeof (setup_packet), &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_drv_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_drv_async_control_write_status(hc, target, &handle);
	if (rc != EOK) {
		return;
	}

	rc = usb_drv_async_wait_for(handle);
	if (rc != EOK) {
		return;
	}

	printf(NAME ": hub address changed\n");
}

/** Check changes on all known hubs.
 */
static void check_hub_changes(void) {
	/*
	 * Iterate through all hubs.
	 */
	for (; false; ) {
		/*
		 * Check status change pipe of this hub.
		 */
		usb_target_t target = {
			.address = 5,
			.endpoint = 1
		};

		size_t port_count = 7;

		/*
		 * Connect to respective HC.
		 */
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
		 * FIXME: check returned value for possible errors
		 */
		usb_drv_async_interrupt_in(hc, target,
				change_bitmap, byte_length, &actual_size,
				&handle);

		usb_drv_async_wait_for(handle);

		/*
		 * TODO: handle the changes.
		 */


		/*
		 * Hang-up the HC-connected phone.
		 */
		ipc_hangup(hc);
	}
}

/**
 * @}
 */
