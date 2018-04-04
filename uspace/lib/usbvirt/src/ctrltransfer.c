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
errno_t process_control_transfer(usbvirt_device_t *dev,
    const usbvirt_control_request_handler_t *control_handlers,
    const usb_device_request_setup_packet_t *setup,
    uint8_t *data, size_t *data_sent_size)
{
	assert(dev);
	assert(setup);

	if (control_handlers == NULL) {
		return EFORWARD;
	}

	const usbvirt_control_request_handler_t *handler = control_handlers;
	for (; handler->callback != NULL; ++handler) {
		if (handler->request != setup->request ||
		    handler->request_type != setup->request_type) {
			continue;
		}

		usb_log_debug("Control transfer: %s(%s)", handler->name,
		    usb_debug_str_buffer((uint8_t *) setup, sizeof(*setup), 0));
		errno_t rc = handler->callback(dev, setup, data, data_sent_size);
		if (rc != EFORWARD) {
			return rc;
		}

	}

	return EFORWARD;
}


/**
 * @}
 */
