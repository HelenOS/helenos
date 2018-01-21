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

#define port_log(lvl, port, fmt, ...) do { usb_log_##lvl("(%p-%u): " fmt, (port->hub), (port->port_number), ##__VA_ARGS__); } while (0)

/** Initialize hub port information.
 *
 * @param port Port to be initialized.
 */
void usb_hub_port_init(usb_hub_port_t *port, usb_hub_dev_t *hub, unsigned int port_number)
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
		port_log(error, port, "Cannot remove the device, failed creating exchange.");
		return;
	}
	
	const int err = usbhc_device_remove(exch, port->port_number);
	if (err)
		port_log(error, port, "Failed to remove device: %s", str_error(err));

	usb_device_bus_exchange_end(exch);
}


static usb_speed_t get_port_speed(usb_hub_port_t *port, uint32_t status)
{
	assert(port);
	assert(port->hub);

	return usb_port_speed(port->hub->speed, status);
}

/**
 * Routine for adding a new device.
 *
 * Separate fibril is needed because the operation blocks on waiting for
 * requesting default address and resetting port, and we must not block the
 * control pipe.
 */
static int enumerate_device(usb_port_t *port_base)
{
	int err;
	usb_hub_port_t *port = get_hub_port(port_base);

	port_log(debug, port, "Setting up new device.");

	async_exch_t *exch = usb_device_bus_exchange_begin(port->hub->usb_device);
	if (!exch) {
		port_log(error, port, "Failed to create exchange.");
		return ENOMEM;
	}

	/* Reserve default address */
	err = usb_hub_reserve_default_address(port->hub, exch, &port->base);
	if (err != EOK) {
		port_log(error, port, "Failed to reserve default address: %s", str_error(err));
		goto out_exch;
	}

	/* Reservation of default address could have blocked */
	if (port->base.state != PORT_CONNECTING)
		goto out_address;

	port_log(debug, port, "Got default address. Resetting port.");
	int rc = usb_hub_set_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_PORT_RESET);
	if (rc != EOK) {
		port_log(warning, port, "Port reset request failed: %s", str_error(rc));
		goto out_address;
	}

	if ((err = usb_port_wait_for_enabled(&port->base))) {
		port_log(error, port, "Failed to reset port: %s", str_error(err));
		goto out_address;
	}

	port_log(debug, port, "Port reset, enumerating device.");

	if ((err = usbhc_device_enumerate(exch, port->port_number, port->speed))) {
		port_log(error, port, "Failed to enumerate device: %s", str_error(err));
		/* Disable the port */
		usb_hub_clear_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_PORT_ENABLE);
		goto out_address;
	}

	port_log(debug, port, "Device enumerated");

out_address:
	usb_hub_release_default_address(port->hub, exch);
out_exch:
	usb_device_bus_exchange_end(exch);
	return err;
}

static void port_changed_connection(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool connected = !!(status & USB_HUB_PORT_STATUS_CONNECTION);
	port_log(debug, port, "Connection change: device %s.", connected ? "attached" : "removed");

	if (connected) {
		usb_port_connected(&port->base, &enumerate_device);
	} else {
		usb_port_disabled(&port->base, &remove_device);
	}
}

static void port_changed_enabled(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLED);
	if (enabled) {
		port_log(warning, port, "Port unexpectedly changed to enabled.");
	} else {
		usb_port_disabled(&port->base, &remove_device);
	}
}

static void port_changed_suspend(usb_hub_port_t *port, usb_port_status_t status)
{
	port_log(error, port, "Port unexpectedly suspend. Weird, we do not support suspending!");
}

static void port_changed_overcurrent(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool overcurrent = !!(status & USB_HUB_PORT_STATUS_OC);

	/* According to the USB specs:
	 * 11.13.5 Over-current Reporting and Recovery
	 * Hub device is responsible for putting port in power off
	 * mode. USB system software is responsible for powering port
	 * back on when the over-current condition is gone */

	usb_port_disabled(&port->base, &remove_device);

	if (!overcurrent) {
		const int err = usb_hub_set_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_PORT_POWER);
		if (err)
			port_log(error, port, "Failed to set port power after OC: %s.", str_error(err));
	}
}

static void port_changed_reset(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLED);

	if (enabled) {
		// The connecting fibril do not touch speed until the port is enabled,
		// so we do not have to lock
		port->speed = get_port_speed(port, status);
		usb_port_enabled(&port->base);
	} else
		usb_port_disabled(&port->base, &remove_device);
}

typedef void (*change_handler_t)(usb_hub_port_t *, usb_port_status_t);

static const change_handler_t port_change_handlers [] = {
	[USB_HUB_FEATURE_C_PORT_CONNECTION] = &port_changed_connection,
	[USB_HUB_FEATURE_C_PORT_ENABLE] = &port_changed_enabled,
	[USB_HUB_FEATURE_C_PORT_SUSPEND] = &port_changed_suspend,
	[USB_HUB_FEATURE_C_PORT_OVER_CURRENT] = &port_changed_overcurrent,
	[USB_HUB_FEATURE_C_PORT_RESET] = &port_changed_reset,
	[sizeof(usb_port_status_t) * 8] = NULL,
};

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
	const int err = usb_hub_get_port_status(port->hub, port->port_number, &status);
	if (err != EOK) {
		port_log(error, port, "Failed to get port status: %s.", str_error(err));
		return;
	}

	if (port->hub->speed == USB_SPEED_SUPER)
		/* Link state change is not a change we shall clear, nor we care about it */
		status &= ~(1 << USB_HUB_FEATURE_C_PORT_LINK_STATE);

	for (uint32_t feature = 16; feature < sizeof(usb_port_status_t) * 8; ++feature) {
		uint32_t mask = 1 << feature;

		if ((status & mask) == 0)
			continue;

		/* Clear the change so it won't come again */
		usb_hub_clear_port_feature(port->hub, port->port_number, feature);

		if (!port_change_handlers[feature])
			continue;

		/* ACK this change */
		status &= ~mask;

		port_change_handlers[feature](port, status);
	}

	/* Check for changes we ignored */
	if (status & 0xffff0000) {
		port_log(debug, port, "Port status change igored. Status: %#08" PRIx32, status);
	}
}


/**
 * @}
 */
