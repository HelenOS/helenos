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

#include <usb/usb.h>    /* usb_address_t */
#include <usb/usbdrv.h> /* usb_drv_*     */
#include <usb/debug.h>

#include "port.h"
#include "port_status.h"

static int uhci_port_new_device(uhci_port_t *port);
static int uhci_port_remove_device(uhci_port_t *port);
static int uhci_port_set_enabled(uhci_port_t *port, bool enabled);
static int uhci_port_check(void *port);

int uhci_port_init(
  uhci_port_t *port, port_status_t *address, unsigned number,
  unsigned usec, device_t *rh)
{
	assert(port);
	port->address = address;
	port->number = number;
	port->wait_period_usec = usec;
	port->attached_device = 0;
	port->rh = rh;
	port->hc_phone = rh->parent_phone;

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
//	fibril_teardown(port->checker);
	return;
}
/*----------------------------------------------------------------------------*/
int uhci_port_check(void *port)
{
	async_usleep( 1000000 );
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
			if (port_status & STATUS_CONNECTED) {
				/* new device */
				uhci_port_new_device(port_instance);
			} else {
				uhci_port_remove_device(port_instance);
			}
		}
		async_usleep(port_instance->wait_period_usec);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_new_device(uhci_port_t *port)
{
	assert(port);
	assert(port->hc_phone);

	usb_log_info("Adding new device on port %d.\n", port->number);


	/* get default address */
	int ret = usb_drv_reserve_default_address(port->hc_phone);
	if (ret != EOK) {
		usb_log_error("Failed to reserve default address.\n");
		return ret;
	}

	const usb_address_t usb_address = usb_drv_request_address(port->hc_phone);

	if (usb_address <= 0) {
		usb_log_error("Recieved invalid address(%d).\n", usb_address);
		return usb_address;
	}
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
	port_status_t port_status =
		port_status_read(port->address);
	port_status |= STATUS_IN_RESET;
	port_status_write(port->address, port_status);
	async_usleep(10000);
	port_status =
		port_status_read(port->address);
	port_status &= ~STATUS_IN_RESET;
	port_status_write(port->address, port_status);

	/* assign address to device */
	ret = usb_drv_req_set_address(port->hc_phone, 0, usb_address);


	if (ret != EOK) { /* address assigning went wrong */
		usb_log_error("Failed(%d) to assign address to the device.\n", ret);
		uhci_port_set_enabled(port, false);
		int release = usb_drv_release_default_address(port->hc_phone);
		if (release != EOK) {
			usb_log_error("Failed to release default address.\n");
			return release;
		}
		return ret;
	}

	/* release default address */
	ret = usb_drv_release_default_address(port->hc_phone);
	if (ret != EOK) {
		usb_log_error("Failed to release default address.\n");
		return ret;
	}

	/* communicate and possibly report to devman */
	assert(port->attached_device == 0);

	ret = usb_drv_register_child_in_devman(port->hc_phone, port->rh,
	  usb_address, &port->attached_device);

	if (ret != EOK) { /* something went wrong */
		usb_log_error("Failed(%d) in usb_drv_register_child.\n", ret);
		uhci_port_set_enabled(port, false);
		return ENOMEM;
	}
	usb_log_info("Sucessfully added device on port(%d) address(%d).\n",
		port->number, usb_address);

	/* TODO: bind the address here */

	return EOK;
}
/*----------------------------------------------------------------------------*/
static int uhci_port_remove_device(uhci_port_t *port)
{
	usb_log_error("Don't know how to remove device %#x.\n",
		(unsigned int)port->attached_device);
	uhci_port_set_enabled(port, false);
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
