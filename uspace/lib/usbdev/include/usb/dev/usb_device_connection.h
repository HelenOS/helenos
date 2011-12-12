/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Common USB types and functions.
 */
#ifndef LIBUSBDEV_DEVICE_CONNECTION_H_
#define LIBUSBDEV_DEVICE_CONNECTION_H_

#include <errno.h>
#include <devman.h>
#include <usb/usb.h>
#include <usb/hc.h>


/** Abstraction of a physical connection to the device.
 * This type is an abstraction of the USB wire that connects the host and
 * the function (device).
 */
typedef struct {
	/** Connection to the host controller device is connected to. */
	usb_hc_connection_t *hc_connection;
	/** Address of the device. */
	usb_address_t address;
} usb_device_connection_t;

static inline int usb_device_connection_initialize(
    usb_device_connection_t *connection, usb_hc_connection_t *hc_connection,
    usb_address_t address)
{
	assert(connection);

	if (hc_connection == NULL) {
		return EBADMEM;
	}

	if ((address < 0) || (address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}

	connection->hc_connection = hc_connection;
	connection->address = address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Initialize connection to USB device on default address.
 *
 * @param dev_connection Device connection structure to be initialized.
 * @param hc_connection Initialized connection to host controller.
 * @return Error code.
 */
static inline int usb_device_connection_initialize_on_default_address(
    usb_device_connection_t *connection, usb_hc_connection_t *hc_conn)
{
	return usb_device_connection_initialize(connection, hc_conn, 0);
}
#endif
/**
 * @}
 */
