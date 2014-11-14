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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Functions needed by hub drivers.
 */

#include <usb/dev/hub.h>
#include <usb/dev/pipes.h>
#include <usb/dev/request.h>
#include <usb/dev/recognise.h>
#include <usb/debug.h>
#include <errno.h>
#include <assert.h>
#include <usb/debug.h>
#include <time.h>
#include <async.h>

/** How much time to wait between attempts to get the default address.
 * The value is based on typical value for port reset + some overhead.
 */
#define DEFAULT_ADDRESS_ATTEMPT_DELAY_USEC (1000 * (10 + 2))

/** Inform host controller about new device.
 *
 * @param connection Opened connection to host controller.
 * @param attached_device Information about the new device.
 * @return Error code.
 */
int usb_hub_register_device(usb_hc_connection_t *connection,
    const usb_hub_attached_device_t *attached_device)
{
	assert(connection);
	if (attached_device == NULL || attached_device->fun == NULL)
		return EBADMEM;
	return usb_hc_bind_address(connection,
	    attached_device->address, ddf_fun_get_handle(attached_device->fun));
}

/** Change address of connected device.
 * This function automatically updates the backing connection to point to
 * the new address. It also unregisterrs the old endpoint and registers
 * a new one.
 * This creates whole bunch of problems:
 *  1. All pipes using this wire are broken because they are not
 *     registered for new address
 *  2. All other pipes for this device are using wrong address,
 *     possibly targeting completely different device
 *
 * @param pipe Control endpoint pipe (session must be already started).
 * @param new_address New USB address to be set (in native endianness).
 * @return Error code.
 */
static int usb_request_set_address(usb_pipe_t *pipe, usb_address_t new_address)
{
	if ((new_address < 0) || (new_address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}
	assert(pipe);
	assert(pipe->wire != NULL);

	const uint16_t addr = uint16_host2usb((uint16_t)new_address);

	int rc = usb_control_request_set(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_ADDRESS, addr, 0, NULL, 0);

	if (rc != EOK) {
		return rc;
	}

	/* TODO: prevent others from accessing the wire now. */
	if (usb_pipe_unregister(pipe) != EOK) {
		usb_log_warning(
		    "Failed to unregister the old pipe on address change.\n");
	}
	/* Address changed. We can release the old one, thus
	 * allowing other to us it. */
	usb_hc_release_address(pipe->wire->hc_connection, pipe->wire->address);

	/* The address is already changed so set it in the wire */
	pipe->wire->address = new_address;
	rc = usb_pipe_register(pipe, 0);
	if (rc != EOK)
		return EADDRNOTAVAIL;

	return EOK;
}

/** Wrapper for registering attached device to the hub.
 *
 * The @p enable_port function is expected to enable signaling on given
 * port.
 * The argument can have arbitrary meaning and it is not touched at all
 * by this function (it is passed as is to the @p enable_port function).
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
 * @param[in] connection Connection to host controller. Must be non-null.
 * @param[in] dev_speed New device speed.
 * @param[in] enable_port Function for enabling signaling through the port the
 *	device is attached to.
 * @param[in] arg Any data argument to @p enable_port.
 * @param[out] assigned_address USB address of the device.
 * @param[in] dev_ops Child device ops. Will use default if not provided.
 * @param[in] new_dev_data Arbitrary pointer to be stored in the child
 *	as @c driver_data. Will allocate and assign usb_hub_attached_device_t
 *	structure if NULL.
 * @param[out] new_fun Storage where pointer to allocated child function
 *	will be written. Must be non-null.
 * @return Error code.
 * @retval EINVAL Either connection or new_fun is a NULL pointer.
 * @retval ENOENT Connection to HC not opened.
 * @retval EADDRNOTAVAIL Failed retrieving free address from host controller.
 * @retval EBUSY Failed reserving default USB address.
 * @retval ENOTCONN Problem connecting to the host controller via USB pipe.
 * @retval ESTALL Problem communication with device (either SET_ADDRESS
 *	request or requests for descriptors when creating match ids).
 */
int usb_hc_new_device_wrapper(ddf_dev_t *parent, ddf_fun_t *fun,
    usb_hc_connection_t *hc_conn, usb_speed_t dev_speed,
    int (*enable_port)(void *arg), void *arg, usb_address_t *assigned_address,
    ddf_dev_ops_t *dev_ops)
{
	if (hc_conn == NULL)
		return EINVAL;
	
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	
	/* We are gona do a lot of communication better open it in advance. */
	int rc = usb_hc_connection_open(hc_conn);
	if (rc != EOK)
		return rc;
	
	/* Request a new address. */
	usb_address_t dev_addr =
	    usb_hc_request_address(hc_conn, 0, false, dev_speed);
	if (dev_addr < 0) {
		rc = EADDRNOTAVAIL;
		goto close_connection;
	}

	/* Initialize connection to device. */
	usb_device_connection_t dev_conn;
	rc = usb_device_connection_initialize(
	    &dev_conn, hc_conn, USB_ADDRESS_DEFAULT);
	if (rc != EOK) {
		rc = ENOTCONN;
		goto leave_release_free_address;
	}

	/* Initialize control pipe on default address. Don't register yet. */
	usb_pipe_t ctrl_pipe;
	rc = usb_pipe_initialize_default_control(&ctrl_pipe, &dev_conn);
	if (rc != EOK) {
		rc = ENOTCONN;
		goto leave_release_free_address;
	}

	/*
	 * The default address request might fail.
	 * That means that someone else is already using that address.
	 * We will simply wait and try again.
	 * (Someone else already wants to add a new device.)
	 */
	do {
		rc = usb_hc_request_address(hc_conn, USB_ADDRESS_DEFAULT,
		    true, dev_speed);
		if (rc == ENOENT) {
			/* Do not overheat the CPU ;-). */
			async_usleep(DEFAULT_ADDRESS_ATTEMPT_DELAY_USEC);
		}
	} while (rc == ENOENT);
	if (rc < 0) {
		goto leave_release_free_address;
	}

	/* Register control pipe on default address. 0 means no interval. */
	rc = usb_pipe_register(&ctrl_pipe, 0);
	if (rc != EOK) {
		rc = ENOTCONN;
		goto leave_release_default_address;
	}
	
	struct timeval end_time;
	gettimeofday(&end_time, NULL);
	
	/* According to the USB spec part 9.1.2 host allows 100ms time for
	 * the insertion process to complete. According to 7.1.7.1 this is the
	 * time between attach detected and port reset. However, the setup done
	 * above might use much of this time so we should only wait to fill
	 * up the 100ms quota*/
	const suseconds_t elapsed = tv_sub(&end_time, &start_time);
	if (elapsed < 100000) {
		async_usleep(100000 - elapsed);
	}

	/* Endpoint is registered. We can enable the port and change address. */
	rc = enable_port(arg);
	if (rc != EOK) {
		goto leave_release_default_address;
	}
	/* USB spec 7.1.7.1: The USB System Software guarantees a minimum of
	 * 10ms for reset recovery. Device response to any bus transactions
	 * addressed to the default device address during the reset recovery
	 * time is undefined.
	 */
	async_usleep(10000);

	/* Get max_packet_size value. */
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


	/* Register the device with devman. */
	/* FIXME: create device_register that will get opened ctrl pipe. */
	rc = usb_device_register_child_in_devman(&ctrl_pipe,
	    parent, fun, dev_ops);
	if (rc != EOK) {
		goto leave_release_free_address;
	}

	const usb_hub_attached_device_t new_device = {
		.address = dev_addr,
		.fun = fun,
	};


	/* Inform the host controller about the handle. */
	rc = usb_hub_register_device(hc_conn, &new_device);
	if (rc != EOK) {
		/* The child function is already created. */
		rc = EDESTADDRREQ;
		goto leave_release_free_address;
	}

	if (assigned_address != NULL) {
		*assigned_address = dev_addr;
	}

	rc = EOK;
	goto close_connection;

	/*
	 * Error handling (like nested exceptions) starts here.
	 * Completely ignoring errors here.
	 */
leave_release_default_address:
	if (usb_hc_release_address(hc_conn, USB_ADDRESS_DEFAULT) != EOK)
		usb_log_warning("%s: Failed to release defaut address.\n",
		    __FUNCTION__);

leave_release_free_address:
	/* This might be either 0:0 or dev_addr:0 */
	if (usb_pipe_unregister(&ctrl_pipe) != EOK)
		usb_log_warning("%s: Failed to unregister default pipe.\n",
		    __FUNCTION__);

	if (usb_hc_release_address(hc_conn, dev_addr) != EOK)
		usb_log_warning("%s: Failed to release address: %d.\n",
		    __FUNCTION__, dev_addr);

close_connection:
	if (usb_hc_connection_close(hc_conn) != EOK)
		usb_log_warning("%s: Failed to close hc connection.\n",
		    __FUNCTION__);

	return rc;
}

/**
 * @}
 */
