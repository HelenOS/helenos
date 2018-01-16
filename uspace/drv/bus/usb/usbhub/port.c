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
	fibril_mutex_initialize(&port->guard);
	fibril_condvar_initialize(&port->state_cv);
	port->hub = hub;
	port->port_number = port_number;
}

/**
 * Utility method to change current port state and notify waiters.
 */
static void port_change_state(usb_hub_port_t *port, port_state_t state)
{
	assert(fibril_mutex_is_locked(&port->guard));
#define S(STATE) [PORT_##STATE] = #STATE
	static const char *state_names [] = {
	    S(DISABLED), S(CONNECTED), S(ERROR), S(IN_RESET), S(ENABLED),
	};
#undef S
	port_log(debug, port, "%s ->%s", state_names[port->state], state_names[state]);
	port->state = state;
	fibril_condvar_broadcast(&port->state_cv);
}

/**
 * Utility method to wait for a particular state.
 *
 * @warning Some states might not be reached because of an external error
 * condition (PORT_ENABLED).
 */
static void port_wait_state(usb_hub_port_t *port, port_state_t state)
{
	assert(fibril_mutex_is_locked(&port->guard));
	while (port->state != state)
		fibril_condvar_wait(&port->state_cv, &port->guard);
}

/**
 * Inform the HC that the device on port is gone.
 */
static int usb_hub_port_device_gone(usb_hub_port_t *port)
{
	assert(port);
	assert(fibril_mutex_is_locked(&port->guard));
	assert(port->state == PORT_ENABLED);

	async_exch_t *exch = usb_device_bus_exchange_begin(port->hub->usb_device);
	if (!exch)
		return ENOMEM;
	const int rc = usbhc_device_remove(exch, port->port_number);
	usb_device_bus_exchange_end(exch);
	return rc;
}

/**
 * Teardown a device on port, no matter which state it was in.
 */
static void port_make_disabled(usb_hub_port_t *port)
{
	assert(fibril_mutex_is_locked(&port->guard));

	port_log(debug, port, "Making device offline.");

	switch (port->state) {
	case PORT_ENABLED:
		port_log(debug, port, "Enabled ->");
		if (usb_hub_port_device_gone(port))
			port_log(error, port, "Failed to remove the device node from HC. Continuing anyway.");
		port_change_state(port, PORT_DISABLED);
		break;

	case PORT_CONNECTED:
		port_log(debug, port, "Connected ->");
		/* fallthrough */
	case PORT_IN_RESET:
		port_log(debug, port, "In reset ->");
		port_change_state(port, PORT_ERROR);
		/* fallthrough */
	case PORT_ERROR:
		port_log(debug, port, "Error ->");
		port_wait_state(port, PORT_DISABLED);
		/* fallthrough */
	case PORT_DISABLED:
		port_log(debug, port, "Disabled.");
		break;
	}

	assert(port->state == PORT_DISABLED);
}

/**
 * Finalize a port. Make sure no fibril is managing its state,
 * and that the HC is aware the device is no longer there.
 */
void usb_hub_port_fini(usb_hub_port_t *port)
{
	assert(port);

	fibril_mutex_lock(&port->guard);
	switch (port->state) {
	case PORT_ENABLED:
		/*
		 * We shall inform the HC that the device is gone.
		 * However, we can't wait for it, because if the device is hub,
		 * it would have to use the same IPC handling fibril as we do.
		 * But we cannot even defer it to another fibril, because then
		 * the HC would assume our driver didn't cleanup properly, and
		 * will remove those devices by himself.
		 *
		 * So the solutions seems to behave like a bad driver and leave
		 * the work for HC.
		 */
		port_change_state(port, PORT_DISABLED);
		break;

	case PORT_CONNECTED:
	case PORT_IN_RESET:
		port_change_state(port, PORT_ERROR);
		/* fallthrough */
	case PORT_ERROR:
		port_wait_state(port, PORT_DISABLED);
		/* fallthrough */
	case PORT_DISABLED:
		break;
	}
	port_log(debug, port, "Finalized.");
	fibril_mutex_unlock(&port->guard);
}

static int port_reset_sync(usb_hub_port_t *port)
{
	assert(fibril_mutex_is_locked(&port->guard));
	assert(port->state == PORT_CONNECTED);

	port_change_state(port, PORT_IN_RESET);
	port_log(debug2, port, "Issuing reset.");
	int rc = usb_hub_set_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_PORT_RESET);
	if (rc != EOK) {
		port_log(warning, port, "Port reset request failed: %s", str_error(rc));
		return rc;
	}

	fibril_condvar_wait_timeout(&port->state_cv, &port->guard, 2000000);
	return port->state == PORT_ENABLED ? EOK : ESTALL;
}

/**
 * Routine for adding a new device.
 *
 * Separate fibril is needed because the operation blocks on waiting for
 * requesting default address and resetting port, and we must not block the
 * control pipe.
 */
static void setup_device(usb_hub_port_t *port)
{
	int err;

	fibril_mutex_lock(&port->guard);

	if (port->state == PORT_ERROR) {
		/* 
		 * The device was removed faster than this fibril acquired the
		 * mutex.
		 */
		port_change_state(port, PORT_DISABLED);
		goto out;
	}

	if (port->state != PORT_CONNECTED) {
		/*
		 * Another fibril already took care of the device.
		 * This may happen for example when the connection is unstable
		 * and a sequence of connect, disconnect and connect come
		 * faster the first fibril manages to request the default
		 * address.
		 */
		goto out;
	}

	port_log(debug, port, "Setting up new device.");

	async_exch_t *exch = usb_device_bus_exchange_begin(port->hub->usb_device);
	if (!exch) {
		port_log(error, port, "Failed to create exchange.");
		goto out;
	}

	/* Reserve default address
	 * TODO: Make the request synchronous.
	 */
	while ((err = usbhc_reserve_default_address(exch)) == EAGAIN) {
		fibril_condvar_wait_timeout(&port->state_cv, &port->guard, 500000);
		if (port->state != PORT_CONNECTED) {
			assert(port->state == PORT_ERROR);
			port_change_state(port, PORT_DISABLED);
			goto out_exch;
		}
	}
	if (err != EOK) {
		port_log(error, port, "Failed to reserve default address: %s", str_error(err));
		goto out_exch;
	}

	port_log(debug, port, "Got default address. Resetting port.");

	if ((err = port_reset_sync(port))) {
		port_log(error, port, "Failed to reset port.");
		port_change_state(port, PORT_DISABLED);
		goto out_address;
	}

	assert(port->state == PORT_ENABLED);

	port_log(debug, port, "Port reset, enumerating device.");

	if ((err = usbhc_device_enumerate(exch, port->port_number, port->speed))) {
		port_log(error, port, "Failed to enumerate device: %s", str_error(err));
		port_change_state(port, PORT_DISABLED);
		goto out_port;
	}

	port_log(debug, port, "Device enumerated");

out_port:
	if (port->state != PORT_ENABLED)
		usb_hub_clear_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_C_PORT_ENABLE);

out_address:
	if ((err = usbhc_release_default_address(exch)))
		port_log(error, port, "Failed to release default address: %s", str_error(err));

out_exch:
	usb_device_bus_exchange_end(exch);

out:
	assert(port->state == PORT_ENABLED || port->state == PORT_DISABLED);
	fibril_mutex_unlock(&port->guard);
}

static int setup_device_worker(void *arg)
{
	setup_device(arg);
	return EOK;
}

/** Start device adding when connection change is detected.
 *
 * This fires a new fibril to complete the device addition.
 *
 * @param hub Hub where the change occured.
 * @param port Port index (starting at 1).
 * @param speed Speed of the device.
 * @return Error code.
 */
static int create_setup_device_fibril(usb_hub_port_t *port)
{
	assert(port);

	fid_t fibril = fibril_create(setup_device_worker, port);
	if (!fibril)
		return ENOMEM;

	fibril_add_ready(fibril);
	return EOK;
}

static void port_changed_connection(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool connected = !!(status & USB_HUB_PORT_STATUS_CONNECTION);
	port_log(debug, port, "Connection change: device %s.", connected ? "attached" : "removed");

	if (connected) {
		if (port->state == PORT_ENABLED)
			port_log(warning, port, "Connection detected on port that is currently enabled. Resetting.");

		port_make_disabled(port);
		port_change_state(port, PORT_CONNECTED);
		port->speed = usb_port_speed(status);
		create_setup_device_fibril(port);
	} else {
		port_make_disabled(port);
	}
}

static void port_changed_enabled(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLED);
	if (enabled) {
		port_log(warning, port, "Port unexpectedly changed to enabled.");
	} else {
		port_make_disabled(port);
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

	port_make_disabled(port);

	if (!overcurrent) {
		const int err = usb_hub_set_port_feature(port->hub, port->port_number, USB_HUB_FEATURE_PORT_POWER);
		if (err)
			port_log(error, port, "Failed to set port power after OC: %s.", str_error(err));
	}
}

static void port_changed_reset(usb_hub_port_t *port, usb_port_status_t status)
{
	const bool enabled = !!(status & USB_HUB_PORT_STATUS_ENABLED);

	/* Check if someone is waiting for the result */
	if (port->state != PORT_IN_RESET)
		return;

	port_change_state(port, enabled ? PORT_ENABLED : PORT_ERROR);
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
void usb_hub_port_process_interrupt(usb_hub_port_t *port, usb_hub_dev_t *hub)
{
	assert(port);
	assert(hub);
	port_log(debug2, port, "Interrupt.");

	usb_port_status_t status = 0;
	const int err = usb_hub_get_port_status(port->hub, port->port_number, &status);
	if (err != EOK) {
		port_log(error, port, "Failed to get port status: %s.", str_error(err));
		return;
	}

	fibril_mutex_lock(&port->guard);

	for (uint32_t feature = 0; feature < sizeof(usb_port_status_t) * 8; ++feature) {
		uint32_t mask = 1 << feature;

		if ((status & mask) == 0)
			continue;

		if (!port_change_handlers[feature])
			continue;

		/* ACK this change */
		status &= ~mask;
		usb_hub_clear_port_feature(port->hub, port->port_number, feature);

		port_change_handlers[feature](port, status);
	}

	fibril_mutex_unlock(&port->guard);

	port_log(debug2, port, "Port status after handling: %#08" PRIx32, status);
}


/**
 * @}
 */
