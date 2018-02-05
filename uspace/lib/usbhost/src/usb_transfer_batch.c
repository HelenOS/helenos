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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * USB transfer transaction structures (implementation).
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str_error.h>
#include <usb/debug.h>

#include "endpoint.h"
#include "bus.h"

#include "usb_transfer_batch.h"

/**
 * Create a batch on a given endpoint.
 *
 * If the bus callback is not defined, it just creates a default batch.
 */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *ep)
{
	assert(ep);

	bus_t *bus = endpoint_get_bus(ep);

	if (!bus->ops->batch_create) {
		usb_transfer_batch_t *batch = calloc(1, sizeof(usb_transfer_batch_t));
		if (!batch)
			return NULL;
		usb_transfer_batch_init(batch, ep);
		return batch;
	}

	return bus->ops->batch_create(ep);
}

/**
 * Initialize given batch structure.
 */
void usb_transfer_batch_init(usb_transfer_batch_t *batch, endpoint_t *ep)
{
	assert(ep);
	/* Batch reference */
	endpoint_add_ref(ep);
	batch->ep = ep;
}

/**
 * Destroy the batch. If there's no bus callback, just free it.
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	bus_t *bus = endpoint_get_bus(batch->ep);
	endpoint_t *ep = batch->ep;

	if (bus->ops) {
		usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " destroying.",
		    batch, USB_TRANSFER_BATCH_ARGS(*batch));
		bus->ops->batch_destroy(batch);
	}
	else {
		usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " disposing.",
		    batch, USB_TRANSFER_BATCH_ARGS(*batch));
		free(batch);
	}

	/* Batch reference */
	endpoint_del_ref(ep);
}

/**
 * Prepare a DMA buffer according to endpoint policy.
 *
 * If the buffer is suitable to be used directly, it is. Otherwise, a bounce
 * buffer is created.
 */
errno_t usb_transfer_batch_prepare_buffer(
    usb_transfer_batch_t *batch, char *buf)
{
	const dma_policy_t policy = batch->ep->transfer_buffer_policy;

	/* Empty transfers do not need a buffer */
	if (batch->buffer_size == 0)
		return EOK;

	if (dma_buffer_check_policy(buf, batch->buffer_size, policy)) {
		/* Mark this case with invalid address */
		batch->original_buffer = NULL;

		/* Fill the buffer with virtual address and lock it for DMA */
		return dma_buffer_lock(&batch->dma_buffer, buf, batch->buffer_size);
	}
	else {
		usb_log_debug("Batch(%p): Buffer cannot be used directly, "
		    "falling back to bounce buffer!", batch);
		const errno_t err = dma_buffer_alloc_policy(&batch->dma_buffer,
		    batch->buffer_size, policy);

		/* Copy the data out */
		if (!err && batch->dir == USB_DIRECTION_OUT)
			memcpy(batch->dma_buffer.virt, buf, batch->buffer_size);

		/* Store the buffer to store the data back before finishing */
		batch->original_buffer = buf;

		return err;
	}
}

/**
 * Finish a transfer batch: call handler, destroy batch, release endpoint.
 *
 * Call only after the batch have been scheduled && completed!
 */
void usb_transfer_batch_finish(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " finishing.",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch));

	if (batch->error == EOK && batch->buffer_size > 0) {
		if (batch->original_buffer == NULL) {
			/* Unlock the buffer for DMA */
			dma_buffer_unlock(&batch->dma_buffer,
			    batch->buffer_size);
		}
		else {
			/* We we're forced to use bounce buffer, copy it back */
			if (batch->dir == USB_DIRECTION_IN)
				memcpy(batch->original_buffer,
				    batch->dma_buffer.virt,
				    batch->transferred_size);

			dma_buffer_free(&batch->dma_buffer);
		}
	}

	if (batch->on_complete) {
		const int err = batch->on_complete(batch->on_complete_data, batch->error, batch->transferred_size);
		if (err)
			usb_log_warning("Batch %p failed to complete: %s",
			    batch, str_error(err));
	}

	usb_transfer_batch_destroy(batch);
}

/**
 * @}
 */
