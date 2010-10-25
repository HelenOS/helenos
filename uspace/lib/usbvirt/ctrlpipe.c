/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Device control pipe.
 */
#include <errno.h>

#include "private.h"

#define REQUEST_TYPE_STANDARD 0 
#define REQUEST_TYPE_CLASS 1

#define GET_MIDBITS_MASK(size, shift) \
	(((1 << size) - 1) << shift)
#define GET_MIDBITS(value, size, shift) \
	((value & GET_MIDBITS_MASK(size, shift)) >> shift)

usb_address_t dev_new_address = -1;

/** Tell request type.
 * By type is meant either standard, class, vendor or other.
 */
static int request_get_type(uint8_t request_type)
{
	return GET_MIDBITS(request_type, 2, 5);
}

/** Handle communication over control pipe zero.
 */
int control_pipe(usbvirt_device_t *device, usbvirt_control_transfer_t *transfer)
{
	if (transfer->request_size < sizeof(usb_device_request_setup_packet_t)) {
		return ENOMEM;
	}
	
	usb_device_request_setup_packet_t *request = (usb_device_request_setup_packet_t *) transfer->request;
	uint8_t *remaining_data = transfer->data;
	
	int type = request_get_type(request->request_type);
	
	int rc = EOK;
	
	switch (type) {
		case REQUEST_TYPE_STANDARD:
			rc = handle_std_request(device, request, remaining_data);
			break;
		case REQUEST_TYPE_CLASS:
			if (DEVICE_HAS_OP(device, on_class_device_request)) {
				rc = device->ops->on_class_device_request(device,
				    request, remaining_data);
			}
			break;
		default:
			break;
	}
	
	if (device->new_address != -1) {
		/*
		 * TODO: handle when this request is invalid (e.g.
		 * setting address when in configured state).
		 */
		if (device->new_address == 0) {
			device->state = USBVIRT_STATE_DEFAULT;
		} else {
			device->state = USBVIRT_STATE_ADDRESS;
		}
		device->address = device->new_address;
		
		device->new_address = -1;
	}
	
	return rc;
}

/**
 * @}
 */
