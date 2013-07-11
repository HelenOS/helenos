/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver USB transfer structure
 */
#include <errno.h>
#include <str_error.h>
#include <macros.h>

#include <usb/usb.h>
#include <usb/debug.h>

#include "uhci_batch.h"
#include "transfer_list.h"
#include "hw_struct/transfer_descriptor.h"
#include "utils/malloc32.h"

#define DEFAULT_ERROR_COUNT 3

/** Safely destructs uhci_transfer_batch_t structure.
 *
 * @param[in] uhci_batch Instance to destroy.
 */
static void uhci_transfer_batch_dispose(uhci_transfer_batch_t *uhci_batch)
{
	if (uhci_batch) {
		usb_transfer_batch_destroy(uhci_batch->usb_batch);
		free32(uhci_batch->device_buffer);
		free(uhci_batch);
	}
}

/** Finishes usb_transfer_batch and destroys the structure.
 *
 * @param[in] uhci_batch Instance to finish and destroy.
 */
void uhci_transfer_batch_finish_dispose(uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);
	usb_transfer_batch_finish(uhci_batch->usb_batch,
	    uhci_transfer_batch_data_buffer(uhci_batch));
	uhci_transfer_batch_dispose(uhci_batch);
}

/** Transfer batch setup table. */
static void (*const batch_setup[])(uhci_transfer_batch_t*, usb_direction_t);

/** Allocate memory and initialize internal data structure.
 *
 * @param[in] usb_batch Pointer to generic USB batch structure.
 * @return Valid pointer if all structures were successfully created,
 * NULL otherwise.
 *
 * Determines the number of needed transfer descriptors (TDs).
 * Prepares a transport buffer (that is accessible by the hardware).
 * Initializes parameters needed for the transfer and callback.
 */
uhci_transfer_batch_t * uhci_transfer_batch_get(usb_transfer_batch_t *usb_batch)
{
	assert((sizeof(td_t) % 16) == 0);
#define CHECK_NULL_DISPOSE_RETURN(ptr, message...) \
	if (ptr == NULL) { \
		usb_log_error(message); \
		uhci_transfer_batch_dispose(uhci_batch); \
		return NULL; \
	} else (void)0

	uhci_transfer_batch_t *uhci_batch =
	    calloc(1, sizeof(uhci_transfer_batch_t));
	CHECK_NULL_DISPOSE_RETURN(uhci_batch,
	    "Failed to allocate UHCI batch.\n");
	link_initialize(&uhci_batch->link);
	uhci_batch->td_count =
	    (usb_batch->buffer_size + usb_batch->ep->max_packet_size - 1)
	    / usb_batch->ep->max_packet_size;
	if (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		uhci_batch->td_count += 2;
	}

	const size_t total_size = (sizeof(td_t) * uhci_batch->td_count)
	    + sizeof(qh_t) + usb_batch->setup_size + usb_batch->buffer_size;
	uhci_batch->device_buffer = malloc32(total_size);
	CHECK_NULL_DISPOSE_RETURN(uhci_batch->device_buffer,
	    "Failed to allocate UHCI buffer.\n");
	memset(uhci_batch->device_buffer, 0, total_size);

	uhci_batch->tds = uhci_batch->device_buffer;
	uhci_batch->qh =
	    (uhci_batch->device_buffer + (sizeof(td_t) * uhci_batch->td_count));

	qh_init(uhci_batch->qh);
	qh_set_element_td(uhci_batch->qh, &uhci_batch->tds[0]);

	void *dest =
	    uhci_batch->device_buffer + (sizeof(td_t) * uhci_batch->td_count)
	    + sizeof(qh_t);
	/* Copy SETUP packet data to the device buffer */
	memcpy(dest, usb_batch->setup_buffer, usb_batch->setup_size);
	dest += usb_batch->setup_size;
	/* Copy generic data unless they are provided by the device */
	if (usb_batch->ep->direction != USB_DIRECTION_IN) {
		memcpy(dest, usb_batch->buffer, usb_batch->buffer_size);
	}
	uhci_batch->usb_batch = usb_batch;
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " memory structures ready.\n", usb_batch,
	    USB_TRANSFER_BATCH_ARGS(*usb_batch));

	const usb_direction_t dir = usb_transfer_batch_direction(usb_batch);

	assert(batch_setup[usb_batch->ep->transfer_type]);
	batch_setup[usb_batch->ep->transfer_type](uhci_batch, dir);

	return uhci_batch;
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
bool uhci_transfer_batch_is_complete(const uhci_transfer_batch_t *uhci_batch)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " checking %zu transfer(s) for completion.\n",
	    uhci_batch->usb_batch,
	    USB_TRANSFER_BATCH_ARGS(*uhci_batch->usb_batch),
	    uhci_batch->td_count);
	uhci_batch->usb_batch->transfered_size = 0;

	for (size_t i = 0;i < uhci_batch->td_count; ++i) {
		if (td_is_active(&uhci_batch->tds[i])) {
			return false;
		}

		uhci_batch->usb_batch->error = td_status(&uhci_batch->tds[i]);
		if (uhci_batch->usb_batch->error != EOK) {
			assert(uhci_batch->usb_batch->ep != NULL);

			usb_log_debug("Batch %p found error TD(%zu->%p):%"
			    PRIx32 ".\n", uhci_batch->usb_batch, i,
			    &uhci_batch->tds[i], uhci_batch->tds[i].status);
			td_print_status(&uhci_batch->tds[i]);

			endpoint_toggle_set(uhci_batch->usb_batch->ep,
			    td_toggle(&uhci_batch->tds[i]));
			if (i > 0)
				goto substract_ret;
			return true;
		}

		uhci_batch->usb_batch->transfered_size
		    += td_act_size(&uhci_batch->tds[i]);
		if (td_is_short(&uhci_batch->tds[i]))
			goto substract_ret;
	}
substract_ret:
	uhci_batch->usb_batch->transfered_size
	    -= uhci_batch->usb_batch->setup_size;
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
static void batch_data(uhci_transfer_batch_t *uhci_batch, usb_direction_t dir)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);
	assert(uhci_batch->usb_batch->ep);
	assert(dir == USB_DIRECTION_OUT || dir == USB_DIRECTION_IN);


	const usb_packet_id pid = direction_pids[dir];
	const bool low_speed =
	    uhci_batch->usb_batch->ep->speed == USB_SPEED_LOW;
	const size_t mps = uhci_batch->usb_batch->ep->max_packet_size;
	const usb_target_t target = {{
	    uhci_batch->usb_batch->ep->address,
	    uhci_batch->usb_batch->ep->endpoint }};

	int toggle = endpoint_toggle_get(uhci_batch->usb_batch->ep);
	assert(toggle == 0 || toggle == 1);

	size_t td = 0;
	size_t remain_size = uhci_batch->usb_batch->buffer_size;
	char *buffer = uhci_transfer_batch_data_buffer(uhci_batch);

	while (remain_size > 0) {
		const size_t packet_size = min(remain_size, mps);

		const td_t *next_td = (td + 1 < uhci_batch->td_count)
		    ? &uhci_batch->tds[td + 1] : NULL;

		assert(td < uhci_batch->td_count);
		td_init(
		    &uhci_batch->tds[td], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, target, pid, buffer, next_td);

		++td;
		toggle = 1 - toggle;
		buffer += packet_size;
		remain_size -= packet_size;
	}
	td_set_ioc(&uhci_batch->tds[td - 1]);
	endpoint_toggle_set(uhci_batch->usb_batch->ep, toggle);
	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized.\n", \
	    uhci_batch->usb_batch,
	    usb_str_transfer_type(uhci_batch->usb_batch->ep->transfer_type),
	    usb_str_direction(uhci_batch->usb_batch->ep->direction),
	    USB_TRANSFER_BATCH_ARGS(*uhci_batch->usb_batch));
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
static void batch_control(uhci_transfer_batch_t *uhci_batch, usb_direction_t dir)
{
	assert(uhci_batch);
	assert(uhci_batch->usb_batch);
	assert(uhci_batch->usb_batch->ep);
	assert(dir == USB_DIRECTION_OUT || dir == USB_DIRECTION_IN);
	assert(uhci_batch->td_count >= 2);
	static const usb_packet_id status_stage_pids[] = {
		[USB_DIRECTION_IN] = USB_PID_OUT,
		[USB_DIRECTION_OUT] = USB_PID_IN,
	};

	const usb_packet_id data_stage_pid = direction_pids[dir];
	const usb_packet_id status_stage_pid = status_stage_pids[dir];
	const bool low_speed =
	    uhci_batch->usb_batch->ep->speed == USB_SPEED_LOW;
	const size_t mps = uhci_batch->usb_batch->ep->max_packet_size;
	const usb_target_t target = {{
	    uhci_batch->usb_batch->ep->address,
	    uhci_batch->usb_batch->ep->endpoint }};

	/* setup stage */
	td_init(
	    &uhci_batch->tds[0], DEFAULT_ERROR_COUNT,
	    uhci_batch->usb_batch->setup_size, 0, false,
	    low_speed, target, USB_PID_SETUP,
	    uhci_transfer_batch_setup_buffer(uhci_batch), &uhci_batch->tds[1]);

	/* data stage */
	size_t td = 1;
	unsigned toggle = 1;
	size_t remain_size = uhci_batch->usb_batch->buffer_size;
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

	usb_log_debug2("Control last TD status: %x.\n",
	    uhci_batch->tds[td].status);
}

static void (*const batch_setup[])(uhci_transfer_batch_t*, usb_direction_t) =
{
	[USB_TRANSFER_CONTROL] = batch_control,
	[USB_TRANSFER_BULK] = batch_data,
	[USB_TRANSFER_INTERRUPT] = batch_data,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
};
/**
 * @}
 */
