/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver USB transfer structure
 */

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include <usb/host/utils/malloc32.h>

#include "uhci_batch.h"
#include "hc.h"
#include "hw_struct/transfer_descriptor.h"

#define DEFAULT_ERROR_COUNT 3

/** Transfer batch setup table. */
static void (*const batch_setup[])(uhci_transfer_batch_t *);

/** Destroys uhci_transfer_batch_t structure.
 *
 * @param[in] uhci_batch Instance to destroy.
 */
void uhci_transfer_batch_destroy(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	dma_buffer_free(&uhci_batch->uhci_dma_buffer);
	free(uhci_batch);
}

/** Allocate memory and initialize internal data structure.
 *
 * @param[in] usb_batch Pointer to generic USB batch structure.
 * @return Valid pointer if all structures were successfully created,
 * NULL otherwise.
 */
uhci_transfer_batch_t *uhci_transfer_batch_create(endpoint_t *ep)
{
	uhci_transfer_batch_t *uhci_batch =
	    calloc(1, sizeof(uhci_transfer_batch_t));
	if (!uhci_batch) {
		usb_log_error("Failed to allocate UHCI batch.");
		return NULL;
	}

	usb_transfer_batch_init(&uhci_batch->base, ep);

	link_initialize(&uhci_batch->link);
	return uhci_batch;
}

/*
 * Prepares batch for committing.
 *
 * Determines the number of needed transfer descriptors (TDs).
 * Prepares a transport buffer (that is accessible by the hardware).
 * Initializes parameters needed for the transfer and callback.
 */
int uhci_transfer_batch_prepare(uhci_transfer_batch_t *uhci_batch)
{
	static_assert((sizeof(td_t) % 16) == 0, "");

	usb_transfer_batch_t *usb_batch = &uhci_batch->base;

	uhci_batch->td_count = (usb_batch->size + usb_batch->ep->max_packet_size - 1) /
	    usb_batch->ep->max_packet_size;

	if (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		uhci_batch->td_count += 2;
	}

	const size_t setup_size = (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL) ?
	    USB_SETUP_PACKET_SIZE :
	    0;

	const size_t total_size = (sizeof(td_t) * uhci_batch->td_count) +
	    sizeof(qh_t) + setup_size;

	if (dma_buffer_alloc(&uhci_batch->uhci_dma_buffer, total_size)) {
		usb_log_error("Failed to allocate UHCI buffer.");
		return ENOMEM;
	}
	memset(uhci_batch->uhci_dma_buffer.virt, 0, total_size);

	uhci_batch->tds = uhci_batch->uhci_dma_buffer.virt;
	uhci_batch->qh = (qh_t *) &uhci_batch->tds[uhci_batch->td_count];

	qh_init(uhci_batch->qh);
	qh_set_element_td(uhci_batch->qh, &uhci_batch->tds[0]);

	void *setup_buffer = uhci_transfer_batch_setup_buffer(uhci_batch);
	assert(setup_buffer == (void *) (uhci_batch->qh + 1));
	/* Copy SETUP packet data to the device buffer */
	memcpy(setup_buffer, usb_batch->setup.buffer, setup_size);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " memory structures ready.", usb_batch,
	    USB_TRANSFER_BATCH_ARGS(*usb_batch));

	assert(batch_setup[usb_batch->ep->transfer_type]);
	batch_setup[usb_batch->ep->transfer_type](uhci_batch);

	return EOK;
}

/** Check batch TDs for activity.
 *
 * @param[in] uhci_batch Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs. Stop with false if there is an active one (it is to be
 * processed). Stop with true if an error is found. Return true if the last TD
 * is reached.
 */
bool uhci_transfer_batch_check_completed(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	usb_transfer_batch_t *batch = &uhci_batch->base;

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " checking %zu transfer(s) for completion.",
	    uhci_batch, USB_TRANSFER_BATCH_ARGS(*batch),
	    uhci_batch->td_count);
	batch->transferred_size = 0;

	uhci_endpoint_t *uhci_ep = (uhci_endpoint_t *) batch->ep;

	for (size_t i = 0; i < uhci_batch->td_count; ++i) {
		if (td_is_active(&uhci_batch->tds[i])) {
			return false;
		}

		batch->error = td_status(&uhci_batch->tds[i]);
		if (batch->error != EOK) {
			assert(batch->ep != NULL);

			usb_log_debug("Batch %p found error TD(%zu->%p):%"
			    PRIx32 ".", uhci_batch, i,
			    &uhci_batch->tds[i], uhci_batch->tds[i].status);
			td_print_status(&uhci_batch->tds[i]);

			uhci_ep->toggle = td_toggle(&uhci_batch->tds[i]);
			goto substract_ret;
		}

		batch->transferred_size +=
		    td_act_size(&uhci_batch->tds[i]);
		if (td_is_short(&uhci_batch->tds[i]))
			goto substract_ret;
	}
substract_ret:
	if (batch->transferred_size > 0 && batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		assert(batch->transferred_size >= USB_SETUP_PACKET_SIZE);
		batch->transferred_size -= USB_SETUP_PACKET_SIZE;
	}

	assert(batch->transferred_size <= batch->size);

	return true;
}

/** Direction to pid conversion table */
static const usb_packet_id direction_pids[] = {
	[USB_DIRECTION_IN] = USB_PID_IN,
	[USB_DIRECTION_OUT] = USB_PID_OUT,
};

/** Prepare generic data transfer
 *
 * @param[in] uhci_batch Batch structure to use.
 * @param[in] dir Communication direction.
 *
 * Transactions with alternating toggle bit and supplied pid value.
 * The last transfer is marked with IOC flag.
 */
static void batch_data(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);

	usb_direction_t dir = uhci_batch->base.dir;
	assert(dir == USB_DIRECTION_OUT || dir == USB_DIRECTION_IN);

	const usb_packet_id pid = direction_pids[dir];
	const bool low_speed =
	    uhci_batch->base.ep->device->speed == USB_SPEED_LOW;
	const size_t mps = uhci_batch->base.ep->max_packet_size;

	uhci_endpoint_t *uhci_ep = (uhci_endpoint_t *) uhci_batch->base.ep;

	int toggle = uhci_ep->toggle;
	assert(toggle == 0 || toggle == 1);

	size_t td = 0;
	size_t remain_size = uhci_batch->base.size;
	char *buffer = uhci_transfer_batch_data_buffer(uhci_batch);

	while (remain_size > 0) {
		const size_t packet_size = min(remain_size, mps);

		const td_t *next_td = (td + 1 < uhci_batch->td_count) ?
		    &uhci_batch->tds[td + 1] : NULL;

		assert(td < uhci_batch->td_count);
		td_init(
		    &uhci_batch->tds[td], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, uhci_batch->base.target, pid, buffer, next_td);

		++td;
		toggle = 1 - toggle;
		buffer += packet_size;
		remain_size -= packet_size;
	}
	td_set_ioc(&uhci_batch->tds[td - 1]);
	uhci_ep->toggle = toggle;
	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized.",
	    uhci_batch,
	    usb_str_transfer_type(uhci_batch->base.ep->transfer_type),
	    usb_str_direction(uhci_batch->base.ep->direction),
	    USB_TRANSFER_BATCH_ARGS(uhci_batch->base));
}

/** Prepare generic control transfer
 *
 * @param[in] uhci_batch Batch structure to use.
 * @param[in] dir Communication direction.
 *
 * Setup stage with toggle 0 and USB_PID_SETUP.
 * Data stage with alternating toggle and pid determined by the communication
 * direction.
 * Status stage with toggle 1 and pid determined by the communication direction.
 * The last transfer is marked with IOC.
 */
static void batch_control(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);

	usb_direction_t dir = uhci_batch->base.dir;
	assert(dir == USB_DIRECTION_OUT || dir == USB_DIRECTION_IN);
	assert(uhci_batch->td_count >= 2);
	static const usb_packet_id status_stage_pids[] = {
		[USB_DIRECTION_IN] = USB_PID_OUT,
		[USB_DIRECTION_OUT] = USB_PID_IN,
	};

	const usb_packet_id data_stage_pid = direction_pids[dir];
	const usb_packet_id status_stage_pid = status_stage_pids[dir];
	const bool low_speed =
	    uhci_batch->base.ep->device->speed == USB_SPEED_LOW;
	const size_t mps = uhci_batch->base.ep->max_packet_size;
	const usb_target_t target = uhci_batch->base.target;

	/* setup stage */
	td_init(
	    &uhci_batch->tds[0], DEFAULT_ERROR_COUNT,
	    USB_SETUP_PACKET_SIZE, 0, false,
	    low_speed, target, USB_PID_SETUP,
	    uhci_transfer_batch_setup_buffer(uhci_batch), &uhci_batch->tds[1]);

	/* data stage */
	size_t td = 1;
	unsigned toggle = 1;
	size_t remain_size = uhci_batch->base.size;
	char *buffer = uhci_transfer_batch_data_buffer(uhci_batch);

	while (remain_size > 0) {
		const size_t packet_size = min(remain_size, mps);

		td_init(
		    &uhci_batch->tds[td], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, target, data_stage_pid,
		    buffer, &uhci_batch->tds[td + 1]);

		++td;
		toggle = 1 - toggle;
		buffer += packet_size;
		remain_size -= packet_size;
		assert(td < uhci_batch->td_count);
	}

	/* status stage */
	assert(td == uhci_batch->td_count - 1);

	td_init(
	    &uhci_batch->tds[td], DEFAULT_ERROR_COUNT, 0, 1, false, low_speed,
	    target, status_stage_pid, NULL, NULL);
	td_set_ioc(&uhci_batch->tds[td]);

	usb_log_debug2("Control last TD status: %x.",
	    uhci_batch->tds[td].status);
}

static void (*const batch_setup[])(uhci_transfer_batch_t *) =
    {
	[USB_TRANSFER_CONTROL] = batch_control,
	[USB_TRANSFER_BULK] = batch_data,
	[USB_TRANSFER_INTERRUPT] = batch_data,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
};
/**
 * @}
 */
