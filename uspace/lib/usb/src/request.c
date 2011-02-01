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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Standard USB requests (implementation).
 */
#include <usb/request.h>
#include <usb/devreq.h>
#include <errno.h>

/** Prepare setup packet.
 *
 * @param name Variable name with the setup packet.
 * @param p_direction Data transfer direction.
 * @param p_type Request type (standard/class/vendor)
 * @param p_recipient Recipient of the request.
 * @param p_request Request.
 * @param p_value wValue field of setup packet.
 * @param p_index wIndex field of setup packet.
 * @param p_length Length of extra data.
 */
#define PREPARE_SETUP_PACKET(name, p_direction, p_type, p_recipient, \
    p_request, p_value, p_index, p_length) \
	usb_device_request_setup_packet_t name = { \
		.request_type = \
			((p_direction) == USB_DIRECTION_IN ? 128 : 0) \
			| ((p_type) << 5) \
			| (p_recipient), \
		.request = (p_request), \
		{ .value = (p_value) }, \
		.index = (p_index), \
		.length = (p_length) \
	}

/** Prepare setup packet.
 *
 * @param name Variable name with the setup packet.
 * @param p_direction Data transfer direction.
 * @param p_type Request type (standard/class/vendor)
 * @param p_recipient Recipient of the request.
 * @param p_request Request.
 * @param p_value_low wValue field of setup packet (low byte).
 * @param p_value_high wValue field of setup packet (high byte).
 * @param p_index wIndex field of setup packet.
 * @param p_length Length of extra data.
 */
#define PREPARE_SETUP_PACKET_LOHI(name, p_direction, p_type, p_recipient, \
    p_request, p_value_low, p_value_high, p_index, p_length) \
	PREPARE_SETUP_PACKET(name, p_direction, p_type, p_recipient, \
	    p_request, (p_value_low) | ((p_value_high) << 8), \
	    p_index, p_length)


/** Retrieve USB descriptor of a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[out] buffer Buffer where to store the retrieved descriptor.
 * @param[in] size Size of the @p buffer.
 * @param[out] actual_size Number of bytes actually transferred.
 * @return Error code.
 */
int usb_request_get_descriptor(usb_endpoint_pipe_t *pipe,
    usb_request_type_t request_type,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language,
    void *buffer, size_t size, size_t *actual_size)
{
	if (buffer == NULL) {
		return EBADMEM;
	}
	if (size == 0) {
		return EINVAL;
	}

	PREPARE_SETUP_PACKET_LOHI(setup_packet, USB_DIRECTION_IN,
	    request_type, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_GET_DESCRIPTOR, descriptor_index, descriptor_type,
	    language, size);

	int rc = usb_endpoint_pipe_control_read(pipe,
	    &setup_packet, sizeof(setup_packet),
	    buffer, size, actual_size);

	return rc;
}

/**
 * @}
 */
