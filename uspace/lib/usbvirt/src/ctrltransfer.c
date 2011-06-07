/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Control transfer handling.
 */
#include "private.h"
#include <usb/dev/request.h>
#include <usb/debug.h>
#include <assert.h>
#include <errno.h>

/** Find and execute control transfer handler for virtual USB device.
 *
 * @param dev Target virtual device.
 * @param control_handlers Array of control request handlers.
 * @param setup Setup packet.
 * @param data Extra data.
 * @param data_sent_size Size of extra data in bytes.
 * @return Error code.
 * @retval EFORWARD No suitable handler found.
 */
int process_control_transfer(usbvirt_device_t *dev,
    usbvirt_control_request_handler_t *control_handlers,
    usb_device_request_setup_packet_t *setup,
    uint8_t *data, size_t *data_sent_size)
{
	assert(dev);
	assert(setup);

	if (control_handlers == NULL) {
		return EFORWARD;
	}

	usb_direction_t direction = setup->request_type & 128 ?
	    USB_DIRECTION_IN : USB_DIRECTION_OUT;
	usb_request_recipient_t req_recipient = setup->request_type & 31;
	usb_request_type_t req_type = (setup->request_type >> 5) & 3;

	usbvirt_control_request_handler_t *handler = control_handlers;
	while (handler->callback != NULL) {
		if (handler->req_direction != direction) {
			goto next;
		}
		if (handler->req_recipient != req_recipient) {
			goto next;
		}
		if (handler->req_type != req_type) {
			goto next;
		}
		if (handler->request != setup->request) {
			goto next;
		}

		usb_log_debug("Control transfer: %s(%s)\n", handler->name,
		    usb_debug_str_buffer((uint8_t*) setup, sizeof(*setup), 0));
		int rc = handler->callback(dev, setup, data, data_sent_size);
		if (rc == EFORWARD) {
			goto next;
		}

		return rc;

next:
		handler++;
	}

	return EFORWARD;
}


/**
 * @}
 */
