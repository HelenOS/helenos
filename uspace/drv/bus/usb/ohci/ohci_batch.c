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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver USB transaction structure
 */

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdbool.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>

#include "ohci_batch.h"
#include "ohci_bus.h"

static void (*const batch_setup[])(ohci_transfer_batch_t*);

/** Safely destructs ohci_transfer_batch_t structure
 *
 * @param[in] ohci_batch Instance to destroy.
 */
void ohci_transfer_batch_destroy(ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);
	dma_buffer_free(&ohci_batch->ohci_dma_buffer);
	free(ohci_batch);
}

/** Allocate memory and initialize internal data structure.
 *
 * @param[in] ep Endpoint for which the batch will be created
 * @return Valid pointer if all structures were successfully created,
 * NULL otherwise.
 */
ohci_transfer_batch_t * ohci_transfer_batch_create(endpoint_t *ep)
{
	assert(ep);

	ohci_transfer_batch_t *ohci_batch =
	    calloc(1, sizeof(ohci_transfer_batch_t));
	if (!ohci_batch) {
		usb_log_error("Failed to allocate OHCI batch data.");
		return NULL;
	}

	usb_transfer_batch_init(&ohci_batch->base, ep);

	return ohci_batch;
}

/** Prepares a batch to be sent.
 *
 * Determines the number of needed transfer descriptors (TDs).
 * Prepares a transport buffer (that is accessible by the hardware).
 * Initializes parameters needed for the transfer and callback.
 */
int ohci_transfer_batch_prepare(ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);
	usb_transfer_batch_t *usb_batch = &ohci_batch->base;

	if (!batch_setup[usb_batch->ep->transfer_type])
		return ENOTSUP;

	ohci_batch->td_count = (usb_batch->size + OHCI_TD_MAX_TRANSFER - 1)
	    / OHCI_TD_MAX_TRANSFER;
	/* Control transfer need Setup and Status stage */
	if (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		ohci_batch->td_count += 2;
	}

	/* Alloc one more to NULL terminate */
	ohci_batch->tds = calloc(ohci_batch->td_count + 1, sizeof(td_t *));
	if (!ohci_batch->tds)
		return ENOMEM;

	const size_t td_size = ohci_batch->td_count * sizeof(td_t);
	const size_t setup_size = (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL)
		? USB_SETUP_PACKET_SIZE
		: 0;

	if (dma_buffer_alloc(&ohci_batch->ohci_dma_buffer, td_size + setup_size)) {
		usb_log_error("Failed to allocate OHCI DMA buffer.");
		return ENOMEM;
	}

	td_t *tds = ohci_batch->ohci_dma_buffer.virt;

	for (size_t i = 0; i < ohci_batch->td_count; i++)
		ohci_batch->tds[i] = &tds[i];

	/* Presence of this terminator makes TD initialization easier */
	ohci_batch->tds[ohci_batch->td_count] = NULL;

	ohci_batch->setup_buffer = (void *) (&tds[ohci_batch->td_count]);
	memcpy(ohci_batch->setup_buffer, usb_batch->setup.buffer, setup_size);

	ohci_batch->data_buffer = usb_batch->dma_buffer.virt;

	batch_setup[usb_batch->ep->transfer_type](ohci_batch);

	return EOK;
}

/** Check batch TDs' status.
 *
 * @param[in] ohci_batch Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs (usually there is just one). Stop with false if there is an
 * active TD. Stop with true if an error is found. Return true if the walk
 * completes with the last TD.
 */
bool ohci_transfer_batch_check_completed(ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);

	usb_transfer_batch_t *usb_batch = &ohci_batch->base;
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(usb_batch->ep);
	assert(ohci_ep);

	usb_log_debug("Batch %p checking %zu td(s) for completion.",
	    ohci_batch, ohci_batch->td_count);
	usb_log_debug2("ED: %08x:%08x:%08x:%08x.",
	    ohci_ep->ed->status, ohci_ep->ed->td_head,
	    ohci_ep->ed->td_tail, ohci_ep->ed->next);

	if (!ed_inactive(ohci_ep->ed) && ed_transfer_pending(ohci_ep->ed))
		return false;

	/* Now we may be sure that either the ED is inactive because of errors
	 * or all transfer descriptors completed successfully */

	/* Assume all data got through */
	usb_batch->transferred_size = usb_batch->size;

	/* Check all TDs */
	for (size_t i = 0; i < ohci_batch->td_count; ++i) {
		assert(ohci_batch->tds[i] != NULL);
		usb_log_debug("TD %zu: %08x:%08x:%08x:%08x.", i,
		    ohci_batch->tds[i]->status, ohci_batch->tds[i]->cbp,
		    ohci_batch->tds[i]->next, ohci_batch->tds[i]->be);

		usb_batch->error = td_error(ohci_batch->tds[i]);
		if (usb_batch->error == EOK) {
			/* If the TD got all its data through, it will report
			 * 0 bytes remain, the sole exception is INPUT with
			 * data rounding flag (short), i.e. every INPUT.
			 * Nice thing is that short packets will correctly
			 * report remaining data, thus making this computation
			 * correct (short packets need to be produced by the
			 * last TD)
			 * NOTE: This also works for CONTROL transfer as
			 * the first TD will return 0 remain.
			 * NOTE: Short packets don't break the assumption that
			 * we leave the very last(unused) TD behind.
			 */
			usb_batch->transferred_size
			    -= td_remain_size(ohci_batch->tds[i]);
		} else {
			usb_log_debug("Batch %p found error TD(%zu):%08x.",
			    ohci_batch, i, ohci_batch->tds[i]->status);

			/* ED should be stopped because of errors */
			assert((ohci_ep->ed->td_head & ED_TDHEAD_HALTED_FLAG) != 0);

			/* We don't care where the processing stopped, we just
			 * need to make sure it's not using any of the TDs owned
			 * by the transfer.
			 *
			 * As the chain is terminated by a TD in ownership of
			 * the EP, set it.
			 */
			ed_set_head_td(ohci_ep->ed, ohci_ep->tds[0]);

			/* Clear the halted condition for the next transfer */
			ed_clear_halt(ohci_ep->ed);
			break;
		}
	}
	assert(usb_batch->transferred_size <= usb_batch->size);

	/* Make sure that we are leaving the right TD behind */
	assert(addr_to_phys(ohci_ep->tds[0]) == ed_tail_td(ohci_ep->ed));
	assert(ed_tail_td(ohci_ep->ed) == ed_head_td(ohci_ep->ed));

	return true;
}

/** Starts execution of the TD list
 *
 * @param[in] ohci_batch Batch structure to use
 */
void ohci_transfer_batch_commit(const ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);

	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ohci_batch->base.ep);

	usb_log_debug("Using ED(%p): %08x:%08x:%08x:%08x.", ohci_ep->ed,
	    ohci_ep->ed->status, ohci_ep->ed->td_tail,
	    ohci_ep->ed->td_head, ohci_ep->ed->next);

	/*
	 * According to spec, we need to copy the first TD to the currently
	 * enqueued one.
	 */
	memcpy(ohci_ep->tds[0], ohci_batch->tds[0], sizeof(td_t));
	ohci_batch->tds[0] = ohci_ep->tds[0];

	td_t *last = ohci_batch->tds[ohci_batch->td_count - 1];
	td_set_next(last, ohci_ep->tds[1]);

	ed_set_tail_td(ohci_ep->ed, ohci_ep->tds[1]);

	/* Swap the EP TDs for the next transfer */
	td_t *tmp = ohci_ep->tds[0];
	ohci_ep->tds[0] = ohci_ep->tds[1];
	ohci_ep->tds[1] = tmp;
}

/** Prepare generic control transfer
 *
 * @param[in] ohci_batch Batch structure to use.
 * @param[in] dir Communication direction
 *
 * Setup stage with toggle 0 and direction BOTH(SETUP_PID)
 * Data stage with alternating toggle and direction supplied by parameter.
 * Status stage with toggle 1 and direction supplied by parameter.
 */
static void batch_control(ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);

	usb_direction_t dir = ohci_batch->base.dir;
	assert(dir == USB_DIRECTION_IN || dir == USB_DIRECTION_OUT);

	static const usb_direction_t reverse_dir[] = {
		[USB_DIRECTION_IN]  = USB_DIRECTION_OUT,
		[USB_DIRECTION_OUT] = USB_DIRECTION_IN,
	};

	int toggle = 0;
	const usb_direction_t data_dir = dir;
	const usb_direction_t status_dir = reverse_dir[dir];

	/* Setup stage */
	td_init(
	    ohci_batch->tds[0], ohci_batch->tds[1], USB_DIRECTION_BOTH,
	    ohci_batch->setup_buffer, USB_SETUP_PACKET_SIZE, toggle);
	usb_log_debug("Created CONTROL SETUP TD: %08x:%08x:%08x:%08x.",
	    ohci_batch->tds[0]->status, ohci_batch->tds[0]->cbp,
	    ohci_batch->tds[0]->next, ohci_batch->tds[0]->be);

	/* Data stage */
	size_t td_current = 1;
	const char* buffer = ohci_batch->data_buffer;
	size_t remain_size = ohci_batch->base.size;
	while (remain_size > 0) {
		const size_t transfer_size =
		    min(remain_size, OHCI_TD_MAX_TRANSFER);
		toggle = 1 - toggle;

		td_init(ohci_batch->tds[td_current],
		    ohci_batch->tds[td_current + 1],
		    data_dir, buffer, transfer_size, toggle);
		usb_log_debug("Created CONTROL DATA TD: %08x:%08x:%08x:%08x.",
		    ohci_batch->tds[td_current]->status,
		    ohci_batch->tds[td_current]->cbp,
		    ohci_batch->tds[td_current]->next,
		    ohci_batch->tds[td_current]->be);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < ohci_batch->td_count - 1);
		++td_current;
	}

	/* Status stage */
	assert(td_current == ohci_batch->td_count - 1);
	td_init(ohci_batch->tds[td_current], ohci_batch->tds[td_current + 1],
	    status_dir, NULL, 0, 1);
	usb_log_debug("Created CONTROL STATUS TD: %08x:%08x:%08x:%08x.",
	    ohci_batch->tds[td_current]->status,
	    ohci_batch->tds[td_current]->cbp,
	    ohci_batch->tds[td_current]->next,
	    ohci_batch->tds[td_current]->be);
	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized.",
	    &ohci_batch->base,
	    usb_str_transfer_type(ohci_batch->base.ep->transfer_type),
	    usb_str_direction(dir),
	    USB_TRANSFER_BATCH_ARGS(ohci_batch->base));
}

/** Prepare generic data transfer
 *
 * @param[in] ohci_batch Batch structure to use.
 * @paramp[in] dir Communication direction.
 *
 * Direction is supplied by the associated ep and toggle is maintained by the
 * OHCI hw in ED.
 */
static void batch_data(ohci_transfer_batch_t *ohci_batch)
{
	assert(ohci_batch);

	usb_direction_t dir = ohci_batch->base.dir;
	assert(dir == USB_DIRECTION_IN || dir == USB_DIRECTION_OUT);

	size_t td_current = 0;
	size_t remain_size = ohci_batch->base.size;
	char *buffer = ohci_batch->data_buffer;
	while (remain_size > 0) {
		const size_t transfer_size = remain_size > OHCI_TD_MAX_TRANSFER
		    ? OHCI_TD_MAX_TRANSFER : remain_size;

		td_init(
		    ohci_batch->tds[td_current], ohci_batch->tds[td_current + 1],
		    dir, buffer, transfer_size, -1);

		usb_log_debug("Created DATA TD: %08x:%08x:%08x:%08x.",
		    ohci_batch->tds[td_current]->status,
		    ohci_batch->tds[td_current]->cbp,
		    ohci_batch->tds[td_current]->next,
		    ohci_batch->tds[td_current]->be);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < ohci_batch->td_count);
		++td_current;
	}
	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized.",
	    &ohci_batch->base,
	    usb_str_transfer_type(ohci_batch->base.ep->transfer_type),
	    usb_str_direction(dir),
	    USB_TRANSFER_BATCH_ARGS(ohci_batch->base));
}

/** Transfer setup table. */
static void (*const batch_setup[])(ohci_transfer_batch_t*) =
{
	[USB_TRANSFER_CONTROL] = batch_control,
	[USB_TRANSFER_BULK] = batch_data,
	[USB_TRANSFER_INTERRUPT] = batch_data,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
};
/**
 * @}
 */
