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
 * @brief Preprocessing of standard device requests.
 */
#include <errno.h>
#include <usb/devreq.h>

#include "private.h"

/*
 * All sub handlers must return EFORWARD to inform the caller that
 * they were not able to process the request (yes, it is abuse of
 * this error code but such error code shall not collide with anything
 * else in this context).
 */
 
static int handle_get_descriptor(uint8_t type, uint8_t index, uint16_t language,
    uint16_t length)
{
	/* 
	 * Standard device descriptor.
	 */
	if ((type == USB_DESCTYPE_DEVICE) && (index == 0)) {
		if (device->standard_descriptor) {
			return device->send_data(device, 0,
			    device->standard_descriptor,
			    device->standard_descriptor->length);
		} else {
			return EFORWARD;
		}
	}
	
	return EFORWARD;
}

static int handle_set_address(uint16_t new_address,
    uint16_t zero1, uint16_t zero2)
{
	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}
	
	if (new_address > 127) {
		return EINVAL;
	}
	
	/*
	 * TODO: inform the HC that device has new address assigned.
	 */
	return EOK;
}

int handle_std_request(usb_device_request_setup_packet_t *request, uint8_t *data)
{
	int rc;
	
	switch (request->request) {
		case USB_DEVREQ_GET_DESCRIPTOR:
			rc = handle_get_descriptor(
			    request->value_low, request->value_high,
			    request->index, request->length);
			break;
		
		case USB_DEVREQ_SET_ADDRESS:
			rc = handle_set_address(request->value,
			    request->index, request->length);
			break;
		
		default:
			rc = EFORWARD;
			break;
	}
	
	/*
	 * We preprocessed all we could.
	 * If it was not enough, pass the request to the actual driver.
	 */
	if (rc == EFORWARD) {
		if (DEVICE_HAS_OP(device, on_devreq_std)) {
			return device->ops->on_devreq_std(device,
			    request, data);
		}
	}
	
	return EOK;
}

/**
 * @}
 */
