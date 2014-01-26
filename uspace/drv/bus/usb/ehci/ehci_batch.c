/*
 * Copyright (c) 2014 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver USB transaction structure
 */

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdbool.h>

#include <usb/usb.h>
#include <usb/debug.h>

#include "ehci_batch.h"
#include "ehci_endpoint.h"
#include "utils/malloc32.h"

/* The buffer pointer list in the qTD is long enough to support a maximum
 * transfer size of 20K bytes. This case occurs when all five buffer pointers
 * are used and the first offset is zero. A qTD handles a 16Kbyte buffer
 * with any starting buffer alignment. EHCI specs p. 87 (pdf p. 97) */
#define EHCI_TD_MAX_TRANSFER   (16 * 1024)

static void (*const batch_setup[])(ehci_transfer_batch_t*, usb_direction_t);

/** Safely destructs ehci_transfer_batch_t structure
 *
 * @param[in] ehci_batch Instance to destroy.
 */
static void ehci_transfer_batch_dispose(ehci_transfer_batch_t *ehci_batch)
{
	if (!ehci_batch)
		return;
	if (ehci_batch->tds) {
		const ehci_endpoint_t *ehci_ep =
		    ehci_endpoint_get(ehci_batch->usb_batch->ep);
		assert(ehci_ep);
		for (size_t i = 0; i < ehci_batch->td_count; ++i) {
			free32(ehci_batch->tds[i]);
		}
		free(ehci_batch->tds);
	}
	usb_transfer_batch_destroy(ehci_batch->usb_batch);
	free32(ehci_batch->device_buffer);
	free(ehci_batch);
}

/** Finishes usb_transfer_batch and destroys the structure.
 *
 * @param[in] uhci_batch Instance to finish and destroy.
 */
void ehci_transfer_batch_finish_dispose(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);
	assert(ehci_batch->usb_batch);
	usb_transfer_batch_finish(ehci_batch->usb_batch,
	    ehci_batch->device_buffer + ehci_batch->usb_batch->setup_size);
	ehci_transfer_batch_dispose(ehci_batch);
}

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
ehci_transfer_batch_t * ehci_transfer_batch_get(usb_transfer_batch_t *usb_batch)
{
	assert(usb_batch);

	ehci_transfer_batch_t *ehci_batch =
	    calloc(1, sizeof(ehci_transfer_batch_t));
	if (!ehci_batch) {
		usb_log_error("Failed to allocate EHCI batch data.");
		goto dispose;
	}
	link_initialize(&ehci_batch->link);
	ehci_batch->td_count =
	    (usb_batch->buffer_size + EHCI_TD_MAX_TRANSFER - 1)
	    / EHCI_TD_MAX_TRANSFER;

	/* Control transfer need Setup and Status stage */
	if (usb_batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		ehci_batch->td_count += 2;
	}

	ehci_batch->tds = calloc(ehci_batch->td_count, sizeof(td_t*));
	if (!ehci_batch->tds) {
		usb_log_error("Failed to allocate EHCI transfer descriptors.");
		goto dispose;
	}

	/* Add TD left over by the previous transfer */
	ehci_batch->qh = ehci_endpoint_get(usb_batch->ep)->qh;

	for (unsigned i = 0; i < ehci_batch->td_count; ++i) {
		ehci_batch->tds[i] = malloc32(sizeof(td_t));
		if (!ehci_batch->tds[i]) {
			usb_log_error("Failed to allocate TD %d.", i);
			goto dispose;
		}
	}


	/* Mix setup stage and data together, we have enough space */
        if (usb_batch->setup_size + usb_batch->buffer_size > 0) {
		/* Use one buffer for setup and data stage */
		ehci_batch->device_buffer =
		    malloc32(usb_batch->setup_size + usb_batch->buffer_size);
		if (!ehci_batch->device_buffer) {
			usb_log_error("Failed to allocate device buffer");
			goto dispose;
		}
		/* Copy setup data */
                memcpy(ehci_batch->device_buffer, usb_batch->setup_buffer,
		    usb_batch->setup_size);
		/* Copy generic data */
		if (usb_batch->ep->direction != USB_DIRECTION_IN)
			memcpy(
			    ehci_batch->device_buffer + usb_batch->setup_size,
			    usb_batch->buffer, usb_batch->buffer_size);
        }
	ehci_batch->usb_batch = usb_batch;

	const usb_direction_t dir = usb_transfer_batch_direction(usb_batch);
	assert(batch_setup[usb_batch->ep->transfer_type]);
	batch_setup[usb_batch->ep->transfer_type](ehci_batch, dir);

	return ehci_batch;
dispose:
	ehci_transfer_batch_dispose(ehci_batch);
	return NULL;
}

/** Check batch TDs' status.
 *
 * @param[in] ehci_batch Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs (usually there is just one). Stop with false if there is an
 * active TD. Stop with true if an error is found. Return true if the walk
 * completes with the last TD.
 */
bool ehci_transfer_batch_is_complete(const ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);
	assert(ehci_batch->usb_batch);

	usb_log_debug("Batch %p checking %zu td(s) for completion.\n",
	    ehci_batch->usb_batch, ehci_batch->td_count);

	usb_log_debug2("QH: %08x:%08x:%08x:%08x:%08x:%08x.\n",
	    ehci_batch->qh->ep_char, ehci_batch->qh->ep_cap,
	    ehci_batch->qh->status, ehci_batch->qh->current,
	    ehci_batch->qh->next, ehci_batch->qh->alternate);

	if (!qh_halted(ehci_batch->qh) && qh_transfer_pending(ehci_batch->qh))
		return false;

	/* Now we may be sure that either the ED is inactive because of errors
	 * or all transfer descriptors completed successfully */

	/* Assume all data got through */
	ehci_batch->usb_batch->transfered_size =
	    ehci_batch->usb_batch->buffer_size;

	/* Check all TDs */
	for (size_t i = 0; i < ehci_batch->td_count; ++i) {
		assert(ehci_batch->tds[i] != NULL);
		usb_log_debug("TD %zu: %08x:%08x:%08x.", i,
		    ehci_batch->tds[i]->status, ehci_batch->tds[i]->next,
		    ehci_batch->tds[i]->alternate);

		ehci_batch->usb_batch->error = td_error(ehci_batch->tds[i]);
		if (ehci_batch->usb_batch->error == EOK) {
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
			ehci_batch->usb_batch->transfered_size
			    -= td_remain_size(ehci_batch->tds[i]);
		} else {
			usb_log_debug("Batch %p found error TD(%zu):%08x.\n",
			    ehci_batch->usb_batch, i,
			    ehci_batch->tds[i]->status);
			/* Clear possible ED HALT */
			qh_clear_halt(ehci_batch->qh);
			break;
		}
	}

	assert(ehci_batch->usb_batch->transfered_size <=
	    ehci_batch->usb_batch->buffer_size);
	/* Clear TD pointers */
	ehci_batch->qh->next = LINK_POINTER_TERM;
	ehci_batch->qh->current = LINK_POINTER_TERM;

	return true;
}

/** Starts execution of the TD list
 *
 * @param[in] ehci_batch Batch structure to use
 */
void ehci_transfer_batch_commit(const ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);
	qh_set_next_td(ehci_batch->qh, ehci_batch->tds[0]);
}

/** Prepare generic control transfer
 *
 * @param[in] ehci_batch Batch structure to use.
 * @param[in] dir Communication direction
 *
 * Setup stage with toggle 0 and direction BOTH(SETUP_PID)
 * Data stage with alternating toggle and direction supplied by parameter.
 * Status stage with toggle 1 and direction supplied by parameter.
 */
static void batch_control(ehci_transfer_batch_t *ehci_batch, usb_direction_t dir)
{
	assert(ehci_batch);
	assert(ehci_batch->usb_batch);
	assert(dir == USB_DIRECTION_IN || dir == USB_DIRECTION_OUT);

	usb_log_debug2("Control QH(%"PRIxn"): %08x:%08x:%08x:%08x:%08x:%08x",
	    addr_to_phys(ehci_batch->qh),
	    ehci_batch->qh->ep_char, ehci_batch->qh->ep_cap,
	    ehci_batch->qh->status, ehci_batch->qh->current,
	    ehci_batch->qh->next, ehci_batch->qh->alternate);
	static const usb_direction_t reverse_dir[] = {
		[USB_DIRECTION_IN]  = USB_DIRECTION_OUT,
		[USB_DIRECTION_OUT] = USB_DIRECTION_IN,
	};

	int toggle = 0;
	const char* buffer = ehci_batch->device_buffer;
	const usb_direction_t data_dir = dir;
	const usb_direction_t status_dir = reverse_dir[dir];

	/* Setup stage */
	td_init(
	    ehci_batch->tds[0], ehci_batch->tds[1], USB_DIRECTION_BOTH,
	    buffer, ehci_batch->usb_batch->setup_size, toggle, false);
	usb_log_debug("Created CONTROL SETUP TD(%"PRIxn"): %08x:%08x:%08x",
	    addr_to_phys(ehci_batch->tds[0]),
	    ehci_batch->tds[0]->status, ehci_batch->tds[0]->next,
	    ehci_batch->tds[0]->alternate);
	buffer += ehci_batch->usb_batch->setup_size;

	/* Data stage */
	size_t td_current = 1;
	size_t remain_size = ehci_batch->usb_batch->buffer_size;
	while (remain_size > 0) {
		const size_t transfer_size =
		    min(remain_size, EHCI_TD_MAX_TRANSFER);
		toggle = 1 - toggle;

		td_init(ehci_batch->tds[td_current],
		    ehci_batch->tds[td_current + 1], data_dir, buffer,
		    transfer_size, toggle, false);
		usb_log_debug2("Created CONTROL DATA TD(%"PRIxn"): %08x:%08x:%08x",
		    addr_to_phys(ehci_batch->tds[td_current]),
		    ehci_batch->tds[td_current]->status,
		    ehci_batch->tds[td_current]->next,
		    ehci_batch->tds[td_current]->alternate);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < ehci_batch->td_count - 1);
		++td_current;
	}

	/* Status stage */
	assert(td_current == ehci_batch->td_count - 1);
	td_init(ehci_batch->tds[td_current], NULL, status_dir, NULL, 0, 1, true);
	usb_log_debug("Created CONTROL STATUS TD(%"PRIxn"): %08x:%08x:%08x",
	    addr_to_phys(ehci_batch->tds[td_current]),
	    ehci_batch->tds[td_current]->status,
	    ehci_batch->tds[td_current]->next,
	    ehci_batch->tds[td_current]->alternate);

	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized.\n", \
	    ehci_batch->usb_batch,
	    usb_str_transfer_type(ehci_batch->usb_batch->ep->transfer_type),
	    usb_str_direction(dir),
	    USB_TRANSFER_BATCH_ARGS(*ehci_batch->usb_batch));
}

/** Prepare generic data transfer
 *
 * @param[in] ehci_batch Batch structure to use.
 * @paramp[in] dir Communication direction.
 *
 * Direction is supplied by the associated ep and toggle is maintained by the
 * EHCI hw in ED.
 */
static void batch_data(ehci_transfer_batch_t *ehci_batch, usb_direction_t dir)
{
	assert(ehci_batch);
	assert(ehci_batch->usb_batch);
	assert(dir == USB_DIRECTION_IN || dir == USB_DIRECTION_OUT);

	usb_log_debug2("Control QH: %08x:%08x:%08x:%08x:%08x:%08x",
	    ehci_batch->qh->ep_char, ehci_batch->qh->ep_cap,
	    ehci_batch->qh->status, ehci_batch->qh->current,
	    ehci_batch->qh->next, ehci_batch->qh->alternate);

	size_t td_current = 0;
	size_t remain_size = ehci_batch->usb_batch->buffer_size;
	char *buffer = ehci_batch->device_buffer;
	while (remain_size > 0) {
		const size_t transfer_size = remain_size > EHCI_TD_MAX_TRANSFER
		    ? EHCI_TD_MAX_TRANSFER : remain_size;

		td_init(
		    ehci_batch->tds[td_current], ehci_batch->tds[td_current + 1],
		    dir, buffer, transfer_size, -1, remain_size == transfer_size);

		usb_log_debug("Created DATA TD(%"PRIxn": %08x:%08x:%08x",
		    addr_to_phys(ehci_batch->tds[td_current]),
		    ehci_batch->tds[td_current]->status,
		    ehci_batch->tds[td_current]->next,
		    ehci_batch->tds[td_current]->alternate);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < ehci_batch->td_count);
		++td_current;
	}

	usb_log_debug2(
	    "Batch %p %s %s " USB_TRANSFER_BATCH_FMT " initialized",
	    ehci_batch->usb_batch,
	    usb_str_transfer_type(ehci_batch->usb_batch->ep->transfer_type),
	    usb_str_direction(dir),
	    USB_TRANSFER_BATCH_ARGS(*ehci_batch->usb_batch));
}

/** Transfer setup table. */
static void (*const batch_setup[])(ehci_transfer_batch_t*, usb_direction_t) =
{
	[USB_TRANSFER_CONTROL] = batch_control,
	[USB_TRANSFER_BULK] = batch_data,
	[USB_TRANSFER_INTERRUPT] = batch_data,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
};
/**
 * @}
 */

