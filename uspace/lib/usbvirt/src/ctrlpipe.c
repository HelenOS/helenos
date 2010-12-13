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

/** Compares handler type with request packet type.
 *
 * @param handler Handler.
 * @param request_packet Request packet.
 * @return Whether handler can serve this packet.
 */
static bool is_suitable_handler(usbvirt_control_transfer_handler_t *handler,
    usb_device_request_setup_packet_t *request_packet)
{
	return (
	    (handler->request_type == request_packet->request_type)
	    && (handler->request == request_packet->request));

}

/** Find suitable transfer handler for given request packet.
 *
 * @param handlers Array of available handlers.
 * @param request_packet Request SETUP packet.
 * @return Handler or NULL.
 */
static usbvirt_control_transfer_handler_t *find_handler(
    usbvirt_control_transfer_handler_t *handlers,
    usb_device_request_setup_packet_t *request_packet)
{
	if (handlers == NULL) {
		return NULL;
	}

	while (handlers->callback != NULL) {
		if (is_suitable_handler(handlers, request_packet)) {
			return handlers;
		}
		handlers++;
	}

	return NULL;
}



/** Handle communication over control pipe zero.
 */
int control_pipe(usbvirt_device_t *device, usbvirt_control_transfer_t *transfer)
{
	device->lib_debug(device, 1, USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO,
	    "op on control pipe zero (request_size=%u)", transfer->request_size);
	
	if (transfer->request_size < sizeof(usb_device_request_setup_packet_t)) {
		return ENOMEM;
	}
	
	usb_device_request_setup_packet_t *request
	    = (usb_device_request_setup_packet_t *) transfer->request;
	printf("Request: %d,%d\n", request->request_type, request->request);
	
	/*
	 * First, see whether user provided its own callback.
	 */
	int rc = EFORWARD;
	if (device->ops) {
		usbvirt_control_transfer_handler_t *user_handler
		    = find_handler(device->ops->control_transfer_handlers,
		    request);
		if (user_handler != NULL) {
			rc = user_handler->callback(device, request,
			    transfer->data);
		}
	}

	/*
	 * If there was no user callback or the callback returned EFORWARD,
	 * we need to run a local handler.
	 */
	if (rc == EFORWARD) {
		usbvirt_control_transfer_handler_t *lib_handler
		    = find_handler(control_pipe_zero_local_handlers,
		    request);
		if (lib_handler != NULL) {
			rc = lib_handler->callback(device, request,
			    transfer->data);
		}
	}
	
	/*
	 * Check for SET_ADDRESS finalization.
	 */
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
		
		device->lib_debug(device, 2, USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO,
		    "device address changed to %d (state %s)",
		    device->address, str_device_state(device->state));
	}
	
	return rc;
}

/**
 * @}
 */
