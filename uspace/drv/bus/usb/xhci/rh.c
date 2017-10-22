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

int xhci_rh_init(xhci_rh_t *rh, xhci_hc_t *hc, ddf_dev_t *device)
{
	assert(rh);
	assert(hc);

	rh->hc = hc;
	rh->max_ports = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PORTS);
	rh->devices = (xhci_device_t **) calloc(rh->max_ports, sizeof(xhci_device_t *));
	hc->rh.hc_device = device;

	return device_init(&hc->rh.device);
}

static void setup_control_ep0_ctx(xhci_ep_ctx_t *ctx, xhci_trb_ring_t *ring,
		const xhci_port_speed_t *speed)
{
	XHCI_EP_TYPE_SET(*ctx, EP_TYPE_CONTROL);
	// TODO: must be changed with a command after USB descriptor is read
	// See 4.6.5 in XHCI specification, first note
	XHCI_EP_MAX_PACKET_SIZE_SET(*ctx, speed->major == 3 ? 512 : 8);
	XHCI_EP_MAX_BURST_SIZE_SET(*ctx, 0);
	XHCI_EP_TR_DPTR_SET(*ctx, ring->dequeue);
	XHCI_EP_DCS_SET(*ctx, 1);
	XHCI_EP_INTERVAL_SET(*ctx, 0);
	XHCI_EP_MAX_P_STREAMS_SET(*ctx, 0);
	XHCI_EP_MULT_SET(*ctx, 0);
	XHCI_EP_ERROR_COUNT_SET(*ctx, 3);
}

// TODO: This currently assumes the device is attached to rh directly.
//       Also, we should consider moving a lot of functionailty to xhci bus
int xhci_rh_address_device(xhci_rh_t *rh, device_t *dev, xhci_bus_t *bus)
{
	int err;
	xhci_device_t *xhci_dev = xhci_device_get(dev);

	/* FIXME: Certainly not generic solution. */
	const uint32_t route_str = 0;

	xhci_dev->hc = rh->hc;

	const xhci_port_speed_t *speed = xhci_rh_get_port_speed(rh, dev->port);
	xhci_dev->usb3 = speed->major == 3;

	/* Enable new slot */
	if ((err = hc_enable_slot(rh->hc, &xhci_dev->slot_id)) != EOK)
		return err;
	usb_log_debug2("Obtained slot ID: %u.\n", xhci_dev->slot_id);

	/* Setup input context */
	xhci_input_ctx_t *ictx = malloc32(sizeof(xhci_input_ctx_t));
	if (!ictx)
		return ENOMEM;
	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 1);

	/* Initialize slot_ctx according to section 4.3.3 point 3. */
	/* Attaching to root hub port, root string equals to 0. */
	XHCI_SLOT_ROOT_HUB_PORT_SET(ictx->slot_ctx, dev->port);
	XHCI_SLOT_CTX_ENTRIES_SET(ictx->slot_ctx, 1);
	XHCI_SLOT_ROUTE_STRING_SET(ictx->slot_ctx, route_str);

	endpoint_t *ep0_base = bus_create_endpoint(&rh->hc->bus.base);
	if (!ep0_base)
		goto err_ictx;
	xhci_endpoint_t *ep0 = xhci_endpoint_get(ep0_base);

	/* Control endpoints don't use streams. */
	/* FIXME: Sync this with xhci_device_add_endpoint. */
	ep0->max_streams = 0;
	ep0->max_burst = 0;
	ep0->mult = 0;
	if ((err = xhci_endpoint_alloc_transfer_ds(ep0)))
		goto err_ictx;

	setup_control_ep0_ctx(&ictx->endpoint_ctx[0], &ep0->ring, speed);

	/* Setup and register device context */
	xhci_device_ctx_t *dctx = malloc32(sizeof(xhci_device_ctx_t));
	if (!dctx) {
		err = ENOMEM;
		goto err_ep;
	}
	xhci_dev->dev_ctx = dctx;
	rh->hc->dcbaa[xhci_dev->slot_id] = addr_to_phys(dctx);
	memset(dctx, 0, sizeof(xhci_device_ctx_t));

	/* Address device */
	if ((err = hc_address_device(rh->hc, xhci_dev->slot_id, ictx)) != EOK)
		goto err_dctx;
	dev->address = XHCI_SLOT_DEVICE_ADDRESS(dctx->slot_ctx);
	usb_log_debug2("Obtained USB address: %d.\n", dev->address);

	/* From now on, the device is officially online, yay! */
	fibril_mutex_lock(&dev->guard);
	xhci_dev->online = true;
	fibril_mutex_unlock(&dev->guard);

	// XXX: Going around bus, duplicating code
	ep0_base->device = dev;
	ep0_base->target.address = dev->address;
	ep0_base->target.endpoint = 0;
	ep0_base->direction = USB_DIRECTION_BOTH;
	ep0_base->transfer_type = USB_TRANSFER_CONTROL;
	ep0_base->max_packet_size = CTRL_PIPE_MIN_PACKET_SIZE;
	ep0_base->packets = 1;
	ep0_base->bandwidth = CTRL_PIPE_MIN_PACKET_SIZE;

	bus_register_endpoint(&rh->hc->bus.base, ep0_base);

	if (!rh->devices[dev->port - 1]) {
		/* Only save the device if it's the first one connected to this port. */
		rh->devices[dev->port - 1] = xhci_dev;
	}

	free32(ictx);
	return EOK;

err_dctx:
	free32(dctx);
	rh->hc->dcbaa[xhci_dev->slot_id] = 0;
err_ep:
	xhci_endpoint_fini(ep0);
	free(ep0);
err_ictx:
	free32(ictx);
	return err;
}

/** Create a device node for device directly connected to RH.
 */
static int rh_setup_device(xhci_rh_t *rh, uint8_t port_id)
{
	int err;
	assert(rh);
	assert(rh->hc_device);

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

/** Deal with a detached device.
 */
static int handle_disconnected_device(xhci_rh_t *rh, uint8_t port_id)
{
	assert(rh);
	int err;

	/* Find XHCI device by the port. */
	xhci_device_t *dev = rh->devices[port_id - 1];
	if (!dev) {
		/* Must be extraneous call. */
		return EOK;
	}

	usb_log_info("Device '%s' at port %u has been disconnected.", ddf_fun_get_name(dev->base.fun), port_id);

	/* Block creation of new endpoints and transfers. */
	fibril_mutex_lock(&dev->base.guard);
	dev->online = false;
	fibril_mutex_unlock(&dev->base.guard);

	fibril_mutex_lock(&rh->device.guard);
	list_remove(&dev->base.link);
	fibril_mutex_unlock(&rh->device.guard);

	rh->devices[port_id - 1] = NULL;
	usb_log_debug2("Aborting all active transfers to '%s'.", ddf_fun_get_name(dev->base.fun));

	/* Abort running transfers. */
	for (size_t i = 0; i < ARRAY_SIZE(dev->endpoints); ++i) {
		xhci_endpoint_t *ep = dev->endpoints[i];
		if (!ep || !ep->base.active)
			continue;

		if ((err = xhci_transfer_abort(&ep->active_transfer))) {
			usb_log_warning("Failed to abort active %s transfer to "
			    " endpoint %d of detached device '%s': %s",
			    usb_str_transfer_type(ep->base.transfer_type),
			    ep->base.target.endpoint, ddf_fun_get_name(dev->base.fun),
			    str_error(err));
		}
	}

	/* Make DDF (and all drivers) forget about the device. */
	if ((err = ddf_fun_unbind(dev->base.fun))) {
		usb_log_warning("Failed to unbind DDF function of detached device '%s': %s",
		    ddf_fun_get_name(dev->base.fun), str_error(err));
	}

	/* FIXME:
	 * A deadlock happens on the previous line. For some reason, the HID driver
	 * does not fully release its DDF function (tracked it down to release_endpoint
 	 * in usb_remote.c so far).
	 *
	 * For that reason as well, the following 3 lines are untested.
	 */

	xhci_bus_remove_device(&rh->hc->bus, rh->hc, &dev->base);
	hc_disable_slot(rh->hc, dev->slot_id);
	hcd_ddf_device_destroy(&dev->base);

	// TODO: Free device context.
	// TODO: Free TRB rings.
	// TODO: Figure out what was forgotten and free that as well.

	return EOK;
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

static inline int get_hub_available_bandwidth(xhci_device_t* dev, uint8_t speed, xhci_port_bandwidth_ctx_t *ctx) {
	// TODO: find a correct place for this function + API
	// We need speed, because a root hub device has both USB 2 and USB 3 speeds
	// and the command can query only one of them
	// ctx is an out parameter as of now
	assert(dev);

	ctx = malloc(sizeof(xhci_port_bandwidth_ctx_t));
	if(!ctx)
		return ENOMEM;

	xhci_cmd_t cmd;
	xhci_cmd_init(&cmd);

	xhci_get_port_bandwidth_command(dev->hc, &cmd, ctx, speed);

	int err = xhci_cmd_wait(&cmd, XHCI_DEFAULT_TIMEOUT);
	if(err != EOK) {
		free(ctx);
		ctx = NULL;
	}

	return EOK;
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

	free(rh->devices);

	return EOK;
}

/**
 * @}
 */
