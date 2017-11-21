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
 * There can currently be only one active transfer, because
 * usb_transfer_batch_init locks the endpoint by endpoint_use.
 * Therefore, we store the only active transfer per endpoint there.
 */
xhci_transfer_t* xhci_transfer_create(endpoint_t* ep)
{
	xhci_transfer_t *transfer = calloc(1, sizeof(xhci_transfer_t));
	if (!transfer)
		return NULL;

	usb_transfer_batch_init(&transfer->batch, ep);
	return transfer;
}

void xhci_transfer_destroy(xhci_transfer_t* transfer)
{
	assert(transfer);

	dma_buffer_free(&transfer->hc_buffer);
}

static xhci_trb_ring_t *get_ring(xhci_hc_t *hc, xhci_transfer_t *transfer)
{
	return &xhci_endpoint_get(transfer->batch.ep)->ring;
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
	// FIXME: TD size 4.11.2.4
	TRB_CTRL_SET_TD_SIZE(trb, 1);

	// we want an interrupt after this td is done
	TRB_CTRL_SET_IOC(trb, 1);

	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_NORMAL);

	xhci_trb_ring_t* ring = get_ring(hc, transfer);

	return xhci_trb_ring_enqueue(ring, &trb, &transfer->interrupt_trb_phys);
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

static xhci_isoch_transfer_t* isoch_transfer_get_enqueue(xhci_endpoint_t *ep) {
	if ((ep->isoch_enqueue % XHCI_ISOCH_BUFFER_COUNT) == ep->isoch_dequeue) {
		/* None ready */
		return NULL;
	}
	xhci_isoch_transfer_t *isoch_transfer = ep->isoch_transfers[ep->isoch_enqueue];
	ep->isoch_enqueue = (ep->isoch_enqueue + 1) % XHCI_ISOCH_BUFFER_COUNT;
	return isoch_transfer;
}

static xhci_isoch_transfer_t* isoch_transfer_get_dequeue(xhci_endpoint_t *ep) {
	xhci_isoch_transfer_t *isoch_transfer = ep->isoch_transfers[ep->isoch_dequeue];
	ep->isoch_dequeue = (ep->isoch_dequeue + 1) % XHCI_ISOCH_BUFFER_COUNT;
	return isoch_transfer;
}

static int schedule_isochronous_trb(xhci_trb_ring_t *ring, xhci_endpoint_t *ep, xhci_trb_t *trb,
	const size_t len, uintptr_t *trb_phys)
{
	TRB_CTRL_SET_XFER_LEN(*trb, len);
	// FIXME: TD size 4.11.2.4 (there is no next TRB, so 0?)
	TRB_CTRL_SET_TD_SIZE(*trb, 0);
	TRB_CTRL_SET_IOC(*trb, 1);
	TRB_CTRL_SET_TRB_TYPE(*trb, XHCI_TRB_TYPE_ISOCH);

	// see 4.14.1 and 4.11.2.3 for the explanation, how to calculate those
	size_t tdpc = len / 1024 + ((len % 1024) ? 1 : 0);
	size_t tbc = tdpc / (ep->max_burst + 1);
	if(!tdpc % (ep->max_burst + 1)) --tbc;
	size_t bsp = tdpc % (ep->max_burst + 1);
	size_t tlbpc = (bsp ? bsp - 1 : ep->max_burst);

	TRB_CTRL_SET_TBC(*trb, tbc);
	TRB_CTRL_SET_TLBPC(*trb, tlbpc);

	// FIXME: do we want this? 6.4.1.3, p 366 (also possibly frame id?)
	TRB_CTRL_SET_SIA(*trb, 1);

	return xhci_trb_ring_enqueue(ring, trb, trb_phys);
}

static int schedule_isochronous_out(xhci_hc_t* hc, xhci_transfer_t* transfer, xhci_endpoint_t *xhci_ep,
	xhci_device_t *xhci_dev)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);

	// FIXME add some locks
	xhci_isoch_transfer_t *isoch_transfer = isoch_transfer_get_enqueue(xhci_ep);
	while (!isoch_transfer) {
		// FIXME: add condvar sleep;
	}

	isoch_transfer->size = transfer->batch.buffer_size;
	if (isoch_transfer->size > 0) {
		memcpy(isoch_transfer->data.virt, transfer->batch.buffer, isoch_transfer->size);
	}

	trb.parameter = isoch_transfer->data.phys;

	xhci_trb_ring_t *ring = get_ring(hc, transfer);
	int err = schedule_isochronous_trb(ring, xhci_ep, &trb, isoch_transfer->size,
		&isoch_transfer->interrupt_trb_phys);
	if (err) {
		return err;
	}

	/* If not yet started, start the isochronous endpoint transfers - after buffer count - 1 writes */
	/* The -2 is there because of the enqueue != dequeue check. The buffer must have at least 2 transfers. */
	if (xhci_ep->isoch_enqueue == XHCI_ISOCH_BUFFER_COUNT - 2 && !xhci_ep->isoch_started) {
		const uint8_t slot_id = xhci_dev->slot_id;
		const uint8_t target = xhci_endpoint_index(xhci_ep) + 1; /* EP Doorbells start at 1 */
		err = hc_ring_doorbell(hc, slot_id, target);
		if (err) {
			return err;
		}
		xhci_ep->isoch_started = true;
	}

	/* Isochronous transfers don't handle errors, they skip them all. */
	transfer->batch.error = EOK;
	transfer->batch.transfered_size = transfer->batch.buffer_size;
	usb_transfer_batch_finish(&transfer->batch);
	return EOK;
}

static int schedule_isochronous_in(xhci_hc_t* hc, xhci_transfer_t* transfer, xhci_endpoint_t *xhci_ep,
	xhci_device_t *xhci_dev)
{
	/* If not yet started, start the isochronous endpoint transfers - before first read */
	if (!xhci_ep->isoch_started) {
		const uint8_t slot_id = xhci_dev->slot_id;
		const uint8_t target = xhci_endpoint_index(xhci_ep) + 1; /* EP Doorbells start at 1 */
		int err = hc_ring_doorbell(hc, slot_id, target);
		if (err) {
			return err;
		}
		xhci_ep->isoch_started = true;
	}

	// FIXME add some locks
	xhci_isoch_transfer_t *isoch_transfer = isoch_transfer_get_enqueue(xhci_ep);
	while(!isoch_transfer) {
		// FIXME: add condvar sleep;
	}

	isoch_transfer->size = transfer->batch.buffer_size;
	if (transfer->batch.buffer_size <= isoch_transfer->size) {
		if (transfer->batch.buffer_size > 0) {
			memcpy(transfer->batch.buffer, isoch_transfer->data.virt, transfer->batch.buffer_size);
		}
		if (transfer->batch.buffer_size < isoch_transfer->size) {
			// FIXME: somehow notify that buffer was too small, probably batch error code
		}
		transfer->batch.transfered_size = transfer->batch.buffer_size;
	}
	else {
		memcpy(transfer->batch.buffer, isoch_transfer->data.virt, isoch_transfer->size);
		transfer->batch.transfered_size = isoch_transfer->size;
	}

	// Clear and requeue the transfer with new TRB
	xhci_trb_t trb;
	xhci_trb_clean(&trb);

	trb.parameter = isoch_transfer->data.phys;
	isoch_transfer->size = xhci_ep->isoch_max_size;

	xhci_trb_ring_t *ring = get_ring(hc, transfer);
	int err = schedule_isochronous_trb(ring, xhci_ep, &trb, isoch_transfer->size,
		&isoch_transfer->interrupt_trb_phys);
	if (err) {
		return err;
	}

	/* Isochronous transfers don't handle errors, they skip them all. */
	transfer->batch.error = EOK;
	usb_transfer_batch_finish(&transfer->batch);
	return EOK;
}

static int schedule_isochronous(xhci_hc_t* hc, xhci_transfer_t* transfer, xhci_endpoint_t *xhci_ep,
	xhci_device_t *xhci_dev)
{
	if (transfer->batch.buffer_size > xhci_ep->isoch_max_size) {
		usb_log_error("Cannot schedule an oversized isochronous transfer.");
		return EINVAL;
	}

	if (xhci_ep->base.direction == USB_DIRECTION_OUT) {
		return schedule_isochronous_out(hc, transfer, xhci_ep, xhci_dev);
	}
	else {
		return schedule_isochronous_in(hc, transfer, xhci_ep, xhci_dev);
	}
}

static int handle_isochronous_transfer_event(xhci_hc_t *hc, xhci_trb_t *trb, xhci_endpoint_t *ep) {
	fibril_mutex_lock(&ep->base.guard);

	xhci_isoch_transfer_t *isoch_transfer = isoch_transfer_get_dequeue(ep);
	if (isoch_transfer->interrupt_trb_phys != trb->parameter) {
		usb_log_error("Non-matching trb to isochronous transfer, skipping.");
		// FIXME: what to do? probably just kill the whole endpoint
		return ENOENT;
	}

	if (ep->base.direction == USB_DIRECTION_IN) {
		// We may have received less data, that's fine
		isoch_transfer->size -= TRB_TRANSFER_LENGTH(*trb);
	}

	// TODO: Send notify to waiting fibrils in case they are waiting here

	fibril_mutex_unlock(&ep->base.guard);
	return EOK;
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
	xhci_endpoint_t *ep = xhci_device_get_endpoint(dev, ep_num);
	if (!ep) {
		usb_log_error("Transfer event on dropped endpoint %u of device "
		    XHCI_DEV_FMT, ep_num, XHCI_DEV_ARGS(*dev));
		return ENOENT;
	}

	/* FIXME: This is racy. Do we care? */
	ep->ring.dequeue = addr;

	if (ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		return handle_isochronous_transfer_event(hc, trb, ep);
	}

	fibril_mutex_lock(&ep->base.guard);
	usb_transfer_batch_t *batch = ep->base.active_batch;
	if (!batch) {
		fibril_mutex_unlock(&ep->base.guard);
		return ENOENT;
	}

	const xhci_trb_completion_code_t completion_code = TRB_COMPLETION_CODE(*trb);
	switch (completion_code) {
		case XHCI_TRBC_SHORT_PACKET:
			usb_log_debug("Short transfer.");
			/* fallthrough */
		case XHCI_TRBC_SUCCESS:
			batch->error = EOK;
			batch->transfered_size = batch->buffer_size - TRB_TRANSFER_LENGTH(*trb);
			break;

		default:
			usb_log_warning("Transfer not successfull: %u", completion_code);
			batch->error = EIO;
	}

	usb_transfer_batch_reset_toggle(batch);
	endpoint_deactivate_locked(&ep->base);
	fibril_mutex_unlock(&ep->base.guard);

	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);

	if (batch->dir == USB_DIRECTION_IN) {
		assert(batch->buffer);
		assert(batch->transfered_size <= batch->buffer_size);
		memcpy(batch->buffer, transfer->hc_buffer.virt, batch->transfered_size);
	}

	usb_transfer_batch_finish(batch);
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

	/* Offline devices don't schedule transfers other than on EP0. */
	if (!xhci_dev->online && ep->endpoint > 0) {
		return EAGAIN;
	}

	// FIXME: find a better way to check if the ring is not initialized
	if (!xhci_ep->ring.segment_count) {
		usb_log_error("Ring not initialized for endpoint " XHCI_EP_FMT,
		    XHCI_EP_ARGS(*xhci_ep));
		return EINVAL;
	}

	// Isochronous transfer needs to be handled differently
	if(batch->ep->transfer_type == USB_TRANSFER_ISOCHRONOUS) {
		return schedule_isochronous(hc, transfer, xhci_ep, xhci_dev);
	}

	const usb_transfer_type_t type = batch->ep->transfer_type;
	assert(type >= 0 && type < ARRAY_SIZE(transfer_handlers));
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
	const uint8_t target = xhci_endpoint_index(xhci_ep) + 1; /* EP Doorbells start at 1 */
	return hc_ring_doorbell(hc, slot_id, target);
}
