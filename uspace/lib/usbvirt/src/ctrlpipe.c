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

#define _GET_BIT(byte, bit) \
	(((byte) & (1 << (bit))) ? '1' : '0')
#define _GET_BITS(byte) \
	_GET_BIT(byte, 7), _GET_BIT(byte, 6), _GET_BIT(byte, 5), \
	_GET_BIT(byte, 4), _GET_BIT(byte, 3), _GET_BIT(byte, 2), \
	_GET_BIT(byte, 1), _GET_BIT(byte, 0)

static int find_and_run_handler(usbvirt_device_t *device,
    usbvirt_control_transfer_handler_t *handlers,
    usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data)
{
	int rc = EFORWARD;
	usbvirt_control_transfer_handler_t *suitable_handler
	    = find_handler(handlers, setup_packet);
	if (suitable_handler != NULL) {
		const char *callback_name = "user handler";
		if (suitable_handler->name != NULL) {
			callback_name = suitable_handler->name;
		}
		device->lib_debug(device, 1, USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO,
		    "pipe #0 - calling %s " \
		        "[%c.%c%c.%c%c%c%c%c, R%d, V%d, I%d, L%d]",
		    callback_name,
		    _GET_BITS(setup_packet->request_type),
		    setup_packet->request, setup_packet->value,
		    setup_packet->index, setup_packet->length);
		rc = suitable_handler->callback(device, setup_packet, data);
	}

	return rc;
}
#undef _GET_BITS
#undef _GET_BIT


/** Handle communication over control pipe zero.
 */
int control_pipe(usbvirt_device_t *device, usbvirt_control_transfer_t *transfer)
{
	device->lib_debug(device, 2, USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO,
	    "op on control pipe zero (request_size=%u)", transfer->request_size);
	
	if (transfer->request_size < sizeof(usb_device_request_setup_packet_t)) {
		return ENOMEM;
	}
	
	usb_device_request_setup_packet_t *request
	    = (usb_device_request_setup_packet_t *) transfer->request;

	/*
	 * First, see whether user provided its own callback.
	 */
	int rc = EFORWARD;
	if (device->ops) {
		rc = find_and_run_handler(device,
		    device->ops->control_transfer_handlers,
		    request, transfer->data);
	}

	/*
	 * If there was no user callback or the callback returned EFORWARD,
	 * we need to run a local handler.
	 */
	if (rc == EFORWARD) {
		rc = find_and_run_handler(device,
		    control_pipe_zero_local_handlers,
		    request, transfer->data);
	}
	
	/*
	 * Check for SET_ADDRESS finalization.
	 */
	if (device->new_address != -1) {
		/*
		 * TODO: handle when this request is invalid (e.g.
		 * setting address when in configured state).
		 */
		usbvirt_device_state_t new_state;
		if (device->new_address == 0) {
			new_state = USBVIRT_STATE_DEFAULT;
		} else {
			new_state = USBVIRT_STATE_ADDRESS;
		}
		device->address = device->new_address;
		
		device->new_address = -1;
		
		if (DEVICE_HAS_OP(device, on_state_change)) {
			device->ops->on_state_change(device, device->state,
			    new_state);
		}
		device->state = new_state;

		device->lib_debug(device, 2, USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO,
		    "device address changed to %d (state %s)",
		    device->address, str_device_state(device->state));
	}
	
	return rc;
}

/**
 * @}
 */
