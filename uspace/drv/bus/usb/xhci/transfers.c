/*
 * Copyright (c) 2018 Michal Staruch, Ondrej Hlavaty, Petr Manek, Jan Hrach
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

/** Get direction flag of data stage.
 *  See Table 7 of xHCI specification.
 */
static inline stage_dir_flag_t get_status_direction_flag(xhci_trb_t *trb,
    uint8_t bmRequestType, uint16_t wLength)
{
	/* See Table 7 of xHCI specification */
	return SETUP_REQUEST_TYPE_IS_DEVICE_TO_HOST(bmRequestType) && (wLength > 0) ?
	    STAGE_OUT :
	    STAGE_IN;
}

typedef enum {
	DATA_STAGE_NO = 0,
	DATA_STAGE_OUT = 2,
	DATA_STAGE_IN = 3,
} data_stage_type_t;

/** Get transfer type flag.
 *  See Table 8 of xHCI specification.
 */
static inline data_stage_type_t get_transfer_type(xhci_trb_t *trb, uint8_t
    bmRequestType, uint16_t wLength)
{
	if (wLength == 0)
		return DATA_STAGE_NO;

	/* See Table 7 of xHCI specification */
	return SETUP_REQUEST_TYPE_IS_DEVICE_TO_HOST(bmRequestType) ?
	    DATA_STAGE_IN :
	    DATA_STAGE_NO;
}

static inline bool configure_endpoint_needed(usb_device_request_setup_packet_t *setup)
{
	usb_request_type_t request_type = SETUP_REQUEST_TYPE_GET_TYPE(setup->request_type);

	return request_type == USB_REQUEST_TYPE_STANDARD &&
	    (setup->request == USB_DEVREQ_SET_CONFIGURATION ||
	    setup->request == USB_DEVREQ_SET_INTERFACE);
}

/**
 * Create a xHCI-specific transfer batch.
 *
 * Bus callback.
 */
usb_transfer_batch_t *xhci_transfer_create(endpoint_t *ep)
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
void xhci_transfer_destroy(usb_transfer_batch_t *batch)
{
	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);
	free(transfer);
}

static xhci_trb_ring_t *get_ring(xhci_transfer_t *transfer)
{
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);
	return xhci_endpoint_get_ring(xhci_ep, transfer->batch.target.stream);
}

#define MAX_CHUNK_SIZE (1 << 16)

typedef struct {
	/* Input parameters */
	dma_buffer_t buf;
	size_t chunk_size, packet_count, mps, max_trb_count;

	/* Changing at runtime */
	size_t transferred, remaining;
	void *pos;
} trb_splitter_t;

static void trb_splitter_init(trb_splitter_t *ts, xhci_transfer_t *transfer)
{
	ts->buf = transfer->batch.dma_buffer;

	const size_t chunk_mask = dma_policy_chunk_mask(ts->buf.policy);
	ts->chunk_size = (chunk_mask > MAX_CHUNK_SIZE + 1) ?
	    MAX_CHUNK_SIZE : (chunk_mask + 1);

	ts->remaining = transfer->batch.size;
	ts->max_trb_count = (ts->remaining + ts->chunk_size - 1) / ts->chunk_size + 1;
	ts->mps = transfer->batch.ep->max_packet_size;
	ts->packet_count = (ts->remaining + ts->mps - 1) / ts->mps;

	ts->transferred = 0;
	ts->pos = ts->buf.virt + transfer->batch.offset;
}

static void trb_split_next(xhci_trb_t *trb, trb_splitter_t *ts)
{
	xhci_trb_clean(trb);

	size_t size = min(ts->remaining, ts->chunk_size);

	/* First TRB might be misaligned */
	if (ts->transferred == 0) {
		const size_t offset = (ts->pos - ts->buf.virt) % ts->chunk_size;
		size = min(size, ts->chunk_size - offset);
	}

	ts->transferred += size;
	ts->remaining -= size;

	const size_t tx_packets = (ts->transferred + ts->mps - 1) / ts->mps;
	const unsigned td_size = min(31, ts->packet_count - tx_packets);

	/* Last TRB must have TD Size = 0 */
	assert(ts->remaining > 0 || td_size == 0);

	uintptr_t phys = dma_buffer_phys(&ts->buf, ts->pos);

	trb->parameter = host2xhci(64, phys);
	TRB_CTRL_SET_TD_SIZE(*trb, td_size);
	TRB_CTRL_SET_XFER_LEN(*trb, size);
	TRB_CTRL_SET_TRB_TYPE(*trb, XHCI_TRB_TYPE_NORMAL);

	if (ts->remaining)
		TRB_CTRL_SET_CHAIN(*trb, 1);

	ts->pos += size;
}

static errno_t schedule_control(xhci_hc_t *hc, xhci_transfer_t *transfer)
{
	usb_transfer_batch_t *batch = &transfer->batch;
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(transfer->batch.ep);

	usb_device_request_setup_packet_t *setup = &batch->setup.packet;

	trb_splitter_t splitter;
	trb_splitter_init(&splitter, transfer);

	xhci_trb_t trbs[splitter.max_trb_count + 2];
	size_t trbs_used = 0;

	xhci_trb_t *trb_setup = &trbs[trbs_used++];
	xhci_trb_clean(trb_setup);

	trb_setup->parameter = batch->setup.packed;

	/* Size of the setup packet is always 8 */
	TRB_CTRL_SET_XFER_LEN(*trb_setup, 8);

	/* Immediate data */
	TRB_CTRL_SET_IDT(*trb_setup, 1);
	TRB_CTRL_SET_TRB_TYPE(*trb_setup, XHCI_TRB_TYPE_SETUP_STAGE);
	TRB_CTRL_SET_TRT(*trb_setup,
	    get_transfer_type(trb_setup, setup->request_type, setup->length));

	stage_dir_flag_t stage_dir = (transfer->batch.dir == USB_DIRECTION_IN) ?
	    STAGE_IN : STAGE_OUT;

	/* Data stage - first TRB is special */
	if (splitter.remaining > 0) {
		xhci_trb_t *trb = &trbs[trbs_used++];
		trb_split_next(trb, &splitter);
		TRB_CTRL_SET_TRB_TYPE(*trb, XHCI_TRB_TYPE_DATA_STAGE);
		TRB_CTRL_SET_DIR(*trb, stage_dir);
	}
	while (splitter.remaining > 0)
		trb_split_next(&trbs[trbs_used++], &splitter);

	/* Status stage */
	xhci_trb_t *trb_status = &trbs[trbs_used++];
	xhci_trb_clean(trb_status);

	TRB_CTRL_SET_IOC(*trb_status, 1);
	TRB_CTRL_SET_TRB_TYPE(*trb_status, XHCI_TRB_TYPE_STATUS_STAGE);
	TRB_CTRL_SET_DIR(*trb_status, get_status_direction_flag(trb_setup,
	    setup->request_type, setup->length));

	// Issue a Configure Endpoint command, if needed.
	if (configure_endpoint_needed(setup)) {
		const errno_t err = hc_configure_device(xhci_ep_to_dev(xhci_ep));
		if (err)
			return err;
	}

	return xhci_trb_ring_enqueue_multiple(get_ring(transfer), trbs,
	    trbs_used, &transfer->interrupt_trb_phys);
}

static errno_t schedule_bulk_intr(xhci_hc_t *hc, xhci_transfer_t *transfer)
{
	xhci_trb_ring_t *const ring = get_ring(transfer);
	if (!ring)
		return EINVAL;

	/* The stream-enabled endpoints need to chain ED trb */
	xhci_endpoint_t *ep = xhci_endpoint_get(transfer->batch.ep);
	const bool use_streams = !!ep->primary_stream_data_size;

	trb_splitter_t splitter;
	trb_splitter_init(&splitter, transfer);

	const size_t trb_count = splitter.max_trb_count + use_streams;
	xhci_trb_t trbs[trb_count];
	size_t trbs_used = 0;

	while (splitter.remaining > 0)
		trb_split_next(&trbs[trbs_used++], &splitter);

	if (!use_streams) {
		/* Set the interrupt bit for last TRB */
		TRB_CTRL_SET_IOC(trbs[trbs_used - 1], 1);
	} else {
		/* Clear the chain bit on the last TRB */
		TRB_CTRL_SET_CHAIN(trbs[trbs_used - 1], 1);
		TRB_CTRL_SET_ENT(trbs[trbs_used - 1], 1);

		xhci_trb_t *ed = &trbs[trbs_used++];
		xhci_trb_clean(ed);
		ed->parameter = host2xhci(64, (uintptr_t) transfer);
		TRB_CTRL_SET_TRB_TYPE(*ed, XHCI_TRB_TYPE_EVENT_DATA);
		TRB_CTRL_SET_IOC(*ed, 1);
	}

	return xhci_trb_ring_enqueue_multiple(ring, trbs, trbs_used,
	    &transfer->interrupt_trb_phys);
}

static int schedule_isochronous(xhci_transfer_t *transfer)
{
	endpoint_t *ep = transfer->batch.ep;

	return ep->direction == USB_DIRECTION_OUT ?
	    isoch_schedule_out(transfer) :
	    isoch_schedule_in(transfer);
}

errno_t xhci_handle_transfer_event(xhci_hc_t *hc, xhci_trb_t *trb)
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
		/* We schedule those only when streams are involved */
		assert(ep->primary_stream_ctx_array != NULL);

		/* We are received transfer pointer instead - work with that */
		transfer = (xhci_transfer_t *) addr;
		xhci_trb_ring_update_dequeue(get_ring(transfer),
		    transfer->interrupt_trb_phys);
		batch = &transfer->batch;
	} else {
		xhci_trb_ring_update_dequeue(&ep->ring, addr);

		if (ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS) {
			isoch_handle_transfer_event(hc, ep, trb);
			/* Dropping temporary reference */
			endpoint_del_ref(&ep->base);
			return EOK;
		}

		fibril_mutex_lock(&ep->guard);
		batch = ep->base.active_batch;
		endpoint_deactivate_locked(&ep->base);
		fibril_mutex_unlock(&ep->guard);

		if (!batch) {
			/* Dropping temporary reference */
			endpoint_del_ref(&ep->base);
			return ENOENT;
		}

		transfer = xhci_transfer_from_batch(batch);
	}

	const xhci_trb_completion_code_t completion_code = TRB_COMPLETION_CODE(*trb);
	switch (completion_code) {
	case XHCI_TRBC_SHORT_PACKET:
	case XHCI_TRBC_SUCCESS:
		batch->error = EOK;
		batch->transferred_size = batch->size - TRB_TRANSFER_LENGTH(*trb);
		break;

	case XHCI_TRBC_DATA_BUFFER_ERROR:
		usb_log_warning("Transfer ended with data buffer error.");
		batch->error = EAGAIN;
		batch->transferred_size = 0;
		break;

	case XHCI_TRBC_BABBLE_DETECTED_ERROR:
		usb_log_warning("Babble detected during the transfer.");
		batch->error = EAGAIN;
		batch->transferred_size = 0;
		break;

	case XHCI_TRBC_USB_TRANSACTION_ERROR:
		usb_log_warning("USB Transaction error.");
		batch->error = EAGAIN;
		batch->transferred_size = 0;
		break;

	case XHCI_TRBC_TRB_ERROR:
		usb_log_error("Invalid transfer parameters.");
		batch->error = EINVAL;
		batch->transferred_size = 0;
		break;

	case XHCI_TRBC_STALL_ERROR:
		usb_log_warning("Stall condition detected.");
		batch->error = ESTALL;
		batch->transferred_size = 0;
		break;

	case XHCI_TRBC_SPLIT_TRANSACTION_ERROR:
		usb_log_error("Split transcation error detected.");
		batch->error = EAGAIN;
		batch->transferred_size = 0;
		break;

	default:
		usb_log_warning("Transfer not successfull: %u", completion_code);
		batch->error = EIO;
	}

	assert(batch->transferred_size <= batch->size);

	usb_transfer_batch_finish(batch);
	/* Dropping temporary reference */
	endpoint_del_ref(&ep->base);
	return EOK;
}

typedef errno_t (*transfer_handler)(xhci_hc_t *, xhci_transfer_t *);

static const transfer_handler transfer_handlers[] = {
	[USB_TRANSFER_CONTROL] = schedule_control,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
	[USB_TRANSFER_BULK] = schedule_bulk_intr,
	[USB_TRANSFER_INTERRUPT] = schedule_bulk_intr,
};

/**
 * Schedule a batch for xHC.
 *
 * Bus callback.
 */
errno_t xhci_transfer_schedule(usb_transfer_batch_t *batch)
{
	endpoint_t *ep = batch->ep;

	xhci_hc_t *hc = bus_to_hc(endpoint_get_bus(batch->ep));
	xhci_transfer_t *transfer = xhci_transfer_from_batch(batch);
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);
	xhci_device_t *xhci_dev = xhci_ep_to_dev(xhci_ep);

	if (!batch->target.address) {
		usb_log_error("Attempted to schedule transfer to address 0.");
		return EINVAL;
	}

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

	/*
	 * If this is a ClearFeature(ENDPOINT_HALT) request, we have to issue
	 * the Reset Endpoint command.
	 */
	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL &&
	    batch->dir == USB_DIRECTION_OUT) {
		const usb_device_request_setup_packet_t *request = &batch->setup.packet;
		if (request->request == USB_DEVREQ_CLEAR_FEATURE &&
		    request->request_type == USB_REQUEST_RECIPIENT_ENDPOINT &&
		    request->value == USB_FEATURE_ENDPOINT_HALT) {
			const uint16_t index = uint16_usb2host(request->index);
			const usb_endpoint_t ep_num = index & 0xf;
			const usb_direction_t dir = (index >> 7) ?
			    USB_DIRECTION_IN :
			    USB_DIRECTION_OUT;
			endpoint_t *halted_ep = bus_find_endpoint(&xhci_dev->base, ep_num, dir);
			if (halted_ep) {
				/*
				 * TODO: Find out how to come up with stream_id. It might be
				 * possible that we have to clear all of them.
				 */
				const errno_t err = xhci_endpoint_clear_halt(xhci_endpoint_get(halted_ep), 0);
				endpoint_del_ref(halted_ep);
				if (err) {
					/*
					 * The endpoint halt condition failed to be cleared in HC.
					 * As it does not make sense to send the reset to the device
					 * itself, return as unschedulable answer.
					 *
					 * Furthermore, if this is a request to clear EP 0 stall, it
					 * would be gone forever, as the endpoint is halted.
					 */
					return err;
				}
			} else {
				usb_log_warning("Device(%u): Resetting unregistered endpoint"
				    " %u %s.", xhci_dev->base.address, ep_num,
				    usb_str_direction(dir));
			}
		}
	}

	errno_t err;
	fibril_mutex_lock(&xhci_ep->guard);

	if ((err = endpoint_activate_locked(ep, batch))) {
		fibril_mutex_unlock(&xhci_ep->guard);
		return err;
	}

	if ((err = transfer_handlers[batch->ep->transfer_type](hc, transfer))) {
		endpoint_deactivate_locked(ep);
		fibril_mutex_unlock(&xhci_ep->guard);
		return err;
	}

	hc_ring_ep_doorbell(xhci_ep, batch->target.stream);
	fibril_mutex_unlock(&xhci_ep->guard);
	return EOK;
}
