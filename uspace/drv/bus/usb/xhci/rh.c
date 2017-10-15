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
#include <usb/host/utils/malloc32.h>
#include <usb/host/bus.h>
#include <usb/host/ddf_helpers.h>
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

int xhci_rh_init(xhci_rh_t *rh, xhci_hc_t *hc)
{
	assert(rh);
	assert(hc);

	rh->hc = hc;
	rh->max_ports = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PORTS);

	return EOK;
}

// TODO: Check device deallocation, we free device_ctx in hc.c, not
//       sure about the other structs.
// TODO: This currently assumes the device is attached to rh directly.
//       Also, we should consider moving a lot of functionailty to xhci bus
int xhci_rh_address_device(xhci_rh_t *rh, device_t *dev, xhci_bus_t *bus)
{
	int err;
	xhci_hc_t *hc = rh->hc;

	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd);

	/* XXX Certainly not generic solution. */
	uint32_t route_str = 0;

	const xhci_port_speed_t *speed = xhci_rh_get_port_speed(rh, dev->port);

	xhci_send_enable_slot_command(hc, &cmd);
	if ((err = xhci_cmd_wait(&cmd, 100000)) != EOK)
		return err;

	uint32_t slot_id = cmd.slot_id;

	usb_log_debug2("Obtained slot ID: %u.\n", slot_id);
	xhci_cmd_fini(&cmd);

	xhci_input_ctx_t *ictx = malloc(sizeof(xhci_input_ctx_t));
	if (!ictx) {
		return ENOMEM;
	}

	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 1);

	/* Initialize slot_ctx according to section 4.3.3 point 3. */
	/* Attaching to root hub port, root string equals to 0. */
	XHCI_SLOT_ROOT_HUB_PORT_SET(ictx->slot_ctx, dev->port);
	XHCI_SLOT_CTX_ENTRIES_SET(ictx->slot_ctx, 1);
	XHCI_SLOT_ROUTE_STRING_SET(ictx->slot_ctx, route_str);

	xhci_trb_ring_t *ep_ring = malloc(sizeof(xhci_trb_ring_t));
	if (!ep_ring) {
		err = ENOMEM;
		goto err_ictx;
	}

	err = xhci_trb_ring_init(ep_ring);
	if (err)
		goto err_ring;

	XHCI_EP_TYPE_SET(ictx->endpoint_ctx[0], EP_TYPE_CONTROL);
	// TODO: must be changed with a command after USB descriptor is read
	// See 4.6.5 in XHCI specification, first note
	XHCI_EP_MAX_PACKET_SIZE_SET(ictx->endpoint_ctx[0],
	    speed->major == 3 ? 512 : 8);
	XHCI_EP_MAX_BURST_SIZE_SET(ictx->endpoint_ctx[0], 0);
	/* FIXME physical pointer? */
	XHCI_EP_TR_DPTR_SET(ictx->endpoint_ctx[0], ep_ring->dequeue);
	XHCI_EP_DCS_SET(ictx->endpoint_ctx[0], 1);
	XHCI_EP_INTERVAL_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_MAX_P_STREAMS_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_MULT_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_ERROR_COUNT_SET(ictx->endpoint_ctx[0], 3);

	xhci_device_ctx_t *dctx = malloc32(sizeof(xhci_device_ctx_t));
	if (!dctx) {
		err = ENOMEM;
		goto err_ring;
	}
	memset(dctx, 0, sizeof(xhci_device_ctx_t));

	hc->dcbaa[slot_id] = addr_to_phys(dctx);

	memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
	hc->dcbaa_virt[slot_id].dev_ctx = dctx;
	hc->dcbaa_virt[slot_id].trs[0] = ep_ring;

	xhci_cmd_init(&cmd);
	cmd.slot_id = slot_id;
	xhci_send_address_device_command(hc, &cmd, ictx);
	if ((err = xhci_cmd_wait(&cmd, 100000)) != EOK)
		goto err_dctx;

	xhci_cmd_fini(&cmd);

	dev->address = XHCI_SLOT_DEVICE_ADDRESS(dctx->slot_ctx);
	usb_log_debug2("Obtained USB address: %d.\n", dev->address);

	// Ask libusbhost to create a control endpoint for EP0.
	bus_t *bus_base = &bus->base;
	usb_target_t ep0_target = { .address = dev->address, .endpoint = 0 };
	usb_direction_t ep0_direction = USB_DIRECTION_BOTH;

	// TODO: Should this call be unified with other calls to `bus_add_ep()`?
	err = bus_add_ep(bus_base, dev, ep0_target.endpoint, ep0_direction,
	    USB_TRANSFER_CONTROL, CTRL_PIPE_MIN_PACKET_SIZE, CTRL_PIPE_MIN_PACKET_SIZE, 1);

	if (err != EOK)
		goto err_add_ep;

	// Save all data structures in the corresponding xhci_device_t.
	endpoint_t *ep0_base = bus_find_endpoint(bus_base, ep0_target, ep0_direction);
	xhci_endpoint_t *ep0 = xhci_endpoint_get(ep0_base);
	xhci_device_t *xhci_dev = ep0->device;

	xhci_dev->device = dev;
	xhci_dev->slot_id = slot_id;
	xhci_dev->usb3 = speed->major == 3;
	xhci_dev->hc = hc;

	// TODO: Save anything else?

	return EOK;

err_add_ep:
err_dctx:
	if (dctx) {
		free(dctx);
		hc->dcbaa[slot_id] = 0;
		memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
	}
err_ring:
	if (ep_ring) {
		xhci_trb_ring_fini(ep_ring);
		free(ep_ring);
	}
err_ictx:
	free(ictx);
	return err;
}

/** Create a device node for device directly connected to RH.
 */
static int rh_setup_device(xhci_rh_t *rh, uint8_t port_id)
{
	int err;
	assert(rh);

	xhci_bus_t *bus = &rh->hc->bus;

	device_t *dev = hcd_ddf_device_create(rh->hc_device, bus->base.device_size);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	dev->hub = &rh->device;
	dev->port = port_id;

	if ((err = xhci_bus_enumerate_device(bus, rh->hc, dev))) {
		usb_log_error("Failed to enumerate USB device: %s", str_error(err));
		return err;
	}

	if (!ddf_fun_get_name(dev->fun)) {
		device_set_default_name(dev);
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Device(%d): Failed to register: %s.", dev->address, str_error(err));
		goto err_usb_dev;
	}

	fibril_mutex_lock(&rh->device.guard);
	list_append(&dev->link, &rh->device.devices);
	fibril_mutex_unlock(&rh->device.guard);

	return EOK;

err_usb_dev:
	hcd_ddf_device_destroy(dev);
	return err;
}

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
		/*
			FIXME: we need to wait for the event triggered by the reset
			and then alloc_dev()... can't it be done directly instead of
			going around?
		*/
		return EOK;
	}
}

/** Handle an incoming Port Change Detected Event.
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
			if (connected)
				handle_connected_device(rh, i);
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

const xhci_port_speed_t *xhci_rh_get_port_speed(xhci_rh_t *rh, uint8_t port)
{
	xhci_port_regs_t *port_regs = &rh->hc->op_regs->portrs[port - 1];

	unsigned psiv = XHCI_REG_RD(port_regs, XHCI_PORT_PS);
	return &rh->speeds[psiv];
}

int xhci_rh_reset_port(xhci_rh_t* rh, uint8_t port)
{
	usb_log_debug2("Resetting port %u.", port);
	xhci_port_regs_t *regs = &rh->hc->op_regs->portrs[port-1];
	XHCI_REG_SET(regs, XHCI_PORT_PR, 1);

	return EOK;
}

int xhci_rh_fini(xhci_rh_t *rh)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called xhci_rh_fini().");
	return EOK;
}

/**
 * @}
 */
