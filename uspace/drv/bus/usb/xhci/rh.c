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

enum {
	HUB_STATUS_CHANGE_PIPE = 1,
};

static usbvirt_device_ops_t ops;

int xhci_rh_init(xhci_rh_t *rh)
{
	return virthub_base_init(&rh->base, "xhci rh", &ops, rh, NULL,
	    &rh->hub_descriptor.header, HUB_STATUS_CHANGE_PIPE);
}

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

	memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
	hc->dcbaa_virt[slot_id].dev_ctx = dctx;

	cmd = xhci_alloc_command();
	cmd->ictx = ictx;
	xhci_send_address_device_command(hc, cmd);
	if ((err = xhci_wait_for_command(cmd, 100000)) != EOK)
		goto err_dctx;

	xhci_free_command(cmd);
	ictx = NULL;

        // TODO: Issue configure endpoint commands (sec 4.3.5).

	return EOK;

err_dctx:
	if (dctx) {
		free32(dctx);
		hc->dcbaa[slot_id] = 0;
		memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
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
	xhci_port_regs_t *regs = &hc->op_regs->portrs[port-1];
	XHCI_REG_WR(regs, XHCI_PORT_PR, 1);

	return EOK;
}

int xhci_rh_schedule(xhci_rh_t *rh, usb_transfer_batch_t *batch)
{
	assert(rh);
	assert(batch);
	const usb_target_t target = {{
		.address = batch->ep->address,
		.endpoint = batch->ep->endpoint,
	}};
	batch->error = virthub_base_request(&rh->base, target,
	    usb_transfer_batch_direction(batch), (void*)batch->setup_buffer,
	    batch->buffer, batch->buffer_size, &batch->transfered_size);
	if (batch->error == ENAK) {
		/* This is safe because only status change interrupt transfers
		 * return NAK. The assertion holds true because the batch
		 * existence prevents communication with that ep */
		assert(rh->unfinished_interrupt_transfer == NULL);
		rh->unfinished_interrupt_transfer = batch;
	} else {
		usb_transfer_batch_finish(batch, NULL);
		usb_transfer_batch_destroy(batch);
	}
	return EOK;
}

int xhci_rh_interrupt(xhci_rh_t *rh)
{
	usb_log_debug2("Called xhci_rh_interrupt().");

	/* TODO: atomic swap needed */
	usb_transfer_batch_t *batch = rh->unfinished_interrupt_transfer;
	rh->unfinished_interrupt_transfer = NULL;
	if (batch) {
		const usb_target_t target = {{
			.address = batch->ep->address,
			.endpoint = batch->ep->endpoint,
		}};
		batch->error = virthub_base_request(&rh->base, target,
		    usb_transfer_batch_direction(batch),
		    (void*)batch->setup_buffer,
		    batch->buffer, batch->buffer_size, &batch->transfered_size);
		usb_transfer_batch_finish(batch, NULL);
		usb_transfer_batch_destroy(batch);
	}
	return EOK;
}

/** Hub status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static int req_get_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_get_status().");
	return EOK;
}

/** Hub set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static int req_clear_hub_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_clear_hub_feature().");
	return EOK;
}

/** Port status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static int req_get_port_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_get_port_status().");
	return EOK;
}

/** Port clear feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static int req_clear_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_clear_port_feature().");
	return EOK;
}

/** Port set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static int req_set_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_set_port_feature().");
	return EOK;
}

/** Status change handler.
 * @param device Virtual hub device
 * @param endpoint Endpoint number
 * @param tr_type Transfer type
 * @param buffer Response destination
 * @param buffer_size Bytes available in buffer
 * @param actual_size Size us the used part of the dest buffer.
 *
 * Produces status mask. Bit 0 indicates hub status change the other bits
 * represent port status change. Endian does not matter as UHCI root hubs
 * only need 1 byte.
 */
static int req_status_change_handler(usbvirt_device_t *device,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called req_status_change_handler().");
	return ENAK;
}

int xhci_rh_fini(xhci_rh_t *rh)
{
	/* TODO: Implement me! */
	usb_log_debug2("Called xhci_rh_fini().");
	return EOK;
}

/** XHCI root hub request handlers */
static const usbvirt_control_request_handler_t control_transfer_handlers[] = {
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_DESCRIPTOR),
		.name = "GetHubDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		.callback = req_clear_hub_feature,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		.callback = req_get_status,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		.callback = req_nop,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature,
	},
	{
		.callback = NULL
	}
};

/** Virtual XHCI root hub ops */
static usbvirt_device_ops_t ops = {
        .control = control_transfer_handlers,
        .data_in[HUB_STATUS_CHANGE_PIPE] = req_status_change_handler,
};


/**
 * @}
 */
