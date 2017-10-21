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

#include <usb/host/usb_transfer_batch.h>
#include <usb/host/endpoint.h>
#include <usb/host/bus.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>


/** Create a batch on given endpoint.
 */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *ep)
{
	assert(ep);
	assert(ep->bus);

	usb_transfer_batch_t *batch;
	if (ep->bus->ops.create_batch)
		batch = ep->bus->ops.create_batch(ep->bus, ep);
	else
		batch = malloc(sizeof(usb_transfer_batch_t));

	return batch;
}

/** Initialize given batch structure.
 */
void usb_transfer_batch_init(usb_transfer_batch_t *batch, endpoint_t *ep)
{
	memset(batch, 0, sizeof(*batch));

	batch->ep = ep;

	endpoint_use(ep);
}

/** Call the handler of the batch.
 *
 * @param[in] batch Batch structure to use.
 */
static int batch_complete(usb_transfer_batch_t *batch)
{
	assert(batch);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " completed.\n",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch));

	if (batch->error == EOK && batch->toggle_reset_mode != RESET_NONE) {
		usb_log_debug2("Reseting %s due to transaction of %d:%d.\n",
		    batch->toggle_reset_mode == RESET_ALL ? "all EPs toggle" : "EP toggle",
		    batch->ep->target.address, batch->ep->target.endpoint);
		bus_reset_toggle(batch->ep->bus,
		    batch->ep->target, batch->toggle_reset_mode == RESET_ALL);
	}

	return batch->on_complete
		? batch->on_complete(batch)
		: EOK;
}

/** Destroy the batch.
 *
 * @param[in] batch Batch structure to use.
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);
	assert(batch->ep->bus);

	usb_log_debug2("batch %p " USB_TRANSFER_BATCH_FMT " disposing.\n",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch));

	bus_t *bus = batch->ep->bus;
	if (bus->ops.destroy_batch)
		bus->ops.destroy_batch(batch);
	else
		free(batch);

	endpoint_release(batch->ep);
}

/** Finish a transfer batch: call handler, destroy batch, release endpoint.
 *
 * Call only after the batch have been scheduled && completed!
 *
 * @param[in] batch Batch structure to use.
 */
void usb_transfer_batch_finish(usb_transfer_batch_t *batch)
{
	if (!batch_complete(batch))
		usb_log_warning("failed to complete batch %p!", batch);

	usb_transfer_batch_destroy(batch);
}


struct old_handler_wrapper_data {
	usbhc_iface_transfer_in_callback_t in_callback;
	usbhc_iface_transfer_out_callback_t out_callback;
	void *arg;
};

static int old_handler_wrapper(usb_transfer_batch_t *batch)
{
	struct old_handler_wrapper_data *data = batch->on_complete_data;

	assert(data);

	if (data->out_callback)
		data->out_callback(batch->error, data->arg);

	if (data->in_callback)
		data->in_callback(batch->error, batch->transfered_size, data->arg);

	free(data);
	return EOK;
}

void usb_transfer_batch_set_old_handlers(usb_transfer_batch_t *batch,
	usbhc_iface_transfer_in_callback_t in_callback,
	usbhc_iface_transfer_out_callback_t out_callback,
	void *arg)
{
	struct old_handler_wrapper_data *data = malloc(sizeof(*data));

	data->in_callback = in_callback;
	data->out_callback = out_callback;
	data->arg = arg;

	batch->on_complete = old_handler_wrapper;
	batch->on_complete_data = data;
}
/**
 * @}
 */
