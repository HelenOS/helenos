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
#include <fibril_synch.h>

#include <usb/usb.h>    /* usb_address_t */
#include <usb/usbdevice.h>
#include <usb/hub.h>
#include <usb/request.h>
#include <usb/debug.h>
#include <usb/recognise.h>

#include "port.h"
#include "port_status.h"

static int uhci_port_new_device(uhci_port_t *port, uint16_t status);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static int uhci_port_check(void *port);
static int new_device_enable_port(int portno, void *arg);

int uhci_port_init(
  uhci_port_t *port, port_status_t *address, unsigned number,
  unsigned usec, ddf_dev_t *rh)
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
		/* read register value */
		port_status_t port_status =
			port_status_read(port_instance->address);

		/* debug print */
		static fibril_mutex_t dbg_mtx = FIBRIL_MUTEX_INITIALIZER(dbg_mtx);
		fibril_mutex_lock(&dbg_mtx);
		usb_log_debug("Port %d status at %p: 0x%04x.\n",
		  port_instance->number, port_instance->address, port_status);
		print_port_status(port_status);
		fibril_mutex_unlock(&dbg_mtx);

		if (port_status & STATUS_CONNECTED_CHANGED) {
			usb_log_debug("Change detected on port %d.\n", port_instance->number);
			int rc = usb_hc_connection_open(
			    &port_instance->hc_connection);
			if (rc != EOK) {
				usb_log_error("Failed to connect to HC.");
				goto next;
			}

			/* remove any old device */
			if (port_instance->attached_device) {
				usb_log_debug("Removing device on port %d.\n",
				    port_instance->number);
				uhci_port_remove_device(port_instance);
			}

			port_status_write(port_instance->address, STATUS_CONNECTED_CHANGED);
			usb_log_debug("Change status erased on port %d.\n",
			    port_instance->number);

			if (port_status & STATUS_CONNECTED) {
				/* new device */
				uhci_port_new_device(port_instance, port_status);
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

/** Callback for enabling port during adding a new device.
 *
 * @param portno Port number (unused).
 * @param arg Pointer to uhci_port_t of port with the new device.
 * @return Error code.
 */
static int new_device_enable_port(int portno, void *arg)
{
	uhci_port_t *port = (uhci_port_t *) arg;

	usb_log_debug("new_device_enable_port(%d)\n", port->number);

	/*
	 * The host then waits for at least 100 ms to allow completion of
	 * an insertion process and for power at the device to become stable.
	 */
	async_usleep(100000);


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

	/* Enable the port. */
	uhci_port_set_enabled(port, true);

	return EOK;
}

/*----------------------------------------------------------------------------*/
static int uhci_port_new_device(uhci_port_t *port, uint16_t status)
{
	assert(port);
	assert(usb_hc_connection_is_opened(&port->hc_connection));

	usb_log_info("Detected new device on port %u.\n", port->number);

	usb_address_t dev_addr;
	int rc = usb_hc_new_device_wrapper(port->rh, &port->hc_connection,
	    ((status & STATUS_LOW_SPEED) != 0) ? USB_SPEED_LOW : USB_SPEED_FULL,
	    new_device_enable_port, port->number, port,
	    &dev_addr, &port->attached_device, NULL, NULL, NULL);
	if (rc != EOK) {
		usb_log_error("Failed adding new device on port %u: %s.\n",
		    port->number, str_error(rc));
		uhci_port_set_enabled(port, false);
		return rc;
	}

	usb_log_info("New device on port %u has address %d (handle %zu).\n",
	    port->number, dev_addr, port->attached_device);

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
