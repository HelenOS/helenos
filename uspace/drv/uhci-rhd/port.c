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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>    /* usb_address_t */
#include <usb/usbdevice.h>
#include <usb/hub.h>
#include <usb/request.h>
#include <usb/debug.h>
#include <usb/recognise.h>

#include "port.h"
#include "port_status.h"

static int uhci_port_new_device(uhci_port_t *port);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static int uhci_port_check(void *port);

int uhci_port_init(
  uhci_port_t *port, port_status_t *address, unsigned number,
  unsigned usec, device_t *rh, int parent_phone)
{
	assert(port);
	port->address = address;
	port->number = number;
	port->wait_period_usec = usec;
	port->attached_device = 0;
	port->rh = rh;
	int rc = usb_hc_connection_initialize_from_device(
	    &port->hc_connection, rh);
	if (rc != EOK) {
		usb_log_error("Failed to initialize connection to HC.");
		return rc;
	}

	port->checker = fibril_create(uhci_port_check, port);
	if (port->checker == 0) {
		usb_log_error(": failed to launch root hub fibril.");
		return ENOMEM;
	}
	fibril_add_ready(port->checker);
	usb_log_debug(
	  "Added fibril for port %d: %p.\n", number, port->checker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
void uhci_port_fini(uhci_port_t *port)
{
// TODO: destroy fibril
// TODO: hangup phone
//	fibril_teardown(port->checker);
	return;
}
/*----------------------------------------------------------------------------*/
int uhci_port_check(void *port)
{
	uhci_port_t *port_instance = port;
	assert(port_instance);

	while (1) {
		usb_log_debug("Port(%d) status address %p:\n",
		  port_instance->number, port_instance->address);

		/* read register value */
		port_status_t port_status =
			port_status_read(port_instance->address);

		/* debug print */
		usb_log_info("Port(%d) status %#.4x\n",
		  port_instance->number, port_status);
		print_port_status(port_status);

		if (port_status & STATUS_CONNECTED_CHANGED) {
			int rc = usb_hc_connection_open(
			    &port_instance->hc_connection);
			if (rc != EOK) {
				usb_log_error("Failed to connect to HC.");
				goto next;
			}

			if (port_status & STATUS_CONNECTED) {
				/* new device */
				uhci_port_new_device(port_instance);
			} else {
				uhci_port_remove_device(port_instance);
			}

			rc = usb_hc_connection_close(
			    &port_instance->hc_connection);
			if (rc != EOK) {
				usb_log_error("Failed to disconnect from HC.");
				goto next;
			}
		}
	next:
		async_usleep(port_instance->wait_period_usec);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_new_device(uhci_port_t *port)
{
	assert(port);
	assert(usb_hc_connection_is_opened(&port->hc_connection));

	usb_log_info("Adding new device on port %d.\n", port->number);

	/* get address of the future device */
	const usb_address_t usb_address = usb_hc_request_address(&port->hc_connection);

	if (usb_address <= 0) {
		usb_log_error("Recieved invalid address(%d).\n", usb_address);
		return usb_address;
	}
	usb_log_debug("Sucessfully obtained address %d for port %d.\n",
	    usb_address, port->number);

	/* get default address */
	int ret = usb_hc_reserve_default_address(&port->hc_connection);
	if (ret != EOK) {
		usb_log_error("Failed to reserve default address on port %d.\n",
		    port->number);
		int ret2 = usb_hc_unregister_device(&port->hc_connection,
		    usb_address);
		if (ret2 != EOK) {
			usb_log_fatal("Failed to return requested address on port %d.\n",
			   port->number);
			return ret2;
		}
		usb_log_debug("Successfully returned reserved address on port %d.\n",
			port->number);
		return ret;
	}
	usb_log_debug("Sucessfully obtained default address for port %d.\n",
	    port->number);

	/*
	 * the host then waits for at least 100 ms to allow completion of
	 * an insertion process and for power at the device to become stable.
	 */
	async_usleep(100000);

	/* enable port */
	uhci_port_set_enabled(port, true);

	/* The hub maintains the reset signal to that port for 10 ms
	 * (See Section 11.5.1.5)
	 */
	{
		usb_log_debug("Reset Signal start on port %d.\n",
		    port->number);
		port_status_t port_status =
			port_status_read(port->address);
		port_status |= STATUS_IN_RESET;
		port_status_write(port->address, port_status);
		async_usleep(10000);
		port_status =
			port_status_read(port->address);
		port_status &= ~STATUS_IN_RESET;
		port_status_write(port->address, port_status);
		usb_log_debug("Reset Signal stop on port %d.\n",
		    port->number);
	}

	/*
	 * Initialize connection to the device.
	 */
	/* FIXME: check for errors. */
	usb_device_connection_t new_dev_connection;
	usb_endpoint_pipe_t new_dev_ctrl_pipe;
	usb_device_connection_initialize_on_default_address(
	    &new_dev_connection, &port->hc_connection);
	usb_endpoint_pipe_initialize_default_control(&new_dev_ctrl_pipe,
	    &new_dev_connection);

	/*
	 * Assign new address to the device. This function updates
	 * the backing connection to still point to the same device.
	 */
	/* FIXME: check for errors. */
	usb_endpoint_pipe_start_session(&new_dev_ctrl_pipe);
	ret = usb_request_set_address(&new_dev_ctrl_pipe, usb_address);
	usb_endpoint_pipe_end_session(&new_dev_ctrl_pipe);

	if (ret != EOK) { /* address assigning went wrong */
		usb_log_error("Failed(%d) to assign address to the device.\n", ret);
		uhci_port_set_enabled(port, false);
		int release = usb_hc_release_default_address(&port->hc_connection);
		if (release != EOK) {
			usb_log_error("Failed to release default address on port %d.\n",
			    port->number);
			return release;
		}
		usb_log_debug("Sucessfully released default address on port %d.\n",
		    port->number);
		return ret;
	}
	usb_log_debug("Sucessfully assigned address %d for port %d.\n",
	    usb_address, port->number);

	/* release default address */
	ret = usb_hc_release_default_address(&port->hc_connection);
	if (ret != EOK) {
		usb_log_error("Failed to release default address on port %d.\n",
		    port->number);
		return ret;
	}
	usb_log_debug("Sucessfully released default address on port %d.\n",
	    port->number);

	/* communicate and possibly report to devman */
	assert(port->attached_device == 0);

	ret = usb_device_register_child_in_devman(new_dev_connection.address,
	    new_dev_connection.hc_handle, port->rh, &port->attached_device);

	if (ret != EOK) { /* something went wrong */
		usb_log_error("Failed(%d) in usb_drv_register_child.\n", ret);
		uhci_port_set_enabled(port, false);
		return ENOMEM;
	}
	usb_log_info("Sucessfully added device on port(%d) address(%d) handle %d.\n",
		port->number, usb_address, port->attached_device);

	/*
	 * Register the device in the host controller.
	 */
	usb_hc_attached_device_t new_device = {
		.address = new_dev_connection.address,
		.handle = port->attached_device
	};

	ret = usb_hc_register_device(&port->hc_connection, &new_device);
	// TODO: proper error check here
	assert(ret == EOK);

	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_remove_device(uhci_port_t *port)
{
	usb_log_error("Don't know how to remove device %#x.\n",
		(unsigned int)port->attached_device);
//	uhci_port_set_enabled(port, false);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled)
{
	assert(port);

	/* read register value */
	port_status_t port_status
		= port_status_read(port->address);

	/* enable port: register write */
	if (enabled) {
		port_status |= STATUS_ENABLED;
	} else {
		port_status &= ~STATUS_ENABLED;
	}
	port_status_write(port->address, port_status);

	usb_log_info("%s port %d.\n",
	  enabled ? "Enabled" : "Disabled", port->number);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */
