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

static usb_direction_t request_get_direction(uint8_t request_type)
{
	int bit7 = GET_MIDBITS(request_type, 1, 7);
	return bit7 ? USB_DIRECTION_IN : USB_DIRECTION_OUT;
}

static int request_get_type(uint8_t request_type)
{
	return GET_MIDBITS(request_type, 2, 5);
}

static int request_get_recipient(uint8_t request_type)
{
	return GET_MIDBITS(request_type, 5, 0);
}


typedef struct {
	uint8_t request_type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} __attribute__ ((packed)) devreq_setup_packet_t;

int control_pipe(void *buffer, size_t size)
{
	if (size < sizeof(devreq_setup_packet_t)) {
		return ENOMEM;
	}
	
	devreq_setup_packet_t *request = (devreq_setup_packet_t *) buffer;
	uint8_t *remaining_data = ((uint8_t *) request) + sizeof(devreq_setup_packet_t);
	
	usb_direction_t direction = request_get_direction(request->request_type);
	int type = request_get_type(request->request_type);
	int recipient = request_get_recipient(request->request_type);
	
	
	switch (type) {
		case REQUEST_TYPE_STANDARD:
			return handle_std_request(direction, recipient,
			    request->request, request->value,
			    request->index, request->length,
			    remaining_data);
			break;
		case REQUEST_TYPE_CLASS:
			if (DEVICE_HAS_OP(device, on_devreq_class)) {
				return device->ops->on_devreq_class(device,
				    direction, recipient,
				    request->request, request->value,
				    request->index, request->length,
				    remaining_data);
			}
			break;
		default:
			break;
	}
	
	return EOK;
}

/**
 * @}
 */
