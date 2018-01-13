/*
 * Copyright (c) 2017 Michal Staruch
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The roothub structures abstraction.
 */

#include <errno.h>
#include <str_error.h>
#include <usb/request.h>
#include <usb/debug.h>
#include <usb/host/bus.h>
#include <usb/host/ddf_helpers.h>
#include <usb/host/dma_buffer.h>
#include <usb/host/hcd.h>

#include "debug.h"
#include "commands.h"
#include "endpoint.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "rh.h"
#include "transfers.h"

/* This mask only lists registers, which imply port change. */
static const uint32_t port_events_mask =
	XHCI_REG_MASK(XHCI_PORT_CSC) |
	XHCI_REG_MASK(XHCI_PORT_PEC) |
	XHCI_REG_MASK(XHCI_PORT_WRC) |
	XHCI_REG_MASK(XHCI_PORT_OCC) |
	XHCI_REG_MASK(XHCI_PORT_PRC) |
	XHCI_REG_MASK(XHCI_PORT_PLC) |
	XHCI_REG_MASK(XHCI_PORT_CEC);

/**
 * Initialize the roothub subsystem.
 */
int xhci_rh_init(xhci_rh_t *rh, xhci_hc_t *hc)
{
	assert(rh);
	assert(hc);

	rh->hc = hc;
	rh->max_ports = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PORTS);
	rh->devices_by_port = (xhci_device_t **) calloc(rh->max_ports, sizeof(xhci_device_t *));

	const int err = bus_device_init(&rh->device.base, &rh->hc->bus.base);
	if (err)
		return err;

	/* Initialize route string */
	rh->device.route_str = 0;
	rh->device.tier = 0;

	fibril_mutex_initialize(&rh->event_guard);
	fibril_condvar_initialize(&rh->event_ready);
	fibril_condvar_initialize(&rh->event_handled);

	return EOK;
}

/**
 * Finalize the RH subsystem.
 */
int xhci_rh_fini(xhci_rh_t *rh)
{
	assert(rh);
	free(rh->devices_by_port);
	return EOK;
}

typedef struct rh_event {
	uint8_t port_id;
	uint32_t events;
	unsigned readers_to_go;
} rh_event_t;

static int rh_event_wait_timeout(xhci_rh_t *rh, uint8_t port_id, uint32_t mask, suseconds_t timeout)
{
	int r;
	assert(fibril_mutex_is_locked(&rh->event_guard));

	++rh->event_readers_waiting;

	do {
		r = fibril_condvar_wait_timeout(&rh->event_ready, &rh->event_guard, timeout);
		if (r != EOK)
			break;

		assert(rh->event);
		if (--rh->event->readers_to_go == 0)
			fibril_condvar_broadcast(&rh->event_handled);
	} while (rh->event->port_id != port_id || (rh->event->events & mask) != mask);

	if (r == EOK)
		rh->event->events &= ~mask;

	--rh->event_readers_waiting;

	return r;
}

static void rh_event_run_handlers(xhci_rh_t *rh, uint8_t port_id, uint32_t *events)
{
	assert(fibril_mutex_is_locked(&rh->event_guard));

	/* There can be different event running already */
	while (rh->event)
		fibril_condvar_wait(&rh->event_handled, &rh->event_guard);

	rh_event_t event = {
		.port_id = port_id,
		.events = *events,
		.readers_to_go = rh->event_readers_waiting,
	};

	rh->event = &event;
	fibril_condvar_broadcast(&rh->event_ready);
	while (event.readers_to_go)
		fibril_condvar_wait(&rh->event_handled, &rh->event_guard);
	*events = event.events;
	rh->event = NULL;

	/* Wake other threads potentially waiting to post their event */
	fibril_condvar_broadcast(&rh->event_handled);
}

/**
 * Create and setup a device directly connected to RH. As the xHCI is not using
 * a virtual usbhub device for RH, this routine is called for devices directly.
 */
static int rh_setup_device(xhci_rh_t *rh, uint8_t port_id)
{
	int err;
	assert(rh);
	assert(rh->devices_by_port[port_id - 1] == NULL);

	if (!XHCI_REG_RD(&rh->hc->op_regs->portrs[port_id - 1], XHCI_PORT_PED)) {
		usb_log_error("Cannot setup RH device: port is disabled.");
		return EIO;
	}

	device_t *dev = hcd_ddf_fun_create(&rh->hc->base);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	const xhci_port_speed_t *port_speed = xhci_rh_get_port_speed(rh, port_id);
	xhci_device_t *xhci_dev = xhci_device_get(dev);
	xhci_dev->usb3 = port_speed->major == 3;
	xhci_dev->rh_port = port_id;

	dev->hub = &rh->device.base;
	dev->port = port_id;
	dev->speed = port_speed->usb_speed;

	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to enumerate USB device: %s", str_error(err));
		return err;
	}

	if (!ddf_fun_get_name(dev->fun)) {
		bus_device_set_default_name(dev);
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Failed to register device " XHCI_DEV_FMT " DDF function: %s.",
		    XHCI_DEV_ARGS(*xhci_dev), str_error(err));
		goto err_usb_dev;
	}

	rh->devices_by_port[port_id - 1] = xhci_dev;

	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	return err;
}


static int rh_port_reset_sync(xhci_rh_t *rh, uint8_t port_id)
{
	xhci_port_regs_t *regs = &rh->hc->op_regs->portrs[port_id - 1];

	fibril_mutex_lock(&rh->event_guard);
	XHCI_REG_SET(regs, XHCI_PORT_PR, 1);
	const int r = rh_event_wait_timeout(rh, port_id, XHCI_REG_MASK(XHCI_PORT_PRC), 0);
	fibril_mutex_unlock(&rh->event_guard);
	return r;
}

/**
 * Handle a device connection. USB 3+ devices are set up directly, USB 2 and
 * below first need to have their port reset.
 */
static int handle_connected_device(xhci_rh_t *rh, uint8_t port_id)
{
	xhci_port_regs_t *regs = &rh->hc->op_regs->portrs[port_id - 1];

	uint8_t link_state = XHCI_REG_RD(regs, XHCI_PORT_PLS);
	const xhci_port_speed_t *speed = xhci_rh_get_port_speed(rh, port_id);

	usb_log_info("Detected new %.4s%u.%u device on port %u.", speed->name, speed->major, speed->minor, port_id);

	if (speed->major == 3) {
		if (link_state == 0) {
			/* USB3 is automatically advanced to enabled. */
			return rh_setup_device(rh, port_id);
		}
		else if (link_state == 5) {
			/* USB 3 failed to enable. */
			usb_log_error("USB 3 port couldn't be enabled.");
			return EAGAIN;
		}
		else {
			usb_log_error("USB 3 port is in invalid state %u.", link_state);
			return EINVAL;
		}
	}
	else {
		usb_log_debug("USB 2 device attached, issuing reset.");
		const int err = rh_port_reset_sync(rh, port_id);
		if (err)
			return err;

		rh_setup_device(rh, port_id);
		return EOK;
	}
}

/**
 * Deal with a detached device.
 */
static int handle_disconnected_device(xhci_rh_t *rh, uint8_t port_id)
{
	assert(rh);

	/* Find XHCI device by the port. */
	xhci_device_t *dev = rh->devices_by_port[port_id - 1];
	if (!dev) {
		/* Must be extraneous call. */
		return EOK;
	}

	usb_log_info("Device " XHCI_DEV_FMT " at port %u has been disconnected.",
	    XHCI_DEV_ARGS(*dev), port_id);

	/* Mark the device as detached. */
	rh->devices_by_port[port_id - 1] = NULL;

	/* Remove device from XHCI bus. */
	bus_device_gone(&dev->base);

	return EOK;
}

typedef int (*rh_event_handler_t)(xhci_rh_t *, uint8_t);

typedef struct rh_event_args {
	xhci_rh_t *rh;
	uint8_t port_id;
	rh_event_handler_t handler;
} rh_event_args_t;

static int rh_event_handler_fibril(void *arg) {
	rh_event_args_t *rh_args = arg;
	xhci_rh_t *rh = rh_args->rh;
	uint8_t port_id = rh_args->port_id;
	rh_event_handler_t handler = rh_args->handler;

	free(rh_args);

	return handler(rh, port_id);
}

static fid_t handle_in_fibril(xhci_rh_t *rh, uint8_t port_id, rh_event_handler_t handler)
{
	rh_event_args_t *args = malloc(sizeof(*args));
	*args = (rh_event_args_t) {
		.rh = rh,
		.port_id = port_id,
		.handler = handler,
	};

	const fid_t fid = fibril_create(rh_event_handler_fibril, args);
	fibril_add_ready(fid);
	return fid;
}

/**
 * Handle all changes on specified port.
 */
void xhci_rh_handle_port_change(xhci_rh_t *rh, uint8_t port_id)
{
	fibril_mutex_lock(&rh->event_guard);
	xhci_port_regs_t * const regs = &rh->hc->op_regs->portrs[port_id - 1];

	uint32_t events = XHCI_REG_RD_FIELD(&regs->portsc, 32) & port_events_mask;

	while (events) {
		/*
		 * The PED bit in xHCI has RW1C semantics, which means that
		 * writing 1 to it will disable the port. Which means all
		 * standard mechanisms of register handling fails here.
		 */
		uint32_t portsc = XHCI_REG_RD_FIELD(&regs->portsc, 32);
		portsc &= ~(port_events_mask | XHCI_REG_MASK(XHCI_PORT_PED)); // Clear events + PED
		portsc |= events; // Add back events to assert them
		XHCI_REG_WR_FIELD(&regs->portsc, portsc, 32);

		if (events & XHCI_REG_MASK(XHCI_PORT_CSC)) {
			usb_log_info("Connected state changed on port %u.", port_id);
			events &= ~XHCI_REG_MASK(XHCI_PORT_CSC);

			bool connected = XHCI_REG_RD(regs, XHCI_PORT_CCS);
			if (connected) {
				handle_in_fibril(rh, port_id, handle_connected_device);
			} else {
				handle_in_fibril(rh, port_id, handle_disconnected_device);
			}
		}

		if (events != 0)
			rh_event_run_handlers(rh, port_id, &events);

		if (events != 0)
			usb_log_debug("RH port %u change not handled: 0x%x", port_id, events);
		
		/* Make sure that PSCEG is 0 before exiting the loop. */
		events = XHCI_REG_RD_FIELD(&regs->portsc, 32) & port_events_mask;
	}

	fibril_mutex_unlock(&rh->event_guard);
}

/**
 * Get a port speed for a given port id.
 */
const xhci_port_speed_t *xhci_rh_get_port_speed(xhci_rh_t *rh, uint8_t port)
{
	xhci_port_regs_t *port_regs = &rh->hc->op_regs->portrs[port - 1];

	unsigned psiv = XHCI_REG_RD(port_regs, XHCI_PORT_PS);
	return &rh->hc->speeds[psiv];
}

/**
 * @}
 */
