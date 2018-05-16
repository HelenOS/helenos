/*
 * Copyright (c) 2018 Petr Manek, Ondrej Hlavaty, Michal Staruch, Jaroslav Jindrak
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
#include <usb/dma_buffer.h>
#include <usb/host/hcd.h>
#include <usb/port.h>

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

static const uint32_t port_reset_mask =
    XHCI_REG_MASK(XHCI_PORT_WRC) |
    XHCI_REG_MASK(XHCI_PORT_PRC);

typedef struct rh_port {
	usb_port_t base;
	xhci_rh_t *rh;
	uint8_t major;
	xhci_port_regs_t *regs;
	xhci_device_t *device;
} rh_port_t;

static int rh_worker(void *);

/**
 * Initialize the roothub subsystem.
 */
errno_t xhci_rh_init(xhci_rh_t *rh, xhci_hc_t *hc)
{
	assert(rh);
	assert(hc);

	rh->hc = hc;
	rh->max_ports = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PORTS);
	rh->ports = calloc(rh->max_ports, sizeof(rh_port_t));
	if (!rh->ports)
		return ENOMEM;

	const errno_t err = bus_device_init(&rh->device.base, &rh->hc->bus.base);
	if (err) {
		free(rh->ports);
		return err;
	}

	rh->event_worker = joinable_fibril_create(&rh_worker, rh);
	if (!rh->event_worker) {
		free(rh->ports);
		return err;
	}

	for (unsigned i = 0; i < rh->max_ports; i++) {
		usb_port_init(&rh->ports[i].base);
		rh->ports[i].rh = rh;
		rh->ports[i].regs = &rh->hc->op_regs->portrs[i];
	}

	/* Initialize route string */
	rh->device.route_str = 0;

	xhci_sw_ring_init(&rh->event_ring, rh->max_ports);

	return EOK;
}

/**
 * Finalize the RH subsystem.
 */
errno_t xhci_rh_fini(xhci_rh_t *rh)
{
	assert(rh);
	xhci_rh_stop(rh);

	joinable_fibril_destroy(rh->event_worker);
	xhci_sw_ring_fini(&rh->event_ring);
	return EOK;
}

static rh_port_t *get_rh_port(usb_port_t *port)
{
	assert(port);
	return (rh_port_t *) port;
}

/**
 * Create and setup a device directly connected to RH. As the xHCI is not using
 * a virtual usbhub device for RH, this routine is called for devices directly.
 */
static errno_t rh_enumerate_device(usb_port_t *usb_port)
{
	errno_t err;
	rh_port_t *port = get_rh_port(usb_port);

	if (port->major <= 2) {
		/* USB ports for lower speeds needs their port reset first. */
		XHCI_REG_SET(port->regs, XHCI_PORT_PR, 1);
		if ((err = usb_port_wait_for_enabled(&port->base)))
			return err;
	} else {
		/* Do the Warm reset to ensure the state is clear. */
		XHCI_REG_SET(port->regs, XHCI_PORT_WPR, 1);
		if ((err = usb_port_wait_for_enabled(&port->base)))
			return err;
	}

	/*
	 * We cannot know in advance, whether the speed in the status register
	 * is valid - it depends on the protocol. So we read it later, but then
	 * we have to check if the port is still enabled.
	 */
	uint32_t status = XHCI_REG_RD_FIELD(&port->regs->portsc, 32);

	bool enabled = !!(status & XHCI_REG_MASK(XHCI_PORT_PED));
	if (!enabled)
		return ENOENT;

	unsigned psiv = (status & XHCI_REG_MASK(XHCI_PORT_PS)) >>
	    XHCI_REG_SHIFT(XHCI_PORT_PS);
	const usb_speed_t speed = port->rh->hc->speeds[psiv].usb_speed;

	device_t *dev = hcd_ddf_fun_create(&port->rh->hc->base, speed);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	dev->hub = &port->rh->device.base;
	dev->tier = 1;
	dev->port = port - port->rh->ports + 1;

	port->device = xhci_device_get(dev);
	port->device->rh_port = dev->port;

	usb_log_debug("Enumerating new %s-speed device on port %u.",
	    usb_str_speed(dev->speed), dev->port);

	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to enumerate USB device: %s", str_error(err));
		return err;
	}

	if (!ddf_fun_get_name(dev->fun)) {
		bus_device_set_default_name(dev);
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Failed to register device " XHCI_DEV_FMT " DDF function: %s.",
		    XHCI_DEV_ARGS(*port->device), str_error(err));
		goto err_usb_dev;
	}

	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	port->device = NULL;
	return err;
}

/**
 * Deal with a detached device.
 */
static void rh_remove_device(usb_port_t *usb_port)
{
	rh_port_t *port = get_rh_port(usb_port);

	assert(port->device);
	usb_log_info("Device " XHCI_DEV_FMT " at port %zu has been disconnected.",
	    XHCI_DEV_ARGS(*port->device), port - port->rh->ports + 1);

	/* Remove device from XHCI bus. */
	bus_device_gone(&port->device->base);

	/* Mark the device as detached. */
	port->device = NULL;
}

/**
 * Handle all changes on specified port.
 */
static void handle_port_change(xhci_rh_t *rh, uint8_t port_id)
{
	rh_port_t *const port = &rh->ports[port_id - 1];

	uint32_t status = XHCI_REG_RD_FIELD(&port->regs->portsc, 32);

	while (status & port_events_mask) {
		/*
		 * The PED bit in xHCI has RW1C semantics, which means that
		 * writing 1 to it will disable the port. Which means all
		 * standard mechanisms of register handling fails here.
		 */
		XHCI_REG_WR_FIELD(&port->regs->portsc,
		    status & ~XHCI_REG_MASK(XHCI_PORT_PED), 32);

		const bool connected = !!(status & XHCI_REG_MASK(XHCI_PORT_CCS));
		const bool enabled = !!(status & XHCI_REG_MASK(XHCI_PORT_PED));

		if (status & XHCI_REG_MASK(XHCI_PORT_CSC)) {
			usb_log_info("Connected state changed on port %u.", port_id);
			status &= ~XHCI_REG_MASK(XHCI_PORT_CSC);

			if (connected) {
				usb_port_connected(&port->base, &rh_enumerate_device);
			} else {
				usb_port_disabled(&port->base, &rh_remove_device);
			}
		}

		if (status & port_reset_mask) {
			status &= ~port_reset_mask;

			if (enabled) {
				usb_port_enabled(&port->base);
			} else {
				usb_port_disabled(&port->base, &rh_remove_device);
			}
		}

		status &= port_events_mask;
		if (status != 0)
			usb_log_debug("RH port %u change not handled: 0x%x", port_id, status);

		/* Make sure that PSCEG is 0 before exiting the loop. */
		status = XHCI_REG_RD_FIELD(&port->regs->portsc, 32);
	}
}

void xhci_rh_set_ports_protocol(xhci_rh_t *rh,
    unsigned offset, unsigned count, unsigned major)
{
	for (unsigned i = offset; i < offset + count; i++)
		rh->ports[i - 1].major = major;
}

void xhci_rh_start(xhci_rh_t *rh)
{
	xhci_sw_ring_restart(&rh->event_ring);
	joinable_fibril_start(rh->event_worker);

	/*
	 * The reset changed status of all ports, and SW originated reason does
	 * not cause an interrupt.
	 */
	for (uint8_t i = 0; i < rh->max_ports; ++i) {
		handle_port_change(rh, i + 1);

		rh_port_t *const port = &rh->ports[i];

		/*
		 * When xHCI starts, for some reasons USB 3 ports do not have
		 * the CSC bit, even though they are connected. Try to find
		 * such ports.
		 */
		if (XHCI_REG_RD(port->regs, XHCI_PORT_CCS) &&
		    port->base.state == PORT_DISABLED)
			usb_port_connected(&port->base, &rh_enumerate_device);
	}
}

/**
 * Disconnect all devices on all ports. On contrary to ordinary disconnect, this
 * function waits until the disconnection routine is over.
 */
void xhci_rh_stop(xhci_rh_t *rh)
{
	xhci_sw_ring_stop(&rh->event_ring);
	joinable_fibril_join(rh->event_worker);

	for (uint8_t i = 0; i < rh->max_ports; ++i) {
		rh_port_t *const port = &rh->ports[i];
		usb_port_disabled(&port->base, &rh_remove_device);
		usb_port_fini(&port->base);
	}
}

static int rh_worker(void *arg)
{
	xhci_rh_t *const rh = arg;

	xhci_trb_t trb;
	while (xhci_sw_ring_dequeue(&rh->event_ring, &trb) == EOK) {
		uint8_t port_id = XHCI_QWORD_EXTRACT(trb.parameter, 31, 24);
		usb_log_debug("Port status change event detected for port %u.", port_id);
		handle_port_change(rh, port_id);
	}

	return 0;
}

/**
 * @}
 */
