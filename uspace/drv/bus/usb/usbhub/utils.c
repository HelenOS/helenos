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
 * @brief various utilities
 */
#include <ddf/driver.h>
#include <bool.h>
#include <errno.h>

#include <usbhc_iface.h>
#include <usb/descriptor.h>
#include <usb/classes/hub.h>

#include "usbhub.h"
#include "utils.h"


size_t USB_HUB_MAX_DESCRIPTOR_SIZE = 71;

//*********************************************
//
//  various utils
//
//*********************************************

//hub descriptor utils

/**
 * create uint8_t array with serialized descriptor
 *
 * @param descriptor
 * @return newly created serializd descriptor pointer
 */
void * usb_create_serialized_hub_descriptor(usb_hub_descriptor_t *descriptor) {
	//base size
	size_t size = 7;
	//variable size according to port count
	size_t var_size = (descriptor->ports_count + 7) / 8;
	size += 2 * var_size;
	uint8_t *result = malloc(size);
	//size
	if (result)
		usb_serialize_hub_descriptor(descriptor, result);
	return result;
}

/**
 * serialize descriptor into given buffer
 *
 * The buffer size is not checked.
 * @param descriptor
 * @param serialized_descriptor
 */
void usb_serialize_hub_descriptor(usb_hub_descriptor_t *descriptor,
    void *serialized_descriptor)
{
	//base size
	uint8_t *sdescriptor = serialized_descriptor;
	size_t size = 7;
	//variable size according to port count
	size_t var_size = (descriptor->ports_count + 7) / 8;
	size += 2 * var_size;
	//size
	sdescriptor[0] = size;
	//descriptor type
	sdescriptor[1] = USB_DESCTYPE_HUB;
	sdescriptor[2] = descriptor->ports_count;
	/// @fixme handling of endianness??
	sdescriptor[3] = descriptor->hub_characteristics / 256;
	sdescriptor[4] = descriptor->hub_characteristics % 256;
	sdescriptor[5] = descriptor->pwr_on_2_good_time;
	sdescriptor[6] = descriptor->current_requirement;

	size_t i;
	for (i = 0; i < var_size; ++i) {
		sdescriptor[7 + i] = descriptor->devices_removable[i];
	}
	for (i = 0; i < var_size; ++i) {
		sdescriptor[7 + var_size + i] = 255;
	}
}
/*----------------------------------------------------------------------------*/
/**
 * Deserialize descriptor into given pointer
 *
 * @param serialized_descriptor
 * @param descriptor
 * @return
 */
int usb_deserialize_hub_desriptor(
    void *serialized_descriptor, size_t size, usb_hub_descriptor_t *descriptor)
{
	uint8_t *sdescriptor = serialized_descriptor;

	if (sdescriptor[1] != USB_DESCTYPE_HUB) {
		usb_log_error("Trying to deserialize wrong descriptor %x\n",
		    sdescriptor[1]);
		return EINVAL;
	}
	if (size < 7) {
		usb_log_error("Serialized descriptor too small.\n");
		return EOVERFLOW;
	}

	descriptor->ports_count = sdescriptor[2];
	descriptor->hub_characteristics = sdescriptor[3] + 256 * sdescriptor[4];
	descriptor->pwr_on_2_good_time = sdescriptor[5];
	descriptor->current_requirement = sdescriptor[6];
	const size_t var_size = (descriptor->ports_count + 7) / 8;

	if (size < (7 + var_size)) {
		usb_log_error("Serialized descriptor too small.\n");
		return EOVERFLOW;
	}
	size_t i = 0;
	for (; i < var_size; ++i) {
		descriptor->devices_removable[i] = sdescriptor[7 + i];
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */
