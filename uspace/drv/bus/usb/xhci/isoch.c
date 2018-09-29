/*
 * Copyright (c) 2018 Ondrej Hlavaty, Michal Staruch
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
 * @brief The host controller endpoint management.
 */

#include <str_error.h>
#include <macros.h>

#include "endpoint.h"
#include "hw_struct/trb.h"
#include "hw_struct/regs.h"
#include "trb_ring.h"
#include "hc.h"
#include "bus.h"

#include "isoch.h"

void isoch_init(xhci_endpoint_t *ep, const usb_endpoint_descriptors_t *desc)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	fibril_mutex_initialize(&isoch->guard);
	fibril_condvar_initialize(&isoch->avail);

	const xhci_hc_t *hc = bus_to_xhci_bus(ep->base.device->bus)->hc;

	/*
	 * We shall cover at least twice the IST period, otherwise we will get
	 * an over/underrun every time.
	 */
	isoch->buffer_count = (2 * hc->ist) / ep->interval;

	/* 2 buffers are the very minimum. */
	isoch->buffer_count = max(2, isoch->buffer_count);

	usb_log_debug("[isoch] isoch setup with %zu buffers", isoch->buffer_count);
}

static void isoch_reset(xhci_endpoint_t *ep)
{
	xhci_isoch_t *const isoch = ep->isoch;
	assert(fibril_mutex_is_locked(&isoch->guard));

	isoch->dequeue = isoch->enqueue = isoch->hw_enqueue = 0;

	for (size_t i = 0; i < isoch->buffer_count; ++i) {
		isoch->transfers[i].state = ISOCH_EMPTY;
	}

	fibril_timer_clear_locked(isoch->feeding_timer);
	isoch->last_mf = -1U;
	usb_log_info("[isoch] Endpoint" XHCI_EP_FMT ": Data flow reset.",
	    XHCI_EP_ARGS(*ep));
}

static void isoch_reset_no_timer(xhci_endpoint_t *ep)
{
	xhci_isoch_t *const isoch = ep->isoch;
	assert(fibril_mutex_is_locked(&isoch->guard));
	/*
	 * As we cannot clear timer when we are triggered by it,
	 * we have to avoid doing it in common method.
	 */
	fibril_timer_clear_locked(isoch->reset_timer);
	isoch_reset(ep);
}

static void isoch_reset_timer(void *ep)
{
	xhci_isoch_t *const isoch = xhci_endpoint_get(ep)->isoch;
	fibril_mutex_lock(&isoch->guard);
	isoch_reset(ep);
	fibril_mutex_unlock(&isoch->guard);
}

/*
 * Fast transfers could trigger the reset timer before the data is processed,
 * leading into false reset.
 */
#define RESET_TIMER_DELAY 100000
static void timer_schedule_reset(xhci_endpoint_t *ep)
{
	xhci_isoch_t *const isoch = ep->isoch;
	const usec_t delay = isoch->buffer_count * ep->interval * 125 +
	    RESET_TIMER_DELAY;

	fibril_timer_clear_locked(isoch->reset_timer);
	fibril_timer_set_locked(isoch->reset_timer, delay,
	    isoch_reset_timer, ep);
}

void isoch_fini(xhci_endpoint_t *ep)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	if (isoch->feeding_timer) {
		fibril_timer_clear(isoch->feeding_timer);
		fibril_timer_destroy(isoch->feeding_timer);
		fibril_timer_clear(isoch->reset_timer);
		fibril_timer_destroy(isoch->reset_timer);
	}

	if (isoch->transfers) {
		for (size_t i = 0; i < isoch->buffer_count; ++i)
			dma_buffer_free(&isoch->transfers[i].data);
		free(isoch->transfers);
	}
}

/**
 * Allocate isochronous buffers. Create the feeding timer.
 */
errno_t isoch_alloc_transfers(xhci_endpoint_t *ep)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	isoch->feeding_timer = fibril_timer_create(&isoch->guard);
	isoch->reset_timer = fibril_timer_create(&isoch->guard);
	if (!isoch->feeding_timer)
		return ENOMEM;

	isoch->transfers = calloc(isoch->buffer_count, sizeof(xhci_isoch_transfer_t));
	if (!isoch->transfers)
		goto err;

	for (size_t i = 0; i < isoch->buffer_count; ++i) {
		xhci_isoch_transfer_t *transfer = &isoch->transfers[i];
		if (dma_buffer_alloc(&transfer->data, ep->base.max_transfer_size)) {
			goto err;
		}
	}

	fibril_mutex_lock(&isoch->guard);
	isoch_reset_no_timer(ep);
	fibril_mutex_unlock(&isoch->guard);

	return EOK;
err:
	isoch_fini(ep);
	return ENOMEM;
}

static errno_t schedule_isochronous_trb(xhci_endpoint_t *ep, xhci_isoch_transfer_t *it)
{
	xhci_trb_t trb;
	xhci_trb_clean(&trb);

	trb.parameter = host2xhci(64, dma_buffer_phys_base(&it->data));
	TRB_CTRL_SET_XFER_LEN(trb, it->size);
	TRB_CTRL_SET_TD_SIZE(trb, 0);
	TRB_CTRL_SET_IOC(trb, 1);
	TRB_CTRL_SET_TRB_TYPE(trb, XHCI_TRB_TYPE_ISOCH);

	// see 4.14.1 and 4.11.2.3 for the explanation, how to calculate those
	size_t tdpc = it->size / 1024 + ((it->size % 1024) ? 1 : 0);
	size_t tbc = tdpc / ep->max_burst;
	if (!tdpc % ep->max_burst)
		--tbc;
	size_t bsp = tdpc % ep->max_burst;
	size_t tlbpc = (bsp ? bsp : ep->max_burst) - 1;

	TRB_ISOCH_SET_TBC(trb, tbc);
	TRB_ISOCH_SET_TLBPC(trb, tlbpc);
	TRB_ISOCH_SET_FRAMEID(trb, (it->mfindex / 8) % 2048);

	const errno_t err = xhci_trb_ring_enqueue(&ep->ring, &trb, &it->interrupt_trb_phys);
	return err;
}

/** The number of bits the MFINDEX is stored in at HW */
#define EPOCH_BITS 14
/** The delay in usec for the epoch wrap */
#define EPOCH_DELAY 500000
/** The amount of microframes the epoch is checked for a delay */
#define EPOCH_LOW_MFINDEX 8 * 100

static inline uint64_t get_system_time()
{
	struct timespec ts;
	getuptime(&ts);
	return SEC2USEC(ts.tv_sec) + NSEC2USEC(ts.tv_nsec);
}

static inline uint64_t get_current_microframe(const xhci_hc_t *hc)
{
	const uint32_t reg_mfindex = XHCI_REG_RD(hc->rt_regs, XHCI_RT_MFINDEX);
	/*
	 * If the mfindex is low and the time passed since last mfindex wrap is too
	 * high, we have entered the new epoch already (and haven't received event
	 * yet).
	 */
	uint64_t epoch = hc->wrap_count;
	if (reg_mfindex < EPOCH_LOW_MFINDEX &&
	    get_system_time() - hc->wrap_time > EPOCH_DELAY) {
		++epoch;
	}
	return (epoch << EPOCH_BITS) + reg_mfindex;
}

static inline void calc_next_mfindex(xhci_endpoint_t *ep, xhci_isoch_transfer_t *it)
{
	xhci_isoch_t *const isoch = ep->isoch;
	if (isoch->last_mf == -1U) {
		const xhci_bus_t *bus = bus_to_xhci_bus(ep->base.device->bus);
		const xhci_hc_t *hc = bus->hc;

		/*
		 * Delay the first frame by some time to fill the buffer, but at most 10
		 * miliseconds.
		 */
		const uint64_t delay = min(isoch->buffer_count * ep->interval, 10 * 8);
		it->mfindex = get_current_microframe(hc) + 1 + delay + hc->ist;

		// Align to ESIT start boundary
		it->mfindex += ep->interval - 1;
		it->mfindex &= ~(ep->interval - 1);
	} else {
		it->mfindex = isoch->last_mf + ep->interval;
	}
}

/** 825 ms in uframes */
#define END_FRAME_DELAY   (895000 / 125)

typedef enum {
	WINDOW_TOO_SOON,
	WINDOW_INSIDE,
	WINDOW_TOO_LATE,
} window_position_t;

typedef struct {
	window_position_t position;
	uint64_t offset;
} window_decision_t;

/**
 * Decide on the position of mfindex relatively to the window specified by
 * Start Frame ID and End Frame ID. The resulting structure contains the
 * decision, and in case of the mfindex being outside, also the number of
 * uframes it's off.
 */
static inline void window_decide(window_decision_t *res, xhci_hc_t *hc,
    uint64_t mfindex)
{
	const uint64_t current_mf = get_current_microframe(hc);
	const uint64_t start = current_mf + hc->ist + 1;
	const uint64_t end = current_mf + END_FRAME_DELAY;

	if (mfindex < start) {
		res->position = WINDOW_TOO_LATE;
		res->offset = start - mfindex;
	} else if (mfindex <= end) {
		res->position = WINDOW_INSIDE;
	} else {
		res->position = WINDOW_TOO_SOON;
		res->offset = mfindex - end;
	}
}

static void isoch_feed_out_timer(void *);
static void isoch_feed_in_timer(void *);

/**
 * Schedule TRBs with filled buffers to HW. Takes filled isoch transfers and
 * pushes their TRBs to the ring.
 *
 * According to 4.11.2.5, we can't just push all TRBs we have. We must not do
 * it too late, but also not too soon.
 */
static void isoch_feed_out(xhci_endpoint_t *ep)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;
	assert(fibril_mutex_is_locked(&isoch->guard));

	xhci_bus_t *bus = bus_to_xhci_bus(ep->base.device->bus);
	xhci_hc_t *hc = bus->hc;

	bool fed = false;

	while (isoch->transfers[isoch->hw_enqueue].state == ISOCH_FILLED) {
		xhci_isoch_transfer_t *const it = &isoch->transfers[isoch->hw_enqueue];
		usec_t delay;

		assert(it->state == ISOCH_FILLED);

		window_decision_t wd;
		window_decide(&wd, hc, it->mfindex);

		switch (wd.position) {
		case WINDOW_TOO_SOON:
			delay = wd.offset * 125;
			usb_log_debug("[isoch] delaying feeding buffer %zu for %lldus",
			    it - isoch->transfers, delay);
			fibril_timer_set_locked(isoch->feeding_timer, delay,
			    isoch_feed_out_timer, ep);
			goto out;

		case WINDOW_INSIDE:
			usb_log_debug("[isoch] feeding buffer %zu at 0x%" PRIx64,
			    it - isoch->transfers, it->mfindex);
			it->error = schedule_isochronous_trb(ep, it);
			if (it->error) {
				it->state = ISOCH_COMPLETE;
			} else {
				it->state = ISOCH_FED;
				fed = true;
			}

			isoch->hw_enqueue = (isoch->hw_enqueue + 1) % isoch->buffer_count;
			break;

		case WINDOW_TOO_LATE:
			/*
			 * Missed the opportunity to schedule. Just mark this transfer as
			 * skipped.
			 */
			usb_log_debug("[isoch] missed feeding buffer %zu at 0x%" PRIx64 " by "
			    "%" PRIu64 " uframes", it - isoch->transfers, it->mfindex, wd.offset);
			it->state = ISOCH_COMPLETE;
			it->error = EOK;
			it->size = 0;

			isoch->hw_enqueue = (isoch->hw_enqueue + 1) % isoch->buffer_count;
			break;
		}
	}

out:
	if (fed) {
		hc_ring_ep_doorbell(ep, 0);
		/*
		 * The ring may be dead. If no event happens until the delay, reset the
		 * endpoint.
		 */
		timer_schedule_reset(ep);
	}

}

static void isoch_feed_out_timer(void *ep)
{
	xhci_isoch_t *const isoch = xhci_endpoint_get(ep)->isoch;
	fibril_mutex_lock(&isoch->guard);
	isoch_feed_out(ep);
	fibril_mutex_unlock(&isoch->guard);
}

/**
 * Schedule TRBs with empty, withdrawn buffers to HW. Takes empty isoch
 * transfers and pushes their TRBs to the ring.
 *
 * According to 4.11.2.5, we can't just push all TRBs we have. We must not do
 * it too late, but also not too soon.
 */
static void isoch_feed_in(xhci_endpoint_t *ep)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;
	assert(fibril_mutex_is_locked(&isoch->guard));

	xhci_bus_t *bus = bus_to_xhci_bus(ep->base.device->bus);
	xhci_hc_t *hc = bus->hc;

	bool fed = false;

	while (isoch->transfers[isoch->enqueue].state <= ISOCH_FILLED) {
		xhci_isoch_transfer_t *const it = &isoch->transfers[isoch->enqueue];
		usec_t delay;

		/* IN buffers are "filled" with free space */
		if (it->state == ISOCH_EMPTY) {
			it->size = ep->base.max_transfer_size;
			it->state = ISOCH_FILLED;
			calc_next_mfindex(ep, it);
		}

		window_decision_t wd;
		window_decide(&wd, hc, it->mfindex);

		switch (wd.position) {
		case WINDOW_TOO_SOON:
			/* Not allowed to feed yet. Defer to later. */
			delay = wd.offset * 125;
			usb_log_debug("[isoch] delaying feeding buffer %zu for %lldus",
			    it - isoch->transfers, delay);
			fibril_timer_set_locked(isoch->feeding_timer, delay,
			    isoch_feed_in_timer, ep);
			goto out;
		case WINDOW_TOO_LATE:
			usb_log_debug("[isoch] missed feeding buffer %zu at 0x%" PRIx64 " by"
			    "%" PRIu64 " uframes", it - isoch->transfers, it->mfindex, wd.offset);
			/* Missed the opportunity to schedule. Schedule ASAP. */
			it->mfindex += wd.offset;
			// Align to ESIT start boundary
			it->mfindex += ep->interval - 1;
			it->mfindex &= ~(ep->interval - 1);

			/* fallthrough */
		case WINDOW_INSIDE:
			isoch->enqueue = (isoch->enqueue + 1) % isoch->buffer_count;
			isoch->last_mf = it->mfindex;

			usb_log_debug("[isoch] feeding buffer %zu at 0x%" PRIx64,
			    it - isoch->transfers, it->mfindex);

			it->error = schedule_isochronous_trb(ep, it);
			if (it->error) {
				it->state = ISOCH_COMPLETE;
			} else {
				it->state = ISOCH_FED;
				fed = true;
			}
			break;
		}
	}
out:

	if (fed) {
		hc_ring_ep_doorbell(ep, 0);
		/*
		 * The ring may be dead. If no event happens until the delay, reset the
		 * endpoint.
		 */
		timer_schedule_reset(ep);
	}
}

static void isoch_feed_in_timer(void *ep)
{
	xhci_isoch_t *const isoch = xhci_endpoint_get(ep)->isoch;
	fibril_mutex_lock(&isoch->guard);
	isoch_feed_in(ep);
	fibril_mutex_unlock(&isoch->guard);
}

/**
 * First, withdraw all (at least one) results left by previous transfers to
 * make room in the ring. Stop on first error.
 *
 * When there is at least one buffer free, fill it with data. Then try to feed
 * it to the xHC.
 */
errno_t isoch_schedule_out(xhci_transfer_t *transfer)
{
	errno_t err = EOK;

	xhci_endpoint_t *ep = xhci_endpoint_get(transfer->batch.ep);
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	/* This shall be already checked by endpoint */
	assert(transfer->batch.size <= ep->base.max_transfer_size);

	fibril_mutex_lock(&isoch->guard);

	/* Get the buffer to write to */
	xhci_isoch_transfer_t *it = &isoch->transfers[isoch->enqueue];

	/* Wait for the buffer to be completed */
	while (it->state == ISOCH_FED || it->state == ISOCH_FILLED) {
		fibril_condvar_wait(&isoch->avail, &isoch->guard);
		/* The enqueue ptr may have changed while sleeping */
		it = &isoch->transfers[isoch->enqueue];
	}

	isoch->enqueue = (isoch->enqueue + 1) % isoch->buffer_count;

	/* Withdraw results from previous transfers. */
	transfer->batch.transferred_size = 0;
	xhci_isoch_transfer_t *res = &isoch->transfers[isoch->dequeue];
	while (res->state == ISOCH_COMPLETE) {
		isoch->dequeue = (isoch->dequeue + 1) % isoch->buffer_count;

		res->state = ISOCH_EMPTY;
		transfer->batch.transferred_size += res->size;
		transfer->batch.error = res->error;
		if (res->error)
			break; // Announce one error at a time

		res = &isoch->transfers[isoch->dequeue];
	}

	assert(it->state == ISOCH_EMPTY);

	/* Calculate when to schedule next transfer */
	calc_next_mfindex(ep, it);
	isoch->last_mf = it->mfindex;
	usb_log_debug("[isoch] buffer %zu will be on schedule at 0x%" PRIx64,
	    it - isoch->transfers, it->mfindex);

	/* Prepare the transfer. */
	it->size = transfer->batch.size;
	memcpy(it->data.virt, transfer->batch.dma_buffer.virt, it->size);
	it->state = ISOCH_FILLED;

	fibril_timer_clear_locked(isoch->feeding_timer);
	isoch_feed_out(ep);

	fibril_mutex_unlock(&isoch->guard);

	usb_transfer_batch_finish(&transfer->batch);
	return err;
}

/**
 * IN is in fact easier than OUT. Our responsibility is just to feed all empty
 * buffers, and fetch one filled buffer from the ring.
 */
errno_t isoch_schedule_in(xhci_transfer_t *transfer)
{
	xhci_endpoint_t *ep = xhci_endpoint_get(transfer->batch.ep);
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	if (transfer->batch.size < ep->base.max_transfer_size) {
		usb_log_error("Cannot schedule an undersized isochronous transfer.");
		return ELIMIT;
	}

	fibril_mutex_lock(&isoch->guard);

	xhci_isoch_transfer_t *it = &isoch->transfers[isoch->dequeue];

	/* Wait for at least one transfer to complete. */
	while (it->state != ISOCH_COMPLETE) {
		/* First, make sure we will have something to read. */
		fibril_timer_clear_locked(isoch->feeding_timer);
		isoch_feed_in(ep);

		usb_log_debug("[isoch] waiting for buffer %zu to be completed",
		    it - isoch->transfers);
		fibril_condvar_wait(&isoch->avail, &isoch->guard);

		/* The enqueue ptr may have changed while sleeping */
		it = &isoch->transfers[isoch->dequeue];
	}

	isoch->dequeue = (isoch->dequeue + 1) % isoch->buffer_count;

	/* Withdraw results from previous transfer. */
	if (!it->error) {
		memcpy(transfer->batch.dma_buffer.virt, it->data.virt, it->size);
		transfer->batch.transferred_size = it->size;
		transfer->batch.error = it->error;
	}

	/* Prepare the empty buffer */
	it->state = ISOCH_EMPTY;

	fibril_mutex_unlock(&isoch->guard);
	usb_transfer_batch_finish(&transfer->batch);

	return EOK;
}

void isoch_handle_transfer_event(xhci_hc_t *hc, xhci_endpoint_t *ep,
    xhci_trb_t *trb)
{
	assert(ep->base.transfer_type == USB_TRANSFER_ISOCHRONOUS);
	xhci_isoch_t *const isoch = ep->isoch;

	fibril_mutex_lock(&ep->isoch->guard);

	errno_t err;
	const xhci_trb_completion_code_t completion_code = TRB_COMPLETION_CODE(*trb);

	switch (completion_code) {
	case XHCI_TRBC_RING_OVERRUN:
	case XHCI_TRBC_RING_UNDERRUN:
		/*
		 * For OUT, there was nothing to process.
		 * For IN, the buffer has overfilled.
		 * In either case, reset the ring.
		 */
		usb_log_warning("Ring over/underrun.");
		isoch_reset_no_timer(ep);
		fibril_condvar_broadcast(&ep->isoch->avail);
		fibril_mutex_unlock(&ep->isoch->guard);
		goto out;
	case XHCI_TRBC_SHORT_PACKET:
	case XHCI_TRBC_SUCCESS:
		err = EOK;
		break;
	default:
		usb_log_warning("Transfer not successfull: %u", completion_code);
		err = EIO;
		break;
	}

	/*
	 * The order of delivering events is not necessarily the one we would
	 * expect. It is safer to walk the list of our transfers and check
	 * which one it is.
	 * To minimize the amount of transfers checked, we start at dequeue pointer
	 * and exit the loop as soon as the transfer is found.
	 */
	bool found_mine = false;
	for (size_t i = 0, di = isoch->dequeue; i < isoch->buffer_count; ++i, ++di) {
		/* Wrap it back to 0, don't use modulo every loop traversal */
		if (di == isoch->buffer_count) {
			di = 0;
		}

		xhci_isoch_transfer_t *const it = &isoch->transfers[di];

		if (it->state == ISOCH_FED && it->interrupt_trb_phys == trb->parameter) {
			usb_log_debug("[isoch] buffer %zu completed", it - isoch->transfers);
			it->state = ISOCH_COMPLETE;
			it->size -= TRB_TRANSFER_LENGTH(*trb);
			it->error = err;
			found_mine = true;
			break;
		}
	}

	if (!found_mine) {
		usb_log_warning("[isoch] A transfer event occured for unknown transfer.");
	}

	/*
	 * It may happen that the driver already stopped reading (writing),
	 * and our buffers are filled (empty). As QEMU (and possibly others)
	 * does not send RING_UNDERRUN (OVERRUN) event, we set a timer to
	 * reset it after the buffers should have been consumed. If there
	 * is no issue, the timer will get restarted often enough.
	 */
	timer_schedule_reset(ep);

out:
	fibril_condvar_broadcast(&ep->isoch->avail);
	fibril_mutex_unlock(&ep->isoch->guard);
}

/**
 * @}
 */
