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
#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>
#include "debug.h"
#include "commands.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "rh.h"

// TODO: Check device deallocation, we free device_ctx in hc.c, not
//       sure about the other structs.
static int alloc_dev(xhci_hc_t *hc, uint8_t port)
{
	int err;

	xhci_cmd_t *cmd = xhci_alloc_command();
	if (!cmd)
		return ENOMEM;

	xhci_send_enable_slot_command(hc, cmd);
	if ((err = xhci_wait_for_command(cmd, 100000)) != EOK)
		goto err_command;

	uint32_t slot_id = cmd->slot_id;
	usb_log_debug2("Obtained slot ID: %u.\n", slot_id);

	xhci_free_command(cmd);
	cmd = NULL;

	xhci_input_ctx_t *ictx = malloc32(sizeof(xhci_input_ctx_t));
	if (!ictx) {
		err = ENOMEM;
		goto err_command;
	}

	memset(ictx, 0, sizeof(xhci_input_ctx_t));

	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);
	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 1);

   	/* Initialize slot_ctx according to section 4.3.3 point 3. */
	/* Attaching to root hub port, root string equals to 0. */
	// TODO: shouldn't these macros consider endianity?
	XHCI_SLOT_ROOT_HUB_PORT_SET(ictx->slot_ctx, port);
	XHCI_SLOT_CTX_ENTRIES_SET(ictx->slot_ctx, 1);

	// TODO: where do we save this? the ring should be associated with device structure somewhere
	xhci_trb_ring_t *ep_ring = malloc32(sizeof(xhci_trb_ring_t));
	if (!ep_ring) {
		err = ENOMEM;
		goto err_ictx;
	}

	err = xhci_trb_ring_init(ep_ring, hc);
	if (err)
		goto err_ring;

	xhci_port_regs_t *regs = &hc->op_regs->portrs[port - 1];
	uint8_t port_speed_id = XHCI_REG_RD(regs, XHCI_PORT_PS);

	XHCI_EP_TYPE_SET(ictx->endpoint_ctx[0], 4);
	XHCI_EP_MAX_PACKET_SIZE_SET(ictx->endpoint_ctx[0],
	    hc->speeds[port_speed_id].tx_bps);
	XHCI_EP_MAX_BURST_SIZE_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_TR_DPTR_SET(ictx->endpoint_ctx[0], ep_ring->dequeue);
	XHCI_EP_DCS_SET(ictx->endpoint_ctx[0], 1);
	XHCI_EP_INTERVAL_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_MAX_P_STREAMS_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_MULT_SET(ictx->endpoint_ctx[0], 0);
	XHCI_EP_ERROR_COUNT_SET(ictx->endpoint_ctx[0], 3);

	// TODO: What's the alignment?
	xhci_device_ctx_t *dctx = malloc32(sizeof(xhci_device_ctx_t));
	if (!dctx) {
		err = ENOMEM;
		goto err_ring;
	}
	memset(dctx, 0, sizeof(xhci_device_ctx_t));

	hc->dcbaa[slot_id] = addr_to_phys(dctx);
	hc->dcbaa_virt[slot_id] = dctx;

	cmd = xhci_alloc_command();
	cmd->ictx = ictx;
	xhci_send_address_device_command(hc, cmd);
	if ((err = xhci_wait_for_command(cmd, 100000)) != EOK)
		goto err_dctx;

	return EOK;

err_dctx:
	if (dctx) {
		free32(dctx);
		hc->dcbaa[slot_id] = 0;
		hc->dcbaa_virt[slot_id] = NULL;
	}
err_ring:
	if (ep_ring) {
		xhci_trb_ring_fini(ep_ring);
		free32(ep_ring);
	}
err_ictx:
	if (ictx) {
		/* Avoid double free. */
		if (cmd && cmd->ictx && cmd->ictx == ictx)
			cmd->ictx = NULL;
		free32(ictx);
	}
err_command:
	if (cmd)
		xhci_free_command(cmd);
	return err;
}

static int handle_connected_device(xhci_hc_t* hc, xhci_port_regs_t* regs, uint8_t port_id)
{
	uint8_t link_state = XHCI_REG_RD(regs, XHCI_PORT_PLS);
	// FIXME: do we have a better way to detect if this is usb2 or usb3 device?
	if (link_state == 0) {
		/* USB3 is automatically advanced to enabled. */
		uint8_t port_speed = XHCI_REG_RD(regs, XHCI_PORT_PS);
		usb_log_debug2("Detected new device on port %u, port speed id %u.", port_id, port_speed);

		alloc_dev(hc, port_id);
	} else if (link_state == 5) {
		/* USB 3 failed to enable. */
		usb_log_debug("USB 3 port couldn't be enabled.");
	} else if (link_state == 7) {
		usb_log_debug("USB 2 device attached, issuing reset.");
		xhci_reset_hub_port(hc, port_id);
	}

	return EOK;
}

int xhci_handle_port_status_change_event(xhci_hc_t *hc, xhci_trb_t *trb)
{
	uint8_t port_id = xhci_get_hub_port(trb);
	usb_log_debug("Port status change event detected for port %u.", port_id);
	xhci_port_regs_t* regs = &hc->op_regs->portrs[port_id - 1];

	/* Port reset change */
	if (XHCI_REG_RD(regs, XHCI_PORT_PRC)) {
		/* Clear the flag. */
		XHCI_REG_WR(regs, XHCI_PORT_PRC, 1);

		uint8_t port_speed = XHCI_REG_RD(regs, XHCI_PORT_PS);
		usb_log_debug2("Detected port reset on port %u, port speed id %u.", port_id, port_speed);
		alloc_dev(hc, port_id);
	}

	/* Connection status change */
	if (XHCI_REG_RD(regs, XHCI_PORT_CSC)) {
		XHCI_REG_WR(regs, XHCI_PORT_CSC, 1);

		if (XHCI_REG_RD(regs, XHCI_PORT_CCS) == 1) {
			handle_connected_device(hc, regs, port_id);
		} else {
			// TODO: Device disconnected
		}
	}

	return EOK;
}

int xhci_get_hub_port(xhci_trb_t *trb)
{
	assert(trb);
	uint8_t port_id = XHCI_QWORD_EXTRACT(trb->parameter, 31, 24);

	return port_id;   
}

int xhci_reset_hub_port(xhci_hc_t* hc, uint8_t port)
{
	usb_log_debug2("Resetting port %u.", port);
	xhci_port_regs_t regs = hc->op_regs->portrs[port];
	XHCI_REG_WR(&regs, XHCI_PORT_PR, 1);

	return EOK;
}


/**
 * @}
 */
