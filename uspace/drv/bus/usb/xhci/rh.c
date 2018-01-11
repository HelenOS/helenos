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
static const uint32_t port_change_mask =
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

/**
 * Create and setup a device directly connected to RH. As the xHCI is not using
 * a virtual usbhub device for RH, this routine is called for devices directly.
 */
static int rh_setup_device(xhci_rh_t *rh, uint8_t port_id)
{
	int err;
	assert(rh);

	assert(rh->devices_by_port[port_id - 1] == NULL);

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

	fibril_mutex_lock(&rh->device.base.guard);
	list_append(&dev->link, &rh->device.base.devices);
	rh->devices_by_port[port_id - 1] = xhci_dev;
	fibril_mutex_unlock(&rh->device.base.guard);

	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	return err;
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
		xhci_rh_reset_port(rh, port_id);
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
	fibril_mutex_lock(&rh->device.base.guard);
	list_remove(&dev->base.link);
	rh->devices_by_port[port_id - 1] = NULL;
	fibril_mutex_unlock(&rh->device.base.guard);

	/* Remove device from XHCI bus. */
	bus_device_remove(&dev->base);

	return EOK;
}

/**
 * Handle an incoming Port Change Detected Event.
 */
int xhci_rh_handle_port_status_change_event(xhci_hc_t *hc, xhci_trb_t *trb)
{
	uint8_t port_id = XHCI_QWORD_EXTRACT(trb->parameter, 31, 24);
	usb_log_debug("Port status change event detected for port %u.", port_id);

	/**
	 * We can't be sure that the port change this event announces is the
	 * only port change that happened (see section 4.19.2 of the xHCI
	 * specification). Therefore, we just check all ports for changes.
	 */
	xhci_rh_handle_port_change(&hc->rh);

	return EOK;
}

/**
 * Handle all changes on all ports.
 */
void xhci_rh_handle_port_change(xhci_rh_t *rh)
{
	for (uint8_t i = 1; i <= rh->max_ports; ++i) {
		xhci_port_regs_t *regs = &rh->hc->op_regs->portrs[i - 1];

		uint32_t events = XHCI_REG_RD_FIELD(&regs->portsc, 32);
		XHCI_REG_WR_FIELD(&regs->portsc, events, 32);

		events &= port_change_mask;

		if (events & XHCI_REG_MASK(XHCI_PORT_CSC)) {
			usb_log_info("Connected state changed on port %u.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_CSC);

			bool connected = XHCI_REG_RD(regs, XHCI_PORT_CCS);
			if (connected) {
				handle_connected_device(rh, i);
			} else {
				handle_disconnected_device(rh, i);
			}
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_PEC)) {
			usb_log_info("Port enabled changed on port %u.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_PEC);
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_WRC)) {
			usb_log_info("Warm port reset on port %u completed.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_WRC);
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_OCC)) {
			usb_log_info("Over-current change on port %u.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_OCC);
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_PRC)) {
			usb_log_info("Port reset on port %u completed.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_PRC);

			const xhci_port_speed_t *speed = xhci_rh_get_port_speed(rh, i);
			if (speed->major != 3) {
				/* FIXME: We probably don't want to do this
				 * every time USB2 port is reset. This is a
				 * temporary workaround. */
				rh_setup_device(rh, i);
			}
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_PLC)) {
			usb_log_info("Port link state changed on port %u.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_PLC);
		}

		if (events & XHCI_REG_MASK(XHCI_PORT_CEC)) {
			usb_log_info("Port %u failed to configure link.", i);
			events &= ~XHCI_REG_MASK(XHCI_PORT_CEC);
		}

		if (events) {
			usb_log_warning("Port change (0x%08x) ignored on port %u.", events, i);
		}
	}

	/**
	 * Theory:
	 *
	 * Although more events could have happened while processing, the PCD
	 * bit in USBSTS will be set on every change. Because the PCD is
	 * cleared even before the interrupt is cleared, it is safe to assume
	 * that this handler will be called again.
	 *
	 * But because we could have handled the event in previous run of this
	 * handler, it is not an error when no event is detected.
	 *
	 * Reality:
	 *
	 * The PCD bit is never set. TODO Check why the interrupt never carries
	 * the PCD flag. Possibly repeat the checking until we're sure the
	 * PSCEG is 0 - check section 4.19.2 of the xHCI spec.
	 */
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
 * Issue a port reset for a given port.
 */
int xhci_rh_reset_port(xhci_rh_t* rh, uint8_t port)
{
	usb_log_debug2("Resetting port %u.", port);
	xhci_port_regs_t *regs = &rh->hc->op_regs->portrs[port-1];
	XHCI_REG_SET(regs, XHCI_PORT_PR, 1);

	return EOK;
}

/**
 * @}
 */
