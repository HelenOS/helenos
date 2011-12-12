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

/*----------------------------------------------------------------------------*/
static inline int usb_device_register_endpoint(usb_device_connection_t *conn,
    usb_endpoint_t ep, usb_transfer_type_t type, usb_direction_t direction,
    size_t packet_size, unsigned interval)
{
	assert(conn);
	return usb_hc_register_endpoint(conn->hc_connection,
	    conn->address, ep, type, direction, packet_size, interval);
}
/*----------------------------------------------------------------------------*/
static inline int usb_device_unregister_endpoint(usb_device_connection_t *conn,
    usb_endpoint_t ep, usb_direction_t direction)
{
	assert(conn);
	return usb_hc_unregister_endpoint(conn->hc_connection,
	    conn->address, ep, direction);
}
/*----------------------------------------------------------------------------*/
static inline int usb_device_control_read(usb_device_connection_t *conn,
    usb_endpoint_t ep, uint64_t setup, void *data, size_t size, size_t *rsize)
{
	assert(conn);
	return usb_hc_control_read(conn->hc_connection,
	    conn->address, ep, setup, data, size, rsize);
}
/*----------------------------------------------------------------------------*/
static inline int usb_device_control_write(usb_device_connection_t *conn,
    usb_endpoint_t ep, uint64_t setup, const void *data, size_t size)
{
	assert(conn);
	return usb_hc_control_write(conn->hc_connection,
	    conn->address, ep, setup, data, size);
}
/*----------------------------------------------------------------------------*/
/** Wrapper for read calls with no setup stage.
 * @param[in] connection hc connection to use.
 * @param[in] address USB device address.
 * @param[in] endpoint USB device endpoint.
 * @param[in] data Data buffer.
 * @param[in] size Size of the buffer.
 * @param[out] real_size Size of the transferred data.
 * @return Error code.
 */
static inline int usb_device_read(usb_device_connection_t *conn,
    usb_endpoint_t ep, void *data, size_t size, size_t *real_size)
{
	return usb_device_control_read(conn, ep, 0, data, size, real_size);
}
/*----------------------------------------------------------------------------*/
/** Wrapper for write calls with no setup stage.
 * @param connection hc connection to use.
 * @param address USB device address.
 * @param endpoint USB device endpoint.
 * @param data Data buffer.
 * @param size Size of the buffer.
 * @return Error code.
 */
static inline int usb_device_write(usb_device_connection_t *conn,
    usb_endpoint_t ep, const void *data, size_t size)
{
	return usb_device_control_write(conn, ep, 0, data, size);
}
#endif
/**
 * @}
 */
