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
 * Transfer handling.
 */
#include <usbvirt/device.h>
#include <usb/debug.h>
#include <errno.h>
#include <assert.h>
#include "private.h"

/** Process a control transfer to the virtual USB device.
 *
 * @param dev Target device.
 * @param setup Setup packet data.
 * @param setup_size Size of setup packet.
 * @param data Extra data (DATA stage).
 * @param data_size Size of extra data in bytes.
 * @param data_size_sent Number of actually send bytes during the transfer
 *	(only used for READ transfers).
 * @return Error code.
 */
static errno_t usbvirt_control_transfer(usbvirt_device_t *dev,
    const void *setup, size_t setup_size,
    void *data, size_t data_size, size_t *data_size_sent)
{
	assert(dev);
	assert(dev->ops);

	if (setup_size != sizeof(usb_device_request_setup_packet_t)) {
		return ESTALL;
	}
	const usb_device_request_setup_packet_t *setup_packet = setup;
	if (data_size != setup_packet->length) {
		return ESTALL;
	}

	errno_t rc;

	/* Run user handler first. */
	rc = process_control_transfer(dev, dev->ops->control,
	    setup_packet, data, data_size_sent);
	if (rc != EFORWARD) {
		return rc;
	}

	/* Run the library handlers afterwards. */
	rc = process_control_transfer(dev, library_handlers,
	    setup_packet, data, data_size_sent);

	if (rc == EFORWARD) {
		usb_log_warning("Control transfer {%s (%s)} not handled.",
		    usb_debug_str_buffer(setup, setup_size, 10),
		    setup_packet->request_type & 0x80
		    ? "IN" : usb_debug_str_buffer(data, data_size, 10));
		rc = EBADCHECKSUM;
	}

	return rc;
}

/** Issue a control write transfer to virtual USB device.
 *
 * @see usbvirt_control_transfer
 *
 * @param dev Target virtual device.
 * @param setup Setup data.
 * @param setup_size Size of setup packet.
 * @param data Extra data (DATA stage).
 * @param data_size Size of extra data buffer in bytes.
 * @return Error code.
 */
errno_t usbvirt_control_write(usbvirt_device_t *dev, const void *setup,
    size_t setup_size, void *data, size_t data_size)
{
	return usbvirt_control_transfer(dev, setup, setup_size,
	    data, data_size, NULL);
}

/** Issue a control read transfer to virtual USB device.
 *
 * @see usbvirt_control_transfer
 *
 * @param dev Target virtual device.
 * @param setup Setup data.
 * @param setup_size Size of setup packet.
 * @param data Extra data (DATA stage).
 * @param data_size Size of extra data buffer in bytes.
 * @param data_size_sent Number of actually send bytes during the transfer.
 * @return Error code.
 */
errno_t usbvirt_control_read(usbvirt_device_t *dev,
    const void *setup, size_t setup_size,
    void *data, size_t data_size, size_t *data_size_sent)
{
	return usbvirt_control_transfer(dev, setup, setup_size,
	    data, data_size, data_size_sent);
}

/** Send data to virtual USB device.
 *
 * @param dev Target virtual device.
 * @param transf_type Transfer type (interrupt, bulk).
 * @param endpoint Endpoint number.
 * @param data Data sent from the driver to the device.
 * @param data_size Size of the @p data buffer in bytes.
 * @return Error code.
 */
errno_t usbvirt_data_out(usbvirt_device_t *dev, usb_transfer_type_t transf_type,
    usb_endpoint_t endpoint, const void *data, size_t data_size)
{
	if ((endpoint <= 0) || (endpoint >= USBVIRT_ENDPOINT_MAX)) {
		return ERANGE;
	}
	if ((dev->ops == NULL) || (dev->ops->data_out[endpoint] == NULL)) {
		return ENOTSUP;
	}

	errno_t rc = dev->ops->data_out[endpoint](dev, endpoint, transf_type,
	    data, data_size);

	return rc;
}

/** Request data from virtual USB device.
 *
 * @param dev Target virtual device.
 * @param transf_type Transfer type (interrupt, bulk).
 * @param endpoint Endpoint number.
 * @param data Where to stored data the device returns to the driver.
 * @param data_size Size of the @p data buffer in bytes.
 * @param data_size_sent Number of actually written bytes.
 * @return Error code.
 */
errno_t usbvirt_data_in(usbvirt_device_t *dev, usb_transfer_type_t transf_type,
    usb_endpoint_t endpoint, void *data, size_t data_size, size_t *data_size_sent)
{
	if ((endpoint <= 0) || (endpoint >= USBVIRT_ENDPOINT_MAX)) {
		return ERANGE;
	}
	if ((dev->ops == NULL) || (dev->ops->data_in[endpoint] == NULL)) {
		return ENOTSUP;
	}

	size_t data_size_sent_tmp;
	errno_t rc = dev->ops->data_in[endpoint](dev, endpoint, transf_type,
	    data, data_size, &data_size_sent_tmp);

	if (rc != EOK) {
		return rc;
	}

	if (data_size_sent != NULL) {
		*data_size_sent = data_size_sent_tmp;
	}

	return EOK;
}

/**
 * @}
 */
