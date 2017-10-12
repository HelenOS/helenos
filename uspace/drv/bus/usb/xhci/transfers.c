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
 * @brief The host controller transfer ring management
 */

#include <usb/host/utils/malloc32.h>
#include <usb/debug.h>
#include "endpoint.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "transfers.h"
#include "trb_ring.h"

static inline uint8_t get_transfer_type(xhci_trb_t* trb, uint8_t bmRequestType, uint16_t wLength)
{
	/* See Table 7 of xHCI specification */
	if (bmRequestType & 0x80) {
		/* Device-to-host transfer */
		if (wLength) {
			/* IN data stage */
			return 3;
		}
		else {
			/* No data stage */
			return 0;
		}
	}
	else {
		/* Host-to-device transfer */
		if (wLength) {
			/* OUT data stage */
			return 2;
		}
		else {
			/* No data stage */
			return 0;
		}
	}
}

static inline uint8_t get_data_direction(xhci_trb_t* trb, uint8_t bmRequestType, uint16_t wLength)
{
	/* See Table 7 of xHCI specification */
	if (bmRequestType & 0x80) {
		/* Device-to-host transfer */
		return 1;
	}
	else {
		/* Host-to-device transfer */
		return 0;
	}
}

static inline uint8_t get_status_direction(xhci_trb_t* trb, uint8_t bmRequestType, uint16_t wLength)
{
	/* See Table 7 of xHCI specification */
	if (bmRequestType & 0x80) {
		/* Device-to-host transfer */
		if (wLength) {
			/* Out direction */
			return 0;
		}
		else {
			/* In direction */
			return 1;
		}
	}
	else {
		/* Host-to-device transfer, always IN direction */
		return 1;
	}
}

int xhci_init_transfers(xhci_hc_t *hc)
{
	assert(hc);

	list_initialize(&hc->transfers);
	return EOK;
}

void xhci_fini_transfers(xhci_hc_t *hc)
{
	// Note: Untested.
	assert(hc);
}

xhci_transfer_t* xhci_transfer_alloc(usb_transfer_batch_t* batch) {
	xhci_transfer_t* transfer = malloc(sizeof(xhci_transfer_t));
	if (!transfer)
		return NULL;

	memset(transfer, 0, sizeof(xhci_transfer_t));
	transfer->batch = batch;
	link_initialize(&transfer->link);
	transfer->hc_buffer = malloc32(batch->buffer_size);

	return transfer;
}

void xhci_transfer_fini(xhci_transfer_t* transfer) {
	if (transfer) {
		free32(transfer->hc_buffer);
		free(transfer);
	}
}

int xhci_schedule_control_transfer(xhci_hc_t* hc, usb_transfer_batch_t* batch)
{
	if (!batch->setup_size) {
		usb_log_error("Missing setup packet for the control transfer.");
		return EINVAL;
	}
	if (batch->ep->target.endpoint != 0 || batch->ep->transfer_type != USB_TRANSFER_CONTROL) {
		/* This method only works for control transfers. */
		usb_log_error("Attempted to schedule control transfer to non 0 endpoint.");
		return EINVAL;
	}

	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(batch->ep);

	uint8_t slot_id = xhci_ep->slot_id;
	xhci_trb_ring_t* ring = hc->dcbaa_virt[slot_id].trs[0];

	usb_device_request_setup_packet_t* setup =
		(usb_device_request_setup_packet_t*) batch->setup_buffer;

	/* For the TRB formats, see xHCI specification 6.4.1.2 */
	xhci_transfer_t *transfer = xhci_transfer_alloc(batch);
	memcpy(transfer->hc_buffer, batch->buffer, batch->buffer_size);

	xhci_trb_t trb_setup;
	memset(&trb_setup, 0, sizeof(xhci_trb_t));

	TRB_CTRL_SET_SETUP_WVALUE(trb_setup, setup->value);
	TRB_CTRL_SET_SETUP_WLENGTH(trb_setup, setup->length);
	TRB_CTRL_SET_SETUP_WINDEX(trb_setup, setup->index);
	TRB_CTRL_SET_SETUP_BREQ(trb_setup, setup->request);
	TRB_CTRL_SET_SETUP_BMREQTYPE(trb_setup, setup->request_type);

	/* Size of the setup packet is always 8 */
	TRB_CTRL_SET_XFER_LEN(trb_setup, 8);
	// if we want an interrupt after this td is done, use
	// TRB_CTRL_SET_IOC(trb_setup, 1);

	/* Immediate data */
	TRB_CTRL_SET_IDT(trb_setup, 1);
	TRB_CTRL_SET_TRB_TYPE(trb_setup, XHCI_TRB_TYPE_SETUP_STAGE);
	TRB_CTRL_SET_TRT(trb_setup, get_transfer_type(&trb_setup, setup->request_type, setup->length));

	/* Data stage */
	xhci_trb_t trb_data;
	memset(&trb_data, 0, sizeof(xhci_trb_t));

	if (setup->length > 0) {
		trb_data.parameter = addr_to_phys(transfer->hc_buffer);

		// data size (sent for OUT, or buffer size)
		TRB_CTRL_SET_XFER_LEN(trb_data, batch->buffer_size);
		// FIXME: TD size 4.11.2.4
		TRB_CTRL_SET_TD_SIZE(trb_data, 1);

		// if we want an interrupt after this td is done, use
		// TRB_CTRL_SET_IOC(trb_data, 1);

		// Some more fields here, no idea what they mean
		TRB_CTRL_SET_TRB_TYPE(trb_data, XHCI_TRB_TYPE_DATA_STAGE);

		transfer->direction = get_data_direction(&trb_setup, setup->request_type, setup->length);
		TRB_CTRL_SET_DIR(trb_data, transfer->direction);
	}

	/* Status stage */
	xhci_trb_t trb_status;
	memset(&trb_status, 0, sizeof(xhci_trb_t));

	// FIXME: Evaluate next TRB? 4.12.3
	// TRB_CTRL_SET_ENT(trb_status, 1);

	// if we want an interrupt after this td is done, use
	TRB_CTRL_SET_IOC(trb_status, 1);

	TRB_CTRL_SET_TRB_TYPE(trb_status, XHCI_TRB_TYPE_STATUS_STAGE);
	TRB_CTRL_SET_DIR(trb_status, get_status_direction(&trb_setup, setup->request_type, setup->length));

	uintptr_t dummy = 0;
	xhci_trb_ring_enqueue(ring, &trb_setup, &dummy);
	if (setup->length > 0) {
		xhci_trb_ring_enqueue(ring, &trb_data, &dummy);
	}
	xhci_trb_ring_enqueue(ring, &trb_status, &transfer->interrupt_trb_phys);

	list_append(&transfer->link, &hc->transfers);

	/* For control transfers, the target is always 1. */
	hc_ring_doorbell(hc, slot_id, 1);
	return EOK;
}

int xhci_schedule_bulk_transfer(xhci_hc_t* hc, usb_transfer_batch_t* batch) {
	if (batch->setup_size) {
		usb_log_warning("Setup packet present for a bulk transfer.");
	}

	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(batch->ep);
	uint8_t slot_id = xhci_ep->slot_id;
	xhci_trb_ring_t* ring = hc->dcbaa_virt[slot_id].trs[batch->ep->target.endpoint];

	xhci_transfer_t *transfer = xhci_transfer_alloc(batch);
	memcpy(transfer->hc_buffer, batch->buffer, batch->buffer_size);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(xhci_trb_t));
	trb.parameter = addr_to_phys(transfer->hc_buffer);

	// data size (sent for OUT, or buffer size)
	TRB_CTRL_SET_XFER_LEN(trb, batch->buffer_size);
	// FIXME: TD size 4.11.2.4
	TRB_CTRL_SET_TD_SIZE(trb, 1);

	// we want an interrupt after this td is done
	TRB_CTRL_SET_IOC(trb, 1);

	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

	xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
	list_append(&transfer->link, &hc->transfers);

	// TODO: target = endpoint | stream_id << 16
	hc_ring_doorbell(hc, slot_id, xhci_ep->base->target->endpoint);
	return EOK;
}

int xhci_handle_transfer_event(xhci_hc_t* hc, xhci_trb_t* trb)
{
	uintptr_t addr = trb->parameter;
	xhci_transfer_t *transfer = NULL;

	link_t *transfer_link = list_first(&hc->transfers);
	while (transfer_link) {
		transfer = list_get_instance(transfer_link, xhci_transfer_t, link);

		if (transfer->interrupt_trb_phys == addr)
			break;

		transfer_link = list_next(transfer_link, &hc->transfers);
	}

	if (!transfer_link) {
		usb_log_warning("Transfer not found.");
		return ENOENT;
	}

	list_remove(transfer_link);
	usb_transfer_batch_t *batch = transfer->batch;

	batch->error = (TRB_COMPLETION_CODE(*trb) == XHCI_TRBC_SUCCESS) ? EOK : ENAK;
	batch->transfered_size = batch->buffer_size - TRB_TRANSFER_LENGTH(*trb);
	if (transfer->direction) {
		/* Device-to-host, IN */
		if (batch->callback_in)
			batch->callback_in(batch->error, batch->transfered_size, batch->arg);
	}
	else {
		/* Host-to-device, OUT */
		if (batch->callback_out)
			batch->callback_out(batch->error, batch->arg);
	}

	xhci_transfer_fini(transfer);
	return EOK;
}
