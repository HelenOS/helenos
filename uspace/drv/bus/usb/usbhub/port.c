/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * Hub ports functions.
 */

#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <fibril_synch.h>
#include <usbhc_iface.h>

#include <usb/debug.h>

#include "port.h"
#include "usbhub.h"
#include "status.h"

#define port_log(lvl, port, fmt, ...) do { \
		usb_log_##lvl("(%p-%u): " fmt, \
		    (port->hub), (port->port_number), ##__VA_ARGS__); \
	} while (0)

/** Initialize hub port information.
 *
 * @param port Port to be initialized.
 */
void usb_hub_port_init(usb_hub_port_t *port, usb_hub_dev_t *hub,
    unsigned int port_number)
{
	assert(port);
	memset(port, 0, sizeof(*port));
	port->hub = hub;
	port->port_number = port_number;
	usb_port_init(&port->base);
}

static inline usb_hub_port_t *get_hub_port(usb_port_t *port)
{
	assert(port);
	return (usb_hub_port_t *) port;
}

/**
 * Inform the HC that the device on port is gone.
 */
static void remove_device(usb_port_t *port_base)
{
	usb_hub_port_t *port = get_hub_port(port_base);

	async_exch_t *exch = usb_device_bus_exchange_begin(port->hub->usb_device);
	if (!exch) {
		port_log(error, port, "Cannot remove the device, "
		    "failed creating exchange.");
		return;
	}

	const errno_t err = usbhc_device_remove(exch, port->port_number);
	if (err)
		port_log(error, port, "Failed to remove device: %s",
		    str_error(err));

	usb_device_bus_exchange_end(exch);
}

static usb_speed_t get_port_speed(usb_hub_port_t *port, uint32_t status)
{
	assert(port);
	assert(port->hub);

	return usb_port_speed(port->hub->speed, status);
}

/**
 * Routine for adding a new device in USB2.
 */
static errno_t enumerate_device_usb2(usb_hub_port_t *port, async_exch_t *exch)
{
	errno_t err;

	port_log(debug, port, "Requesting default address.");
	err = usb_hub_reserve_default_address(port->hub, exch, &port->base);
	if (err != EOK) {
		port_log(error, port, "Failed to reserve default address: %s",
		    str_error(err));
		return err;
	}

	/* Reservation of default address could have blocked */
	if (port->base.state != PORT_CONNECTING)
		goto out_address;

	port_log(debug, port, "Resetting port.");
	if ((err = usb_hub_set_port_feature(port->hub, port->port_number,
	    USB_HUB_FEATURE_PORT_RESET))) {
		port_log(warning, port, "Port reset request failed: %s",
		    str_error(err));
		goto out_address;
	}

	if ((err = usb_port_wait_for_enabled(&port->base))) {
		port_log(error, port, "Failed to reset port: %s",
		    str_error(err));
		goto out_address;
	}

	port_log(debug, port, "Enumerating device.");
	if ((err = usbhc_device_enumerate(exch, port->port_number,
	    port->speed))) {
		port_log(error, port, "Failed to enumerate device: %s",
		    str_error(err));
		/* Disable the port */
		usb_hub_clear_port_feature(port->hub, port->port_number,
		    USB2_HUB_FEATURE_PORT_ENABLE);
		goto out_address;
	}

	port_log(debug, port, "Device enumerated");

out_address:
	usb_hub_release_default_address(port->hub, exch);
	return err;
}

/**
 * Routine for adding a new device in USB 3.
 */
static errno_t enumerate_device_usb3(usb_hub_port_t *port, async_exch_t *exch)
{
	errno_t err;

	port_log(debug, port, "Issuing a warm reset.");
	if ((err = usb_hub_set_port_feature(port->hub, port->port_number,
	    USB3_HUB_FEATURE_BH_PORT_RESET))) {
		port_log(warning, port, "Port reset request failed: %s",
		    str_error(err));
		return err;
	}

	if ((err = usb_port_wait_for_enabled(&port->base))) {
		port_log(error, port, "Failed to reset port: %s",
		    str_error(err));
		return err;
	}

	port_log(debug, port, "Enumerating device.");
	if ((err = usbhc_device_enumerate(exch, port->port_number,
	    port->speed))) {
		port_log(error, port, "Failed to enumerate device: %s",
		    str_error(err));
		return err;
	}

	port_log(debug, port, "Device enumerated");
	return EOK;
}

static errno_t enumerate_device(usb_port_t *port_base)
{
	usb_hub_port_t *port = get_hub_port(port_base);

	port_log(debug, port, "Setting up new device.");
	async_exch_t *exch = usb_device_bus_exchange_begin(port->hub->usb_device);
	if (!exch) {
		port_log(error, port, "Failed to create exchange.");
		return ENOMEM;
	}

	const errno_t err = port->hub->speed == USB_SPEED_SUPER ?
	    enumerate_device_usb3(port, exch) :
	    enumerate_device_usb2(port, exch);

	usb_device_bus_exchange_end(exch);
	return err;
}

static void port_changed_connection(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool connected = !!(status & USB_HUB_PORT_STATUS_CONNECTION);
	port_log(debug, port, "Connection change: device %s.", connected ?
	    "attached" : "removed");

	if (connected) {
		usb_port_connected(&port->base, &enumerate_device);
	} else {
		usb_port_disabled(&port->base, &remove_device);
	}
}

static void port_changed_enabled(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLE);
	if (enabled) {
		port_log(warning, port, "Port unexpectedly changed to enabled.");
	} else {
		usb_port_disabled(&port->base, &remove_device);
	}
}

static void port_changed_overcurrent(usb_hub_port_t *port,
    usb_port_status_t status)
{
	const bool overcurrent = !!(status & USB_HUB_PORT_STATUS_OC);

	/*
	 * According to the USB specs:
	 * 11.13.5 Over-current Reporting and Recovery
	 * Hub device is responsible for putting port in power off
	 * mode. USB system software is responsible for powering port
	 * back on when the over-current condition is gone
	 */

	usb_port_disabled(&port->base, &remove_device);

	if (!overcurrent) {
		const errno_t err = usb_hub_set_port_feature(port->hub,
		    port->port_number, USB_HUB_FEATURE_PORT_POWER);
		if (err)
			port_log(error, port, "Failed to set port power "
			    "after OC: %s.", str_error(err));
	}
}

static void port_changed_reset(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLE);

	if (enabled) {
		port->speed = get_port_speed(port, status);
		usb_port_enabled(&port->base);
	} else
		usb_port_disabled(&port->base, &remove_device);
}

typedef void (*change_handler_t)(usb_hub_port_t *, usb_port_status_t);

static void check_port_change(usb_hub_port_t *port, usb_port_status_t *status,
    change_handler_t handler, usb_port_status_t mask,
    usb_hub_class_feature_t feature)
{
	if ((*status & mask) == 0)
		return;

	/* Clear the change so it won't come again */
	usb_hub_clear_port_feature(port->hub, port->port_number, feature);

	if (handler)
		handler(port, *status);

	/* Mark the change as resolved */
	*status &= ~mask;
}

/**
 * Process interrupts on given port
 *
 * Accepts connection, over current and port reset change.
 * @param port port structure
 * @param hub hub representation
 */
void usb_hub_port_process_interrupt(usb_hub_port_t *port)
{
	assert(port);
	port_log(debug2, port, "Interrupt.");

	usb_port_status_t status = 0;
	const errno_t err = usb_hub_get_port_status(port->hub,
	    port->port_number, &status);
	if (err != EOK) {
		port_log(error, port, "Failed to get port status: %s.",
		    str_error(err));
		return;
	}

	check_port_change(port, &status, &port_changed_connection,
	    USB_HUB_PORT_STATUS_C_CONNECTION,
	    USB_HUB_FEATURE_C_PORT_CONNECTION);

	check_port_change(port, &status, &port_changed_overcurrent,
	    USB_HUB_PORT_STATUS_C_OC, USB_HUB_FEATURE_C_PORT_OVER_CURRENT);

	check_port_change(port, &status, &port_changed_reset,
	    USB_HUB_PORT_STATUS_C_RESET, USB_HUB_FEATURE_C_PORT_RESET);

	if (port->hub->speed <= USB_SPEED_HIGH) {
		check_port_change(port, &status, &port_changed_enabled,
		    USB2_HUB_PORT_STATUS_C_ENABLE,
		    USB2_HUB_FEATURE_C_PORT_ENABLE);
	} else {
		check_port_change(port, &status, &port_changed_reset,
		    USB3_HUB_PORT_STATUS_C_BH_RESET,
		    USB3_HUB_FEATURE_C_BH_PORT_RESET);

		check_port_change(port, &status, NULL,
		    USB3_HUB_PORT_STATUS_C_LINK_STATE,
		    USB3_HUB_FEATURE_C_PORT_LINK_STATE);
	}

	/* Check for changes we ignored */
	if (status & 0xffff0000) {
		port_log(debug, port, "Port status change igored. "
		    "Status: %#08" PRIx32, status);
	}
}

/**
 * @}
 */
