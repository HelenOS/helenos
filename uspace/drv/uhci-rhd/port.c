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

static int uhci_port_new_device(uhci_port_t *port, usb_speed_t speed);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static int uhci_port_check(void *port);
static int uhci_port_reset_enable(int portno, void *arg);
/*----------------------------------------------------------------------------*/
/** Initializes UHCI root hub port instance.
 *
 * @param[in] port Memory structure to use.
 * @param[in] addr Address of I/O register.
 * @param[in] number Port number.
 * @param[in] usec Polling interval.
 * @param[in] rh Pointer to ddf instance fo the root hub driver.
 * @return Error code.
 *
 * Starts the polling fibril.
 */
int uhci_port_init(uhci_port_t *port,
    port_status_t *address, unsigned number, unsigned usec, ddf_dev_t *rh)
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
		usb_log_error("Port(%p - %d): failed to launch root hub fibril.",
		    port->address, port->number);
		return ENOMEM;
	}

	fibril_add_ready(port->checker);
	usb_log_debug("Port(%p - %d): Added fibril. %x\n",
	    port->address, port->number, port->checker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Finishes UHCI root hub port instance.
 *
 * @param[in] port Memory structure to use.
 *
 * Stops the polling fibril.
 */
void uhci_port_fini(uhci_port_t *port)
{
	/* TODO: Kill fibril here */
	return;
}
/*----------------------------------------------------------------------------*/
/** Periodically checks port status and reports new devices.
 *
 * @param[in] port Memory structure to use.
 * @return Error code.
 */
int uhci_port_check(void *port)
{
	uhci_port_t *instance = port;
	assert(instance);

	/* Iteration count, for debug purposes only */
	unsigned count = 0;

	while (1) {
		async_usleep(instance->wait_period_usec);

		/* read register value */
		port_status_t port_status = port_status_read(instance->address);

		/* debug print mutex */
		static fibril_mutex_t dbg_mtx =
		    FIBRIL_MUTEX_INITIALIZER(dbg_mtx);
		fibril_mutex_lock(&dbg_mtx);
		usb_log_debug2("Port(%p - %d): Status: %#04x. === %u\n",
		  instance->address, instance->number, port_status, count++);
//		print_port_status(port_status);
		fibril_mutex_unlock(&dbg_mtx);

		if ((port_status & STATUS_CONNECTED_CHANGED) == 0)
			continue;

		usb_log_debug("Port(%p - %d): Connected change detected: %x.\n",
		    instance->address, instance->number, port_status);

		int rc =
		    usb_hc_connection_open(&instance->hc_connection);
		if (rc != EOK) {
			usb_log_error("Port(%p - %d): Failed to connect to HC.",
			    instance->address, instance->number);
			continue;
		}

		/* Remove any old device */
		if (instance->attached_device) {
			usb_log_debug2("Port(%p - %d): Removing device.\n",
			    instance->address, instance->number);
			uhci_port_remove_device(instance);
		}

		if ((port_status & STATUS_CONNECTED) != 0) {
			/* New device */
			const usb_speed_t speed =
			    ((port_status & STATUS_LOW_SPEED) != 0) ?
			    USB_SPEED_LOW : USB_SPEED_FULL;
			uhci_port_new_device(instance, speed);
		} else {
			/* Write one to WC bits, to ack changes */
			port_status_write(instance->address, port_status);
			usb_log_debug("Port(%p - %d): Change status ACK.\n",
			    instance->address, instance->number);
		}

		rc = usb_hc_connection_close(&instance->hc_connection);
		if (rc != EOK) {
			usb_log_error("Port(%p - %d): Failed to disconnect.",
			    instance->address, instance->number);
		}
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Callback for enabling port during adding a new device.
 *
 * @param portno Port number (unused).
 * @param arg Pointer to uhci_port_t of port with the new device.
 * @return Error code.
 */
int uhci_port_reset_enable(int portno, void *arg)
{
	uhci_port_t *port = (uhci_port_t *) arg;

	usb_log_debug2("Port(%p - %d): new_device_enable_port.\n",
	    port->address, port->number);

	/*
	 * The host then waits for at least 100 ms to allow completion of
	 * an insertion process and for power at the device to become stable.
	 */
	async_usleep(100000);


	/* The hub maintains the reset signal to that port for 10 ms
	 * (See Section 11.5.1.5)
	 */
	{
		usb_log_debug("Port(%p - %d): Reset Signal start.\n",
		    port->address, port->number);
		port_status_t port_status =
			port_status_read(port->address);
		port_status |= STATUS_IN_RESET;
		port_status_write(port->address, port_status);
		async_usleep(10000);
		port_status = port_status_read(port->address);
		port_status &= ~STATUS_IN_RESET;
		port_status_write(port->address, port_status);
		usb_log_debug("Port(%p - %d): Reset Signal stop.\n",
		    port->address, port->number);
	}

	/* Enable the port. */
	uhci_port_set_enabled(port, true);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Initializes and reports connected device.
 *
 * @param[in] port Memory structure to use.
 * @param[in] speed Detected speed.
 * @return Error code.
 *
 * Uses libUSB function to do the actual work.
 */
int uhci_port_new_device(uhci_port_t *port, usb_speed_t speed)
{
	assert(port);
	assert(usb_hc_connection_is_opened(&port->hc_connection));

	usb_log_info("Port(%p-%d): Detected new device.\n",
	    port->address, port->number);

	usb_address_t dev_addr;
	int rc = usb_hc_new_device_wrapper(port->rh, &port->hc_connection,
	    speed, uhci_port_reset_enable, port->number, port,
	    &dev_addr, &port->attached_device, NULL, NULL, NULL);

	if (rc != EOK) {
		usb_log_error("Port(%p-%d): Failed(%d) to add device: %s.\n",
		    port->address, port->number, rc, str_error(rc));
		uhci_port_set_enabled(port, false);
		return rc;
	}

	usb_log_info("Port(%p-%d): New device has address %d (handle %zu).\n",
	    port->address, port->number, dev_addr, port->attached_device);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Removes device.
 *
 * @param[in] port Memory structure to use.
 * @return Error code.
 *
 * Does not work DDF does not support device removal.
 */
int uhci_port_remove_device(uhci_port_t *port)
{
	usb_log_error("Port(%p-%d): Don't know how to remove device %#x.\n",
	    port->address, port->number, (unsigned int)port->attached_device);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Enables and disables port.
 *
 * @param[in] port Memory structure to use.
 * @return Error code. (Always EOK)
 */
int uhci_port_set_enabled(uhci_port_t *port, bool enabled)
{
	assert(port);

	/* Read register value */
	port_status_t port_status = port_status_read(port->address);

	/* Set enabled bit */
	if (enabled) {
		port_status |= STATUS_ENABLED;
	} else {
		port_status &= ~STATUS_ENABLED;
	}

	/* Write new value. */
	port_status_write(port->address, port_status);

	usb_log_info("Port(%p-%d): %sabled port.\n",
		port->address, port->number, enabled ? "En" : "Dis");
	return EOK;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */
