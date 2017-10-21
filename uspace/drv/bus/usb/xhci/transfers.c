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
#include <usb/request.h>
#include "endpoint.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "transfers.h"
#include "trb_ring.h"

typedef enum {
    STAGE_OUT,
    STAGE_IN,
} stage_dir_flag_t;

#define REQUEST_TYPE_DTD (0x80)
#define REQUEST_TYPE_IS_DEVICE_TO_HOST(rq) ((rq) & REQUEST_TYPE_DTD)


/** Get direction flag of data stage.
 *  See Table 7 of xHCI specification.
 */
static inline stage_dir_flag_t get_status_direction_flag(xhci_trb_t* trb,
	uint8_t bmRequestType, uint16_t wLength)
{
	/* See Table 7 of xHCI specification */
	return REQUEST_TYPE_IS_DEVICE_TO_HOST(bmRequestType) && (wLength > 0)
		? STAGE_OUT
		: STAGE_IN;
}

typedef enum {
    DATA_STAGE_NO = 0,
    DATA_STAGE_OUT = 2,
    DATA_STAGE_IN = 3,
} data_stage_type_t;

/** Get transfer type flag.
 *  See Table 8 of xHCI specification.
 */
static inline data_stage_type_t get_transfer_type(xhci_trb_t* trb, uint8_t
	bmRequestType, uint16_t wLength)
{
	if (wLength == 0)
		return DATA_STAGE_NO;

	/* See Table 7 of xHCI specification */
	return REQUEST_TYPE_IS_DEVICE_TO_HOST(bmRequestType)
		? DATA_STAGE_IN
		: DATA_STAGE_NO;
}

static inline bool configure_endpoint_needed(usb_device_request_setup_packet_t *setup)
{
	usb_request_type_t request_type = SETUP_REQUEST_TYPE_GET_TYPE(setup->request_type);

	return request_type == USB_REQUEST_TYPE_STANDARD &&
		(setup->request == USB_DEVREQ_SET_CONFIGURATION
		|| setup->request == USB_DEVREQ_SET_INTERFACE);
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

xhci_transfer_t* xhci_transfer_create(endpoint_t* ep) {
	xhci_transfer_t* transfer = calloc(1, sizeof(xhci_transfer_t));
	if (!transfer)
		return NULL;

	usb_transfer_batch_init(&transfer->batch, ep);

	link_initialize(&transfer->link);

	return transfer;
}

void xhci_transfer_destroy(xhci_transfer_t* transfer)
{
	assert(transfer);

	if (transfer->hc_buffer)
		free32(transfer->hc_buffer);

	free(transfer);
}

static xhci_trb_ring_t *get_ring(xhci_hc_t *hc, xhci_transfer_t *transfer)
{
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);
	uint8_t slot_id = xhci_ep->device->slot_id;

	xhci_trb_ring_t* ring = hc->dcbaa_virt[slot_id].trs[transfer->batch.ep->target.endpoint];
	assert(ring);
	return ring;
}

static int schedule_control(xhci_hc_t* hc, xhci_transfer_t* transfer)
{
	usb_transfer_batch_t *batch = &transfer->batch;
	xhci_trb_ring_t *ring = get_ring(hc, transfer);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);

	usb_device_request_setup_packet_t* setup = &batch->setup.packet;

	xhci_trb_t trbs[3];
	int trbs_used = 0;

	xhci_trb_t *trb_setup = trbs + trbs_used++;
	xhci_trb_clean(trb_setup);

	TRB_CTRL_SET_SETUP_WVALUE(*trb_setup, setup->value);
	TRB_CTRL_SET_SETUP_WLENGTH(*trb_setup, setup->length);
	TRB_CTRL_SET_SETUP_WINDEX(*trb_setup, setup->index);
	TRB_CTRL_SET_SETUP_BREQ(*trb_setup, setup->request);
	TRB_CTRL_SET_SETUP_BMREQTYPE(*trb_setup, setup->request_type);

	/* Size of the setup packet is always 8 */
	TRB_CTRL_SET_XFER_LEN(*trb_setup, 8);

	/* Immediate data */
	TRB_CTRL_SET_IDT(*trb_setup, 1);
	TRB_CTRL_SET_TRB_TYPE(*trb_setup, XHCI_TRB_TYPE_SETUP_STAGE);
	TRB_CTRL_SET_TRT(*trb_setup, get_transfer_type(trb_setup, setup->request_type, setup->length));

	/* Data stage */
	xhci_trb_t *trb_data = NULL;
	if (setup->length > 0) {
		trb_data = trbs + trbs_used++;
		xhci_trb_clean(trb_data);

		trb_data->parameter = addr_to_phys(transfer->hc_buffer);

		// data size (sent for OUT, or buffer size)
		TRB_CTRL_SET_XFER_LEN(*trb_data, batch->buffer_size);
		// FIXME: TD size 4.11.2.4
		TRB_CTRL_SET_TD_SIZE(*trb_data, 1);

		// Some more fields here, no idea what they mean
		TRB_CTRL_SET_TRB_TYPE(*trb_data, XHCI_TRB_TYPE_DATA_STAGE);

		int stage_dir = REQUEST_TYPE_IS_DEVICE_TO_HOST(setup->request_type)
					? STAGE_IN : STAGE_OUT;
		TRB_CTRL_SET_DIR(*trb_data, stage_dir);
	}

	/* Status stage */
	xhci_trb_t *trb_status = trbs + trbs_used++;
	xhci_trb_clean(trb_status);

	// FIXME: Evaluate next TRB? 4.12.3
	// TRB_CTRL_SET_ENT(*trb_status, 1);

	TRB_CTRL_SET_IOC(*trb_status, 1);
	TRB_CTRL_SET_TRB_TYPE(*trb_status, XHCI_TRB_TYPE_STATUS_STAGE);
	TRB_CTRL_SET_DIR(*trb_status, get_status_direction_flag(trb_setup, setup->request_type, setup->length));

	// Issue a Configure Endpoint command, if needed.
	if (configure_endpoint_needed(setup)) {
		const int err = xhci_device_configure(xhci_ep->device, hc);
		if (err)
			return err;
	}

	return xhci_trb_ring_enqueue_multiple(ring, trbs, trbs_used, &transfer->interrupt_trb_phys);
}

static int schedule_bulk(xhci_hc_t* hc, xhci_transfer_t *transfer)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);
	trb.parameter = addr_to_phys(transfer->hc_buffer);

	// data size (sent for OUT, or buffer size)
	TRB_CTRL_SET_XFER_LEN(trb, transfer->batch.buffer_size);
	// FIXME: TD size 4.11.2.4
	TRB_CTRL_SET_TD_SIZE(trb, 1);

	// we want an interrupt after this td is done
	TRB_CTRL_SET_IOC(trb, 1);

	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);
	uint8_t slot_id = xhci_ep->device->slot_id;
	xhci_trb_ring_t* ring = hc->dcbaa_virt[slot_id].trs[transfer->batch.ep->target.endpoint];

	return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
}

static int schedule_interrupt(xhci_hc_t* hc, xhci_transfer_t* transfer)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);
	trb.parameter = addr_to_phys(transfer->hc_buffer);

	// data size (sent for OUT, or buffer size)
	TRB_CTRL_SET_XFER_LEN(trb, transfer->batch.buffer_size);
	// FIXME: TD size 4.11.2.4
	TRB_CTRL_SET_TD_SIZE(trb, 1);

	// we want an interrupt after this td is done
	TRB_CTRL_SET_IOC(trb, 1);

	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);
	uint8_t slot_id = xhci_ep->device->slot_id;
	xhci_trb_ring_t* ring = hc->dcbaa_virt[slot_id].trs[transfer->batch.ep->target.endpoint];

	return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
}

static int schedule_isochronous(xhci_hc_t* hc, xhci_transfer_t* transfer)
{
	/* TODO: Implement me. */
	usb_log_error("Isochronous transfers are not yet implemented!");
	return ENOTSUP;
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
	usb_transfer_batch_t *batch = &transfer->batch;

	batch->error = (TRB_COMPLETION_CODE(*trb) == XHCI_TRBC_SUCCESS) ? EOK : ENAK;
	batch->transfered_size = batch->buffer_size - TRB_TRANSFER_LENGTH(*trb);

	if (batch->dir == USB_DIRECTION_IN) {
		assert(batch->buffer);
		assert(batch->transfered_size <= batch->buffer_size);
		memcpy(batch->buffer, transfer->hc_buffer, batch->transfered_size);
	}

	usb_transfer_batch_finish(batch);
	return EOK;
}

typedef int (*transfer_handler)(xhci_hc_t *, xhci_transfer_t *);

static const transfer_handler transfer_handlers[] = {
	[USB_TRANSFER_CONTROL] = schedule_control,
	[USB_TRANSFER_ISOCHRONOUS] = schedule_isochronous,
	[USB_TRANSFER_BULK] = schedule_bulk,
	[USB_TRANSFER_INTERRUPT] = schedule_interrupt,
};

int xhci_transfer_schedule(xhci_hc_t *hc, usb_transfer_batch_t *batch)
{
	assert(hc);

	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(batch->ep);
	uint8_t slot_id = xhci_ep->device->slot_id;

	assert(xhci_ep);
	assert(slot_id);

	const usb_transfer_type_t type = batch->ep->transfer_type;
	assert(type >= 0 && type < ARRAY_SIZE(transfer_handlers));
	assert(transfer_handlers[type]);

	if (batch->buffer_size > 0) {
		transfer->hc_buffer = malloc32(batch->buffer_size);
	}

	if (batch->dir != USB_DIRECTION_IN) {
		// Sending stuff from host to device, we need to copy the actual data.
		memcpy(transfer->hc_buffer, batch->buffer, batch->buffer_size);
	}

	const int err = transfer_handlers[batch->ep->transfer_type](hc, transfer);
	if (err)
		return err;

	list_append(&transfer->link, &hc->transfers);

	const uint8_t target = xhci_endpoint_index(xhci_ep) + 1; /* EP Doorbells start at 1 */
	return hc_ring_doorbell(hc, slot_id, target);
}
