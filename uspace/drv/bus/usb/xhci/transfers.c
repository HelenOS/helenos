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

#include <usb/debug.h>
#include <usb/request.h>
#include "endpoint.h"
#include "hc.h"
#include "hw_struct/trb.h"
#include "streams.h"
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

/**
 * Create a xHCI-specific transfer batch.
 *
 * Bus callback.
 */
usb_transfer_batch_t * xhci_transfer_create(endpoint_t* ep)
{
	xhci_transfer_t *transfer = calloc(1, sizeof(xhci_transfer_t));
	if (!transfer)
		return NULL;

	usb_transfer_batch_init(&transfer->batch, ep);
	return &transfer->batch;
}

/**
 * Destroy a xHCI transfer.
 */
void xhci_transfer_destroy(usb_transfer_batch_t* batch)
{
	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);

	dma_buffer_free(&transfer->hc_buffer);
	free(transfer);
}

static xhci_trb_ring_t *get_ring(xhci_hc_t *hc, xhci_transfer_t *transfer)
{
	xhci_endpoint_t *ep = xhci_endpoint_get(transfer->batch.ep);
	if (ep->primary_stream_data_size == 0) return &ep->ring;
	uint32_t stream_id = transfer->batch.target.stream;

	xhci_stream_data_t *stream_data = xhci_get_stream_ctx_data(ep, stream_id);
	if (stream_data == NULL) {
		usb_log_warning("No transfer ring was found for stream %u.", stream_id);
		return NULL;
	}

	return &stream_data->ring;
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

		trb_data->parameter = host2xhci(64, transfer->hc_buffer.phys);

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
		const int err = hc_configure_device(hc, xhci_ep_to_dev(xhci_ep)->slot_id);
		if (err)
			return err;
	}

	return xhci_trb_ring_enqueue_multiple(ring, trbs, trbs_used, &transfer->interrupt_trb_phys);
}

static int schedule_bulk(xhci_hc_t* hc, xhci_transfer_t *transfer)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);
	trb.parameter = host2xhci(64, transfer->hc_buffer.phys);

	// data size (sent for OUT, or buffer size)
	TRB_CTRL_SET_XFER_LEN(trb, transfer->batch.buffer_size);

	/* The stream-enabled endpoints need to chain ED trb */
	xhci_endpoint_t *ep = xhci_endpoint_get(transfer->batch.ep);
	if (!ep->primary_stream_data_size) {
		// FIXME: TD size 4.11.2.4
		TRB_CTRL_SET_TD_SIZE(trb, 1);

		// we want an interrupt after this td is done
		TRB_CTRL_SET_IOC(trb, 1);
		TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

		xhci_trb_ring_t* ring = get_ring(hc, transfer);
		return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
	}
	else {
		TRB_CTRL_SET_TD_SIZE(trb, 2);
		TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);
		TRB_CTRL_SET_CHAIN(trb, 1);
		TRB_CTRL_SET_ENT(trb, 1);

		xhci_trb_ring_t* ring = get_ring(hc, transfer);
		if (!ring) {
			return EINVAL;
		}

		int err = xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);

		if (err) {
			return err;
		}

		xhci_trb_clean(&trb);
		trb.parameter = host2xhci(64, (uintptr_t) transfer);
		TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_EVENT_DATA);
		TRB_CTRL_SET_IOC(trb, 1);

		return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
	}
}

static int schedule_interrupt(xhci_hc_t* hc, xhci_transfer_t* transfer)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);
	trb.parameter = host2xhci(64, transfer->hc_buffer.phys);

	// data size (sent for OUT, or buffer size)
	TRB_CTRL_SET_XFER_LEN(trb, transfer->batch.buffer_size);
	// FIXME: TD size 4.11.2.4
	TRB_CTRL_SET_TD_SIZE(trb, 1);

	// we want an interrupt after this td is done
	TRB_CTRL_SET_IOC(trb, 1);

	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

	xhci_trb_ring_t* ring = get_ring(hc, transfer);

	return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
}

static int schedule_isochronous(xhci_transfer_t* transfer)
{
	endpoint_t *ep = transfer->batch.ep;

	return ep->direction == USB_DIRECTION_OUT
		? isoch_schedule_out(transfer)
		: isoch_schedule_in(transfer);
}

int xhci_handle_transfer_event(xhci_hc_t* hc, xhci_trb_t* trb)
{
	uintptr_t addr = trb->parameter;
	const unsigned slot_id = XHCI_DWORD_EXTRACT(trb->control, 31, 24);
	const unsigned ep_dci = XHCI_DWORD_EXTRACT(trb->control, 20, 16);

	xhci_device_t *dev = hc->bus.devices_by_slot[slot_id];
	if (!dev) {
		usb_log_error("Transfer event on disabled slot %u", slot_id);
		return ENOENT;
	}

	const usb_endpoint_t ep_num = ep_dci / 2;
	const usb_endpoint_t dir = ep_dci % 2 ? USB_DIRECTION_IN : USB_DIRECTION_OUT;
	/* Creating temporary reference */
	endpoint_t *ep_base = bus_find_endpoint(&dev->base, ep_num, dir);
	if (!ep_base) {
		usb_log_error("Transfer event on dropped endpoint %u %s of device "
		    XHCI_DEV_FMT, ep_num, usb_str_direction(dir), XHCI_DEV_ARGS(*dev));
		return ENOENT;
	}
	xhci_endpoint_t *ep = xhci_endpoint_get(ep_base);

	usb_transfer_batch_t *batch;
	xhci_transfer_t *transfer;

	if (TRB_EVENT_DATA(*trb)) {
		assert(ep->base.transfer_type != USB_TRANSFER_ISOCHRONOUS);
		/* We are received transfer pointer instead - work with that */
		transfer = (xhci_transfer_t *) addr;
		xhci_trb_ring_t * ring = get_ring(hc, transfer);
		xhci_trb_ring_update_dequeue(ring, transfer->interrupt_trb_phys);
		batch = &transfer->batch;

		fibril_mutex_lock(&ep->base.guard);
		endpoint_deactivate_locked(&ep->base);
		fibril_mutex_unlock(&ep->base.guard);
	}
	else {
		xhci_trb_ring_update_dequeue(&ep->ring, addr);

		if (ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
			isoch_handle_transfer_event(hc, ep, trb);
			/* Dropping temporary reference */
			endpoint_del_ref(&ep->base);
			return EOK;
		}

		fibril_mutex_lock(&ep->base.guard);
		batch = ep->base.active_batch;
		if (!batch) {
			fibril_mutex_unlock(&ep->base.guard);
			/* Dropping temporary reference */
			endpoint_del_ref(&ep->base);
			return ENOENT;
		}

		transfer = xhci_transfer_from_batch(batch);

		endpoint_deactivate_locked(&ep->base);
		fibril_mutex_unlock(&ep->base.guard);
	}

	const xhci_trb_completion_code_t completion_code = TRB_COMPLETION_CODE(*trb);
	switch (completion_code) {
		case XHCI_TRBC_SHORT_PACKET:
		case XHCI_TRBC_SUCCESS:
			batch->error = EOK;
			batch->transfered_size = batch->buffer_size - TRB_TRANSFER_LENGTH(*trb);
			break;

		case XHCI_TRBC_DATA_BUFFER_ERROR:
			usb_log_warning("Transfer ended with data buffer error.");
			batch->error = EAGAIN;
			batch->transfered_size = 0;
			break;

		case XHCI_TRBC_BABBLE_DETECTED_ERROR:
			usb_log_warning("Babble detected during the transfer.");
			batch->error = EAGAIN;
			batch->transfered_size = 0;
			break;

		case XHCI_TRBC_USB_TRANSACTION_ERROR:
			usb_log_warning("USB Transaction error.");
			batch->error = ESTALL;
			batch->transfered_size = 0;
			break;

		case XHCI_TRBC_TRB_ERROR:
			usb_log_error("Invalid transfer parameters.");
			batch->error = EINVAL;
			batch->transfered_size = 0;
			break;

		case XHCI_TRBC_STALL_ERROR:
			usb_log_warning("Stall condition detected.");
			batch->error = ESTALL;
			batch->transfered_size = 0;
			break;

		case XHCI_TRBC_SPLIT_TRANSACTION_ERROR:
			usb_log_error("Split transcation error detected.");
			batch->error = EAGAIN;
			batch->transfered_size = 0;
			break;

		default:
			usb_log_warning("Transfer not successfull: %u", completion_code);
			batch->error = EIO;
	}

	
	if (xhci_endpoint_get_state(ep) == EP_STATE_HALTED) {
		usb_log_debug("Endpoint halted, resetting endpoint.");
		const int err = xhci_endpoint_clear_halt(ep, batch->target.stream);
		if (err)
			usb_log_error("Failed to clear halted condition on "
			    "endpoint " XHCI_EP_FMT ". Unexpected results "
			    "coming.", XHCI_EP_ARGS(*ep));
	}

	if (batch->dir == USB_DIRECTION_IN) {
		assert(batch->buffer);
		assert(batch->transfered_size <= batch->buffer_size);
		memcpy(batch->buffer, transfer->hc_buffer.virt, batch->transfered_size);
	}

	usb_transfer_batch_finish(batch);
	/* Dropping temporary reference */
	endpoint_del_ref(&ep->base);
	return EOK;
}

typedef int (*transfer_handler)(xhci_hc_t *, xhci_transfer_t *);

static const transfer_handler transfer_handlers[] = {
	[USB_TRANSFER_CONTROL] = schedule_control,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
	[USB_TRANSFER_BULK] = schedule_bulk,
	[USB_TRANSFER_INTERRUPT] = schedule_interrupt,
};

int xhci_transfer_schedule(xhci_hc_t *hc, usb_transfer_batch_t *batch)
{
	assert(hc);
	endpoint_t *ep = batch->ep;

	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);
	xhci_device_t *xhci_dev = xhci_ep_to_dev(xhci_ep);

	// FIXME: find a better way to check if the ring is not initialized
	if (!xhci_ep->ring.segment_count) {
		usb_log_error("Ring not initialized for endpoint " XHCI_EP_FMT,
		    XHCI_EP_ARGS(*xhci_ep));
		return EINVAL;
	}

	// Isochronous transfer needs to be handled differently
	if (batch->ep->transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		return schedule_isochronous(transfer);
	}

	const usb_transfer_type_t type = batch->ep->transfer_type;
	assert(transfer_handlers[type]);

	if (batch->buffer_size > 0) {
		if (dma_buffer_alloc(&transfer->hc_buffer, batch->buffer_size))
			return ENOMEM;
	}

	if (batch->dir != USB_DIRECTION_IN) {
		// Sending stuff from host to device, we need to copy the actual data.
		memcpy(transfer->hc_buffer.virt, batch->buffer, batch->buffer_size);
	}

	fibril_mutex_lock(&ep->guard);
	endpoint_activate_locked(ep, batch);
	const int err = transfer_handlers[batch->ep->transfer_type](hc, transfer);

	if (err) {
		endpoint_deactivate_locked(ep);
		fibril_mutex_unlock(&ep->guard);
		return err;
	}

	/* After the critical section, the transfer can already be finished or aborted. */
	transfer = NULL; batch = NULL;
	fibril_mutex_unlock(&ep->guard);

	const uint8_t slot_id = xhci_dev->slot_id;
	/* EP Doorbells start at 1 */
	const uint8_t target = (xhci_endpoint_index(xhci_ep) + 1) | (batch->target.stream << 16);
	hc_ring_doorbell(hc, slot_id, target);
	return EOK;
}
