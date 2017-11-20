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
#include <str_error.h>

#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>

#include "ehci_batch.h"
#include "ehci_bus.h"

/* The buffer pointer list in the qTD is long enough to support a maximum
 * transfer size of 20K bytes. This case occurs when all five buffer pointers
 * are used and the first offset is zero. A qTD handles a 16Kbyte buffer
 * with any starting buffer alignment. EHCI specs p. 87 (pdf p. 97) */
#define EHCI_TD_MAX_TRANSFER   (16 * 1024)

static void (*const batch_setup[])(ehci_transfer_batch_t*);

/** Safely destructs ehci_transfer_batch_t structure
 *
 * @param[in] ehci_batch Instance to destroy.
 */
void ehci_transfer_batch_destroy(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);
	if (ehci_batch->tds) {
		for (size_t i = 0; i < ehci_batch->td_count; ++i) {
			free32(ehci_batch->tds[i]);
		}
		free(ehci_batch->tds);
	}
	free32(ehci_batch->device_buffer);
	free(ehci_batch);
	usb_log_debug2("Batch(%p): disposed", ehci_batch);
}

/** Allocate memory and initialize internal data structure.
 *
 * @param[in] usb_batch Pointer to generic USB batch structure.
 * @return Valid pointer if all structures were successfully created,
 * NULL otherwise.
 *
 */
ehci_transfer_batch_t * ehci_transfer_batch_create(endpoint_t *ep)
{
	assert(ep);

	ehci_transfer_batch_t *ehci_batch = calloc(1, sizeof(ehci_transfer_batch_t));
	if (!ehci_batch) {
		usb_log_error("Failed to allocate EHCI batch data.");
		return NULL;
	}

	usb_transfer_batch_init(&ehci_batch->base, ep);
	link_initialize(&ehci_batch->link);

	return ehci_batch;
}

/** Prepares a batch to be sent.
 *
 * Determines the number of needed transfer descriptors (TDs).
 * Prepares a transport buffer (that is accessible by the hardware).
 * Initializes parameters needed for the transfer and callback.
 */
int ehci_transfer_batch_prepare(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);

	const size_t setup_size = (ehci_batch->base.ep->transfer_type == USB_TRANSFER_CONTROL)
		? USB_SETUP_PACKET_SIZE
		: 0;

	const size_t size = ehci_batch->base.buffer_size;

	/* Mix setup stage and data together, we have enough space */
        if (size + setup_size > 0) {
		/* Use one buffer for setup and data stage */
		ehci_batch->device_buffer = malloc32(size + setup_size);
		if (!ehci_batch->device_buffer) {
			usb_log_error("Batch %p: Failed to allocate device "
			    "buffer", ehci_batch);
			return ENOMEM;
		}
		/* Copy setup data */
                memcpy(ehci_batch->device_buffer, ehci_batch->base.setup.buffer,
		    setup_size);
		/* Copy generic data */
		if (ehci_batch->base.dir != USB_DIRECTION_IN)
			memcpy(ehci_batch->device_buffer + setup_size,
			    ehci_batch->base.buffer, ehci_batch->base.buffer_size);
        }

	/* Add TD left over by the previous transfer */
	ehci_batch->qh = ehci_endpoint_get(ehci_batch->base.ep)->qh;

	/* Determine number of TDs needed */
	ehci_batch->td_count = (size + EHCI_TD_MAX_TRANSFER - 1)
		/ EHCI_TD_MAX_TRANSFER;

	/* Control transfer need Setup and Status stage */
	if (ehci_batch->base.ep->transfer_type == USB_TRANSFER_CONTROL) {
		ehci_batch->td_count += 2;
	}

	ehci_batch->tds = calloc(ehci_batch->td_count, sizeof(td_t*));
	if (!ehci_batch->tds) {
		usb_log_error("Batch %p: Failed to allocate EHCI transfer "
		    "descriptors.", ehci_batch);
		return ENOMEM;
	}

	for (unsigned i = 0; i < ehci_batch->td_count; ++i) {
		ehci_batch->tds[i] = malloc32(sizeof(td_t));
		if (!ehci_batch->tds[i]) {
			usb_log_error("Batch %p: Failed to allocate TD %d.",
			    ehci_batch, i);
			return ENOMEM;
		}
		memset(ehci_batch->tds[i], 0, sizeof(td_t));
	}

	assert(batch_setup[ehci_batch->base.ep->transfer_type]);
	batch_setup[ehci_batch->base.ep->transfer_type](ehci_batch);

	usb_log_debug("Batch %p %s " USB_TRANSFER_BATCH_FMT " initialized.\n",
	    ehci_batch, usb_str_direction(ehci_batch->base.dir),
	    USB_TRANSFER_BATCH_ARGS(ehci_batch->base));

	return EOK;
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
bool ehci_transfer_batch_check_completed(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);

	usb_log_debug("Batch %p: checking %zu td(s) for completion.\n",
	    ehci_batch, ehci_batch->td_count);

	usb_log_debug2("Batch %p: QH: %08x:%08x:%08x:%08x:%08x:%08x.\n",
	    ehci_batch,
	    ehci_batch->qh->ep_char, ehci_batch->qh->ep_cap,
	    ehci_batch->qh->status, ehci_batch->qh->current,
	    ehci_batch->qh->next, ehci_batch->qh->alternate);

	if (!qh_halted(ehci_batch->qh) && (qh_transfer_pending(ehci_batch->qh)
	    || qh_transfer_active(ehci_batch->qh)))
		return false;

	/* Now we may be sure that either the ED is inactive because of errors
	 * or all transfer descriptors completed successfully */

	/* Assume all data got through */
	ehci_batch->base.transfered_size = ehci_batch->base.buffer_size;

	/* Check all TDs */
	for (size_t i = 0; i < ehci_batch->td_count; ++i) {
		assert(ehci_batch->tds[i] != NULL);
		usb_log_debug("Batch %p: TD %zu: %08x:%08x:%08x.",
		    ehci_batch, i,
		    ehci_batch->tds[i]->status, ehci_batch->tds[i]->next,
		    ehci_batch->tds[i]->alternate);

		ehci_batch->base.error = td_error(ehci_batch->tds[i]);
		if (ehci_batch->base.error == EOK) {
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
			ehci_batch->base.transfered_size
			    -= td_remain_size(ehci_batch->tds[i]);
		} else {
			usb_log_debug("Batch %p found error TD(%zu):%08x (%d).",
			    ehci_batch, i,
			    ehci_batch->tds[i]->status,
			    ehci_batch->base.error);
			/* Clear possible ED HALT */
			qh_clear_halt(ehci_batch->qh);
			break;
		}
	}

	assert(ehci_batch->base.transfered_size <= ehci_batch->base.buffer_size);

	const size_t setup_size = (ehci_batch->base.ep->transfer_type == USB_TRANSFER_CONTROL)
		? USB_SETUP_PACKET_SIZE
		: 0;

	if (ehci_batch->base.dir == USB_DIRECTION_IN)
		memcpy(ehci_batch->base.buffer,
		    ehci_batch->device_buffer + setup_size,
		    ehci_batch->base.transfered_size);

	/* Clear TD pointers */
	ehci_batch->qh->next = LINK_POINTER_TERM;
	ehci_batch->qh->current = LINK_POINTER_TERM;
	usb_log_debug("Batch %p complete: %s", ehci_batch,
	    str_error(ehci_batch->base.error));

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
 *
 * Setup stage with toggle 0 and direction BOTH(SETUP_PID)
 * Data stage with alternating toggle and direction
 * Status stage with toggle 1 and direction
 */
static void batch_control(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);

	usb_direction_t dir = ehci_batch->base.dir;
	assert(dir == USB_DIRECTION_IN || dir == USB_DIRECTION_OUT);

	usb_log_debug2("Batch %p: Control QH(%"PRIxn"): "
	    "%08x:%08x:%08x:%08x:%08x:%08x", ehci_batch,
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
	td_init(ehci_batch->tds[0], ehci_batch->tds[1], USB_DIRECTION_BOTH,
	    buffer, USB_SETUP_PACKET_SIZE, toggle, false);
	usb_log_debug2("Batch %p: Created CONTROL SETUP TD(%"PRIxn"): "
	    "%08x:%08x:%08x", ehci_batch,
	    addr_to_phys(ehci_batch->tds[0]),
	    ehci_batch->tds[0]->status, ehci_batch->tds[0]->next,
	    ehci_batch->tds[0]->alternate);
	buffer += USB_SETUP_PACKET_SIZE;

	/* Data stage */
	size_t td_current = 1;
	size_t remain_size = ehci_batch->base.buffer_size;
	while (remain_size > 0) {
		const size_t transfer_size =
		    min(remain_size, EHCI_TD_MAX_TRANSFER);
		toggle = 1 - toggle;

		td_init(ehci_batch->tds[td_current],
		    ehci_batch->tds[td_current + 1], data_dir, buffer,
		    transfer_size, toggle, false);
		usb_log_debug2("Batch %p: Created CONTROL DATA TD(%"PRIxn"): "
		    "%08x:%08x:%08x", ehci_batch,
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
	usb_log_debug2("Batch %p: Created CONTROL STATUS TD(%"PRIxn"): "
	    "%08x:%08x:%08x", ehci_batch,
	    addr_to_phys(ehci_batch->tds[td_current]),
	    ehci_batch->tds[td_current]->status,
	    ehci_batch->tds[td_current]->next,
	    ehci_batch->tds[td_current]->alternate);
}

/** Prepare generic data transfer
 *
 * @param[in] ehci_batch Batch structure to use.
 * @paramp[in] dir Communication direction.
 *
 * Direction is supplied by the associated ep and toggle is maintained by the
 * EHCI hw in ED.
 */
static void batch_data(ehci_transfer_batch_t *ehci_batch)
{
	assert(ehci_batch);

	usb_log_debug2("Batch %p: Data QH(%"PRIxn"): "
	    "%08x:%08x:%08x:%08x:%08x:%08x", ehci_batch,
	    addr_to_phys(ehci_batch->qh),
	    ehci_batch->qh->ep_char, ehci_batch->qh->ep_cap,
	    ehci_batch->qh->status, ehci_batch->qh->current,
	    ehci_batch->qh->next, ehci_batch->qh->alternate);

	size_t td_current = 0;
	size_t remain_size = ehci_batch->base.buffer_size;
	char *buffer = ehci_batch->device_buffer;
	while (remain_size > 0) {
		const size_t transfer_size = remain_size > EHCI_TD_MAX_TRANSFER
		    ? EHCI_TD_MAX_TRANSFER : remain_size;

		const bool last = (remain_size == transfer_size);
		td_init(
		    ehci_batch->tds[td_current],
		    last ? NULL : ehci_batch->tds[td_current + 1],
		    ehci_batch->base.dir, buffer, transfer_size, -1, last);

		usb_log_debug2("Batch %p: DATA TD(%"PRIxn": %08x:%08x:%08x",
		    ehci_batch,
		    addr_to_phys(ehci_batch->tds[td_current]),
		    ehci_batch->tds[td_current]->status,
		    ehci_batch->tds[td_current]->next,
		    ehci_batch->tds[td_current]->alternate);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < ehci_batch->td_count);
		++td_current;
	}
}

/** Transfer setup table. */
static void (*const batch_setup[])(ehci_transfer_batch_t*) =
{
	[USB_TRANSFER_CONTROL] = batch_control,
	[USB_TRANSFER_BULK] = batch_data,
	[USB_TRANSFER_INTERRUPT] = batch_data,
	[USB_TRANSFER_ISOCHRONOUS] = NULL,
};
/**
 * @}
 */

