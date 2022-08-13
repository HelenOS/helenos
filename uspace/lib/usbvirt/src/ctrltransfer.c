/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
