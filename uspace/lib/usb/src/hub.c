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
#include <usb/pipes.h>
#include <usb/request.h>
#include <usb/recognise.h>
#include <usbhc_iface.h>
#include <errno.h>
#include <assert.h>
#include <usb/debug.h>

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

/** Ask host controller for free address assignment.
 *
 * @param connection Opened connection to host controller.
 * @param speed Speed of the new device (device that will be assigned
 *    the returned address).
 * @return Assigned USB address or negative error code.
 */
usb_address_t usb_hc_request_address(usb_hc_connection_t *connection,
    usb_speed_t speed)
{
	CHECK_CONNECTION(connection);

	sysarg_t address;
	int rc = async_req_2_1(connection->hc_phone,
	    DEV_IFACE_ID(USBHC_DEV_IFACE),
	    IPC_M_USBHC_REQUEST_ADDRESS, speed,
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

static void unregister_control_endpoint_on_default_address(
    usb_hc_connection_t *connection)
{
	usb_device_connection_t dev_conn;
	int rc = usb_device_connection_initialize_on_default_address(&dev_conn,
	    connection);
	if (rc != EOK) {
		return;
	}

	usb_pipe_t ctrl_pipe;
	rc = usb_pipe_initialize_default_control(&ctrl_pipe, &dev_conn);
	if (rc != EOK) {
		return;
	}

	usb_pipe_unregister(&ctrl_pipe, connection);
}


/** Wrapper for registering attached device to the hub.
 *
 * The @p enable_port function is expected to enable signaling on given
 * port.
 * The two arguments to it can have arbitrary meaning
 * (the @p port_no is only a suggestion)
 * and are not touched at all by this function
 * (they are passed as is to the @p enable_port function).
 *
 * If the @p enable_port fails (i.e. does not return EOK), the device
 * addition is canceled.
 * The return value is then returned (it is good idea to use different
 * error codes than those listed as return codes by this function itself).
 *
 * The @p connection representing connection with host controller does not
 * need to be started.
 * This function duplicates the connection to allow simultaneous calls of
 * this function (i.e. from different fibrils).
 *
 * @param[in] parent Parent device (i.e. the hub device).
 * @param[in] connection Connection to host controller.
 * @param[in] dev_speed New device speed.
 * @param[in] enable_port Function for enabling signaling through the port the
 *	device is attached to.
 * @param[in] port_no Port number (passed through to @p enable_port).
 * @param[in] arg Any data argument to @p enable_port.
 * @param[out] assigned_address USB address of the device.
 * @param[out] assigned_handle Devman handle of the new device.
 * @param[in] dev_ops Child device ops.
 * @param[in] new_dev_data Arbitrary pointer to be stored in the child
 *	as @c driver_data.
 * @param[out] new_fun Storage where pointer to allocated child function
 *	will be written.
 * @return Error code.
 * @retval ENOENT Connection to HC not opened.
 * @retval EADDRNOTAVAIL Failed retrieving free address from host controller.
 * @retval EBUSY Failed reserving default USB address.
 * @retval ENOTCONN Problem connecting to the host controller via USB pipe.
 * @retval ESTALL Problem communication with device (either SET_ADDRESS
 *	request or requests for descriptors when creating match ids).
 */
int usb_hc_new_device_wrapper(ddf_dev_t *parent, usb_hc_connection_t *connection,
    usb_speed_t dev_speed,
    int (*enable_port)(int port_no, void *arg), int port_no, void *arg,
    usb_address_t *assigned_address, devman_handle_t *assigned_handle,
    ddf_dev_ops_t *dev_ops, void *new_dev_data, ddf_fun_t **new_fun)
{
	assert(connection != NULL);
	// FIXME: this is awful, we are accessing directly the structure.
	usb_hc_connection_t hc_conn = {
		.hc_handle = connection->hc_handle,
		.hc_phone = -1
	};

	int rc;

	rc = usb_hc_connection_open(&hc_conn);
	if (rc != EOK) {
		return rc;
	}


	/*
	 * Request new address.
	 */
	usb_address_t dev_addr = usb_hc_request_address(&hc_conn, dev_speed);
	if (dev_addr < 0) {
		usb_hc_connection_close(&hc_conn);
		return EADDRNOTAVAIL;
	}

	/*
	 * We will not register control pipe on default address.
	 * The registration might fail. That means that someone else already
	 * registered that endpoint. We will simply wait and try again.
	 * (Someone else already wants to add a new device.)
	 */
	usb_device_connection_t dev_conn;
	rc = usb_device_connection_initialize_on_default_address(&dev_conn,
	    &hc_conn);
	if (rc != EOK) {
		rc = ENOTCONN;
		goto leave_release_free_address;
	}

	usb_pipe_t ctrl_pipe;
	rc = usb_pipe_initialize_default_control(&ctrl_pipe,
	    &dev_conn);
	if (rc != EOK) {
		rc = ENOTCONN;
		goto leave_release_free_address;
	}

	do {
		rc = usb_pipe_register_with_speed(&ctrl_pipe, dev_speed, 0,
		    &hc_conn);
		if (rc != EOK) {
			/* Do not overheat the CPU ;-). */
			async_usleep(10);
		}
	} while (rc != EOK);

	/*
	 * Endpoint is registered. We can enable the port and change
	 * device address.
	 */
	rc = enable_port(port_no, arg);
	if (rc != EOK) {
		goto leave_release_default_address;
	}

	rc = usb_pipe_probe_default_control(&ctrl_pipe);
	if (rc != EOK) {
		rc = ESTALL;
		goto leave_release_default_address;
	}

	rc = usb_request_set_address(&ctrl_pipe, dev_addr);
	if (rc != EOK) {
		rc = ESTALL;
		goto leave_release_default_address;
	}

	/*
	 * Address changed. We can release the original endpoint, thus
	 * allowing other to access the default address.
	 */
	unregister_control_endpoint_on_default_address(&hc_conn);

	/*
	 * Time to register the new endpoint.
	 */
	rc = usb_pipe_register(&ctrl_pipe, 0, &hc_conn);
	if (rc != EOK) {
		goto leave_release_free_address;
	}

	/*
	 * It is time to register the device with devman.
	 */
	/* FIXME: create device_register that will get opened ctrl pipe. */
	devman_handle_t child_handle;
	rc = usb_device_register_child_in_devman(dev_addr, dev_conn.hc_handle,
	    parent, &child_handle,
	    dev_ops, new_dev_data, new_fun);
	if (rc != EOK) {
		rc = ESTALL;
		goto leave_release_free_address;
	}

	/*
	 * And now inform the host controller about the handle.
	 */
	usb_hc_attached_device_t new_device = {
		.address = dev_addr,
		.handle = child_handle
	};
	rc = usb_hc_register_device(&hc_conn, &new_device);
	if (rc != EOK) {
		rc = EDESTADDRREQ;
		goto leave_release_free_address;
	}

	/*
	 * And we are done.
	 */
	if (assigned_address != NULL) {
		*assigned_address = dev_addr;
	}
	if (assigned_handle != NULL) {
		*assigned_handle = child_handle;
	}

	return EOK;



	/*
	 * Error handling (like nested exceptions) starts here.
	 * Completely ignoring errors here.
	 */
leave_release_default_address:
	usb_pipe_unregister(&ctrl_pipe, &hc_conn);

leave_release_free_address:
	usb_hc_unregister_device(&hc_conn, dev_addr);

	usb_hc_connection_close(&hc_conn);

	return rc;
}

/**
 * @}
 */
