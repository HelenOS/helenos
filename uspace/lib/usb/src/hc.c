/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * General communication with host controller driver (implementation).
 */

#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <usbhc_iface.h>
#include <usb/dev.h>
#include <usb/hc.h>

static int usb_hc_connection_add_ref(usb_hc_connection_t *connection)
{
	assert(connection);
	
	fibril_mutex_lock(&connection->guard);
	if (connection->ref_count == 0) {
		assert(connection->hc_sess == NULL);
		/* Parallel exchange for us */
		connection->hc_sess = devman_device_connect(EXCHANGE_PARALLEL,
		        connection->hc_handle, 0);
		if (!connection->hc_sess) {
			fibril_mutex_unlock(&connection->guard);
			return ENOMEM;
		}
	}
	
	++connection->ref_count;
	fibril_mutex_unlock(&connection->guard);
	return EOK;
}

static int usb_hc_connection_del_ref(usb_hc_connection_t *connection)
{
	assert(connection);
	
	fibril_mutex_lock(&connection->guard);
	if (connection->ref_count == 0) {
		/* Closing already closed connection... */
		assert(connection->hc_sess == NULL);
		fibril_mutex_unlock(&connection->guard);
		return EOK;
	}
	
	--connection->ref_count;
	int ret = EOK;
	if (connection->ref_count == 0) {
		assert(connection->hc_sess);
		ret = async_hangup(connection->hc_sess);
		connection->hc_sess = NULL;
	}
	fibril_mutex_unlock(&connection->guard);
	return ret;
}

#define EXCH_INIT(connection, exch) \
do { \
	exch = NULL; \
	if (!connection) \
		return EBADMEM; \
	const int ret = usb_hc_connection_add_ref(connection); \
	if (ret != EOK) \
		return ret; \
	exch = async_exchange_begin(connection->hc_sess); \
	if (exch == NULL) { \
		usb_hc_connection_del_ref(connection); \
		return ENOMEM; \
	} \
} while (0)

#define EXCH_FINI(connection, exch) \
if (exch) { \
	async_exchange_end(exch); \
	usb_hc_connection_del_ref(connection); \
} else (void)0

/** Initialize connection to USB host controller.
 *
 * @param connection Connection to be initialized.
 * @param device Device connecting to the host controller.
 * @return Error code.
 */
int usb_hc_connection_initialize_from_device(usb_hc_connection_t *connection,
    ddf_dev_t *device)
{
	if (device == NULL)
		return EBADMEM;

	devman_handle_t hc_handle;
	const int rc = usb_get_hc_by_handle(ddf_dev_get_handle(device), &hc_handle);
	if (rc == EOK) {
		usb_hc_connection_initialize(connection, hc_handle);
	}

	return rc;
}

void usb_hc_connection_deinitialize(usb_hc_connection_t *connection)
{
	assert(connection);
	fibril_mutex_lock(&connection->guard);
	if (connection->ref_count != 0) {
		usb_log_warning("%u stale reference(s) to HC connection.\n",
		    connection->ref_count);
		assert(connection->hc_sess);
		async_hangup(connection->hc_sess);
		connection->hc_sess = NULL;
		connection->ref_count = 0;
	}
	fibril_mutex_unlock(&connection->guard);
}

/** Open connection to host controller.
 *
 * @param connection Connection to the host controller.
 * @return Error code.
 */
int usb_hc_connection_open(usb_hc_connection_t *connection)
{
	return usb_hc_connection_add_ref(connection);
}

/** Close connection to the host controller.
 *
 * @param connection Connection to the host controller.
 * @return Error code.
 */
int usb_hc_connection_close(usb_hc_connection_t *connection)
{
	return usb_hc_connection_del_ref(connection);
}

/** Ask host controller for free address assignment.
 *
 * @param connection Opened connection to host controller.
 * @param preferred Preferred SUB address.
 * @param strict Fail if the preferred address is not avialable.
 * @param speed Speed of the new device (device that will be assigned
 *    the returned address).
 * @return Assigned USB address or negative error code.
 */
usb_address_t usb_hc_request_address(usb_hc_connection_t *connection,
    usb_address_t preferred, bool strict, usb_speed_t speed)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	usb_address_t address = preferred;
	const int ret = usbhc_request_address(exch, &address, strict, speed);

	EXCH_FINI(connection, exch);
	return ret == EOK ? address : ret;
}

int usb_hc_bind_address(usb_hc_connection_t * connection,
    usb_address_t address, devman_handle_t handle)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret = usbhc_bind_address(exch, address, handle);

	EXCH_FINI(connection, exch);
	return ret;
}

/** Get handle of USB device with given address.
 *
 * @param[in] connection Opened connection to host controller.
 * @param[in] address Address of device in question.
 * @param[out] handle Where to write the device handle.
 * @return Error code.
 */
int usb_hc_get_handle_by_address(usb_hc_connection_t *connection,
    usb_address_t address, devman_handle_t *handle)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret = usbhc_get_handle(exch, address, handle);

	EXCH_FINI(connection, exch);
	return ret;
}

int usb_hc_release_address(usb_hc_connection_t *connection,
    usb_address_t address)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret = usbhc_release_address(exch, address);

	EXCH_FINI(connection, exch);
	return ret;
}

int usb_hc_register_endpoint(usb_hc_connection_t *connection,
    usb_address_t address, usb_endpoint_t endpoint, usb_transfer_type_t type,
    usb_direction_t direction, size_t packet_size, unsigned interval)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret = usbhc_register_endpoint(exch, address, endpoint,
	    type, direction, packet_size, interval);

	EXCH_FINI(connection, exch);
	return ret;
}

int usb_hc_unregister_endpoint(usb_hc_connection_t *connection,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret =
	    usbhc_unregister_endpoint(exch, address, endpoint, direction);

	EXCH_FINI(connection, exch);
	return ret;
}

int usb_hc_read(usb_hc_connection_t *connection, usb_address_t address,
    usb_endpoint_t endpoint, uint64_t setup, void *data, size_t size,
    size_t *real_size)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret =
	    usbhc_read(exch, address, endpoint, setup, data, size, real_size);

	EXCH_FINI(connection, exch);
	return ret;
}

int usb_hc_write(usb_hc_connection_t *connection, usb_address_t address,
    usb_endpoint_t endpoint, uint64_t setup, const void *data, size_t size)
{
	async_exch_t *exch;
	EXCH_INIT(connection, exch);

	const int ret = usbhc_write(exch, address, endpoint, setup, data, size);

	EXCH_FINI(connection, exch);
	return ret;
}

/**
 * @}
 */
