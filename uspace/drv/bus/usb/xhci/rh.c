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
#include "endpoint.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "rh.h"

#define USB_MAP_VALUE(a, b) [USB_HUB_FEATURE_##a] = b
#define USB_MAP_XHCI(a, b) USB_MAP_VALUE(a, XHCI_REG_MASK(XHCI_PORT_##b))

enum {
	HUB_STATUS_CHANGE_PIPE = 1,
};

static usbvirt_device_ops_t ops;

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

	usb_hub_descriptor_header_t *header = &rh->hub_descriptor.header;
	header->length = sizeof(usb_hub_descriptor_header_t);
	header->descriptor_type = USB_DESCTYPE_HUB;
	header->port_count = rh->max_ports;
	header->characteristics =
		    HUB_CHAR_NO_POWER_SWITCH_FLAG | HUB_CHAR_NO_OC_FLAG;
	header->power_good_time = 10; /* XHCI section 5.4.9 says 20ms max */
	header->max_current = 0;

	return virthub_base_init(&rh->base, "xhci", &ops, rh, NULL,
	    header, HUB_STATUS_CHANGE_PIPE);
}

// TODO: Check device deallocation, we free device_ctx in hc.c, not
//       sure about the other structs.
// static int alloc_dev(xhci_hc_t *hc, uint8_t port, uint32_t route_str)
// {
// 	int err;
//
// 	xhci_cmd_t cmd;
// 	xhci_cmd_init(&cmd);
//
// 	xhci_send_enable_slot_command(hc, &cmd);
// 	if ((err = xhci_cmd_wait(&cmd, 100000)) != EOK)
// 		return err;
//
// 	uint32_t slot_id = cmd.slot_id;
//
// 	usb_log_debug2("Obtained slot ID: %u.\n", slot_id);
// 	xhci_cmd_fini(&cmd);
//
// 	xhci_input_ctx_t *ictx = malloc32(sizeof(xhci_input_ctx_t));
// 	if (!ictx) {
// 		return ENOMEM;
// 	}
//
// 	memset(ictx, 0, sizeof(xhci_input_ctx_t));
//
// 	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 0);
// 	XHCI_INPUT_CTRL_CTX_ADD_SET(ictx->ctrl_ctx, 1);
//
// 	/* Initialize slot_ctx according to section 4.3.3 point 3. */
// 	/* Attaching to root hub port, root string equals to 0. */
// 	XHCI_SLOT_ROOT_HUB_PORT_SET(ictx->slot_ctx, port);
// 	XHCI_SLOT_CTX_ENTRIES_SET(ictx->slot_ctx, 1);
// 	XHCI_SLOT_ROUTE_STRING_SET(ictx->slot_ctx, route_str);
//
// 	xhci_trb_ring_t *ep_ring = malloc32(sizeof(xhci_trb_ring_t));
// 	if (!ep_ring) {
// 		err = ENOMEM;
// 		goto err_ictx;
// 	}
//
// 	err = xhci_trb_ring_init(ep_ring, hc);
// 	if (err)
// 		goto err_ring;
//
// 	XHCI_EP_TYPE_SET(ictx->endpoint_ctx[0], EP_TYPE_CONTROL);
// 	// TODO: must be changed with a command after USB descriptor is read
// 	// See 4.6.5 in XHCI specification, first note
// 	XHCI_EP_MAX_PACKET_SIZE_SET(ictx->endpoint_ctx[0],
// 	    xhci_is_usb3_port(&hc->rh, port) ? 512 : 8);
// 	XHCI_EP_MAX_BURST_SIZE_SET(ictx->endpoint_ctx[0], 0);
// 	/* FIXME physical pointer? */
// 	XHCI_EP_TR_DPTR_SET(ictx->endpoint_ctx[0], ep_ring->dequeue);
// 	XHCI_EP_DCS_SET(ictx->endpoint_ctx[0], 1);
// 	XHCI_EP_INTERVAL_SET(ictx->endpoint_ctx[0], 0);
// 	XHCI_EP_MAX_P_STREAMS_SET(ictx->endpoint_ctx[0], 0);
// 	XHCI_EP_MULT_SET(ictx->endpoint_ctx[0], 0);
// 	XHCI_EP_ERROR_COUNT_SET(ictx->endpoint_ctx[0], 3);
//
// 	// TODO: What's the alignment?
// 	xhci_device_ctx_t *dctx = malloc32(sizeof(xhci_device_ctx_t));
// 	if (!dctx) {
// 		err = ENOMEM;
// 		goto err_ring;
// 	}
// 	memset(dctx, 0, sizeof(xhci_device_ctx_t));
//
// 	hc->dcbaa[slot_id] = addr_to_phys(dctx);
//
// 	memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
// 	hc->dcbaa_virt[slot_id].dev_ctx = dctx;
//	hc->dcbaa_virt[slot_id].trs[0] = ep_ring;
//
// 	xhci_cmd_init(&cmd);
// 	cmd.slot_id = slot_id;
// 	xhci_send_address_device_command(hc, &cmd, ictx);
// 	if ((err = xhci_cmd_wait(&cmd, 100000)) != EOK)
// 		goto err_dctx;
//
// 	xhci_cmd_fini(&cmd);
//
// 	// TODO: Issue configure endpoint commands (sec 4.3.5).
//
// 	return EOK;
//
// err_dctx:
// 	if (dctx) {
// 		free32(dctx);
// 		hc->dcbaa[slot_id] = 0;
// 		memset(&hc->dcbaa_virt[slot_id], 0, sizeof(xhci_virt_device_ctx_t));
// 	}
// err_ring:
// 	if (ep_ring) {
// 		xhci_trb_ring_fini(ep_ring);
// 		free32(ep_ring);
// 	}
// err_ictx:
// 	free32(ictx);
// 	return err;
// }

// static int handle_connected_device(xhci_hc_t* hc, xhci_port_regs_t* regs, uint8_t port_id)
// {
// 	uint8_t link_state = XHCI_REG_RD(regs, XHCI_PORT_PLS);
// 	const xhci_port_speed_t *speed = xhci_get_port_speed(&hc->rh, port_id);
//
// 	usb_log_info("Detected new %.4s%u.%u device on port %u.", speed->name, speed->major, speed->minor, port_id);
//
// 	if (speed->major == 3) {
// 		if(link_state == 0) {
// 			/* USB3 is automatically advanced to enabled. */
// 			return alloc_dev(hc, port_id, 0);
// 		}
// 		else if (link_state == 5) {
// 			/* USB 3 failed to enable. */
// 			usb_log_error("USB 3 port couldn't be enabled.");
// 			return EAGAIN;
// 		}
// 		else {
// 			usb_log_error("USB 3 port is in invalid state %u.", link_state);
// 			return EINVAL;
// 		}
// 	}
// 	else {
// 		usb_log_debug("USB 2 device attached, issuing reset.");
// 		xhci_reset_hub_port(hc, port_id);
// 		/*
// 			FIXME: we need to wait for the event triggered by the reset
// 			and then alloc_dev()... can't it be done directly instead of
// 			going around?
// 		*/
// 		return EOK;
// 	}
// }

int xhci_handle_port_status_change_event(xhci_hc_t *hc, xhci_trb_t *trb)
{
	int err;

	uint8_t port_id = xhci_get_hub_port(trb);
	usb_log_debug("Port status change event detected for port %u.", port_id);

	// Interrupt on the virtual hub status change pipe.
	err = xhci_rh_interrupt(&hc->rh);
	if (err != EOK) {
		usb_log_warning("Invoking interrupt on virtual hub failed: %s",
		    str_error(err));
	}

	return EOK;
}

const xhci_port_speed_t *xhci_get_port_speed(xhci_rh_t *rh, uint8_t port)
{
	xhci_port_regs_t *port_regs = &rh->hc->op_regs->portrs[port - 1];

	unsigned psiv = XHCI_REG_RD(port_regs, XHCI_PORT_PS);
	return &rh->speeds[psiv];
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

#define XHCI_TO_USB(usb_feat, reg_set, ...) \
	(((XHCI_REG_RD(reg_set, ##__VA_ARGS__)) ? 1 : 0) << (usb_feat))

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
	xhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (!setup_packet->index || setup_packet->index > hub->max_ports) {
		return ESTALL;
	}

	/* The index is 1-based. */
	xhci_port_regs_t* regs = &hub->hc->op_regs->portrs[setup_packet->index - 1];

	const uint32_t status = uint32_host2usb(
	    XHCI_TO_USB(USB_HUB_FEATURE_C_PORT_CONNECTION, regs, XHCI_PORT_CSC) |
	    XHCI_TO_USB(USB_HUB_FEATURE_C_PORT_ENABLE, regs, XHCI_PORT_PEC) |
	    XHCI_TO_USB(USB_HUB_FEATURE_C_PORT_OVER_CURRENT, regs, XHCI_PORT_OCC) |
	    XHCI_TO_USB(USB_HUB_FEATURE_C_PORT_RESET, regs, XHCI_PORT_PRC) |
	    XHCI_TO_USB(USB_HUB_FEATURE_PORT_CONNECTION, regs, XHCI_PORT_CCS) |
	    XHCI_TO_USB(USB_HUB_FEATURE_PORT_ENABLE, regs, XHCI_PORT_PED) |
	    XHCI_TO_USB(USB_HUB_FEATURE_PORT_OVER_CURRENT, regs, XHCI_PORT_OCA) |
	    XHCI_TO_USB(USB_HUB_FEATURE_PORT_RESET, regs, XHCI_PORT_PR) |
	    XHCI_TO_USB(USB_HUB_FEATURE_PORT_POWER, regs, XHCI_PORT_PP)
	);

	usb_log_debug2("RH: GetPortStatus(%hu) = %u.", setup_packet->index,
		uint32_usb2host(status));

	memcpy(data, &status, sizeof(status));
	*act_size = sizeof(status);

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
	xhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (!setup_packet->index || setup_packet->index > hub->max_ports) {
		return ESTALL;
	}

	/* The index is 1-based. */
	xhci_port_regs_t* regs = &hub->hc->op_regs->portrs[setup_packet->index - 1];

	const usb_hub_class_feature_t feature = uint16_usb2host(setup_packet->value);
	static const ioport32_t masks[] = {
		USB_MAP_XHCI(C_PORT_CONNECTION, CSC),
		USB_MAP_XHCI(C_PORT_ENABLE, PEC),
		USB_MAP_XHCI(C_PORT_OVER_CURRENT, OCC),
		USB_MAP_XHCI(C_PORT_RESET, PRC),
		USB_MAP_XHCI(PORT_ENABLE, PED),
		USB_MAP_XHCI(PORT_RESET, PR),
		USB_MAP_XHCI(PORT_POWER, PP)
	};

	static const bool is_change[] = {
		USB_MAP_VALUE(C_PORT_CONNECTION, true),
		USB_MAP_VALUE(C_PORT_ENABLE, true),
		USB_MAP_VALUE(C_PORT_OVER_CURRENT, true),
		USB_MAP_VALUE(C_PORT_RESET, true),
		USB_MAP_VALUE(PORT_ENABLE, false),
		USB_MAP_VALUE(PORT_RESET, false),
		USB_MAP_VALUE(PORT_POWER, false)
	};

	usb_log_debug2("RH: ClearPortFeature(%hu) = %d.", setup_packet->index,
		feature);

	if (is_change[feature]) {
		/* Clear the register by writing 1. */
		XHCI_REG_SET_FIELD(&regs->portsc, masks[feature], 32);
	} else {
		/* Clear the register by writing 0. */
		XHCI_REG_CLR_FIELD(&regs->portsc, masks[feature], 32);
	}

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
	xhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (!setup_packet->index || setup_packet->index > hub->max_ports) {
		return ESTALL;
	}

	/* The index is 1-based. */
	xhci_port_regs_t* regs = &hub->hc->op_regs->portrs[setup_packet->index - 1];

	const usb_hub_class_feature_t feature = uint16_usb2host(setup_packet->value);
	static const ioport32_t masks[] = {
		USB_MAP_XHCI(PORT_ENABLE, PED),
		USB_MAP_XHCI(PORT_RESET, PR),
		USB_MAP_XHCI(PORT_POWER, PP)
	};

	usb_log_debug2("RH: SetPortFeature(%hu) = %d.", setup_packet->index,
		feature);

	/* Set the feature in the PIO register. */
	XHCI_REG_SET_FIELD(&regs->portsc, masks[feature], 32);

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
 * represent port status change.
 */
static int req_status_change_handler(usbvirt_device_t *device,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	usb_log_debug2("Called req_status_change_handler().");
	xhci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	uint8_t status[STATUS_BYTES(hub->max_ports)];
	memset(status, 0, sizeof(status));

	if (buffer_size < sizeof(status))
		return ESTALL;

	bool change = false;
	for (size_t i = 1; i <= hub->max_ports; ++i) {
		xhci_port_regs_t *regs = &hub->hc->op_regs->portrs[i - 1];

		if (XHCI_REG_RD_FIELD(&regs->portsc, 32) & port_change_mask) {
			status[i / 8] |= (1 << (i % 8));
			change = true;
		}
	}

	memcpy(buffer, &status, sizeof(status));
	*actual_size = sizeof(status);
	return change ? EOK : ENAK;
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
		/* XHCI root hub has no power source,
		 * over-current is reported by port */
		.callback = virthub_base_get_null_status,
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
