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
 * General communication between device drivers and host controller driver.
 */
#include <devman.h>
#include <async.h>
#include <usb_iface.h>
#include <usb/dev/hc.h>
#include <usb/driver.h>
#include <usb/debug.h>
#include <errno.h>
#include <assert.h>

/** Initialize connection to USB host controller.
 *
 * @param connection Connection to be initialized.
 * @param device Device connecting to the host controller.
 * @return Error code.
 */
int usb_hc_connection_initialize_from_device(usb_hc_connection_t *connection,
    ddf_dev_t *device)
{
	assert(connection);

	if (device == NULL) {
		return EBADMEM;
	}

	devman_handle_t hc_handle;
	int rc = usb_hc_find(device->handle, &hc_handle);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_hc_connection_initialize(connection, hc_handle);

	return rc;
}

/** Manually initialize connection to USB host controller.
 *
 * @param connection Connection to be initialized.
 * @param hc_handle Devman handle of the host controller.
 * @return Error code.
 */
int usb_hc_connection_initialize(usb_hc_connection_t *connection,
    devman_handle_t hc_handle)
{
	assert(connection);

	connection->hc_handle = hc_handle;
	connection->hc_phone = -1;

	return EOK;
}

/** Open connection to host controller.
 *
 * @param connection Connection to the host controller.
 * @return Error code.
 */
int usb_hc_connection_open(usb_hc_connection_t *connection)
{
	assert(connection);

	if (usb_hc_connection_is_opened(connection)) {
		return EBUSY;
	}

	int phone = devman_device_connect(connection->hc_handle, 0);
	if (phone < 0) {
		return phone;
	}

	connection->hc_phone = phone;

	return EOK;
}

/** Tells whether connection to host controller is opened.
 *
 * @param connection Connection to the host controller.
 * @return Whether connection is opened.
 */
bool usb_hc_connection_is_opened(const usb_hc_connection_t *connection)
{
	assert(connection);

	return (connection->hc_phone >= 0);
}

/** Close connection to the host controller.
 *
 * @param connection Connection to the host controller.
 * @return Error code.
 */
int usb_hc_connection_close(usb_hc_connection_t *connection)
{
	assert(connection);

	if (!usb_hc_connection_is_opened(connection)) {
		return ENOENT;
	}

	int rc = async_hangup(connection->hc_phone);
	if (rc != EOK) {
		return rc;
	}

	connection->hc_phone = -1;

	return EOK;
}

/**
 * @}
 */
