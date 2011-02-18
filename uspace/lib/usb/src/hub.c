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
 * Functions needed by hub drivers.
 */
#include <usb/hub.h>
#include <usbhc_iface.h>
#include <errno.h>

/** Check that HC connection is alright.
 *
 * @param conn Connection to be checked.
 */
#define CHECK_CONNECTION(conn) \
	do { \
		assert((conn)); \
		if (!usb_hc_connection_is_opened((conn))) { \
			return ENOENT; \
		} \
	} while (false)


/** Tell host controller to reserve default address.
 *
 * @param connection Opened connection to host controller.
 * @return Error code.
 */
int usb_hc_reserve_default_address(usb_hc_connection_t *connection,
    bool full_speed)
{
	CHECK_CONNECTION(connection);

	return async_req_2_0(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RESERVE_DEFAULT_ADDRESS, full_speed);
}

/** Tell host controller to release default address.
 *
 * @param connection Opened connection to host controller.
 * @return Error code.
 */
int usb_hc_release_default_address(usb_hc_connection_t *connection)
{
	CHECK_CONNECTION(connection);

	return async_req_1_0(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RELEASE_DEFAULT_ADDRESS);
}

/** Ask host controller for free address assignment.
 *
 * @param connection Opened connection to host controller.
 * @return Assigned USB address or negative error code.
 */
usb_address_t usb_hc_request_address(usb_hc_connection_t *connection,
    bool full_speed)
{
	CHECK_CONNECTION(connection);

	sysarg_t address;
	int rc = async_req_2_1(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_REQUEST_ADDRESS, full_speed,
	    &address);
	if (rc != EOK) {
		return (usb_address_t) rc;
	} else {
		return (usb_address_t) address;
	}
}

/** Inform host controller about new device.
 *
 * @param connection Opened connection to host controller.
 * @param attached_device Information about the new device.
 * @return Error code.
 */
int usb_hc_register_device(usb_hc_connection_t * connection,
    const usb_hc_attached_device_t *attached_device)
{
	CHECK_CONNECTION(connection);
	if (attached_device == NULL) {
		return EBADMEM;
	}

	return async_req_3_0(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_BIND_ADDRESS,
	    attached_device->address, attached_device->handle);
}

/** Inform host controller about device removal.
 *
 * @param connection Opened connection to host controller.
 * @param address Address of the device that is being removed.
 * @return Error code.
 */
int usb_hc_unregister_device(usb_hc_connection_t *connection,
    usb_address_t address)
{
	CHECK_CONNECTION(connection);

	return async_req_2_0(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_RELEASE_ADDRESS, address);
}


/**
 * @}
 */
