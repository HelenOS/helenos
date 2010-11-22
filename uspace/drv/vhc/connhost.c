/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Connection handling of calls from host (implementation).
 */
#include <assert.h>
#include <errno.h>
#include <usb/usb.h>

#include "vhcd.h"
#include "conn.h"
#include "hc.h"

typedef struct {
	usb_direction_t direction;
	usb_hcd_transfer_callback_out_t out_callback;
	usb_hcd_transfer_callback_in_t in_callback;
	usb_hc_device_t *hc;
	void *arg;
} transfer_info_t;

static void universal_callback(void *buffer, size_t size,
    usb_transaction_outcome_t outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;

	switch (transfer->direction) {
		case USB_DIRECTION_IN:
			transfer->in_callback(transfer->hc,
			    size, outcome,
			    transfer->arg);
			break;
		case USB_DIRECTION_OUT:
			transfer->out_callback(transfer->hc,
			    outcome,
			    transfer->arg);
			break;
		default:
			assert(false && "unreachable");
			break;
	}

	free(transfer);
}

static transfer_info_t *create_transfer_info(usb_hc_device_t *hc,
    usb_direction_t direction, void *arg)
{
	transfer_info_t *transfer = malloc(sizeof(transfer_info_t));

	transfer->direction = direction;
	transfer->in_callback = NULL;
	transfer->out_callback = NULL;
	transfer->arg = arg;
	transfer->hc = hc;

	return transfer;
}

static int enqueue_transfer_out(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_out_t callback, void *arg)
{
	printf(NAME ": transfer OUT [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(hc, USB_DIRECTION_OUT, arg);
	transfer->out_callback = callback;

	usb_target_t target = {
		.address = dev->address,
		.endpoint = endpoint->endpoint
	};

	hc_add_transaction_to_device(false, target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}

static int enqueue_transfer_setup(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_out_t callback, void *arg)
{
	printf(NAME ": transfer SETUP [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(hc, USB_DIRECTION_OUT, arg);
	transfer->out_callback = callback;

	usb_target_t target = {
		.address = dev->address,
		.endpoint = endpoint->endpoint
	};

	hc_add_transaction_to_device(true, target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}

static int enqueue_transfer_in(usb_hc_device_t *hc,
    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *endpoint,
    void *buffer, size_t size,
    usb_hcd_transfer_callback_in_t callback, void *arg)
{
	printf(NAME ": transfer IN [%d.%d (%s); %u]\n",
	    dev->address, endpoint->endpoint,
	    usb_str_transfer_type(endpoint->transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(hc, USB_DIRECTION_IN, arg);
	transfer->in_callback = callback;

	usb_target_t target = {
		.address = dev->address,
		.endpoint = endpoint->endpoint
	};

	hc_add_transaction_from_device(target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}


usb_hcd_transfer_ops_t vhc_transfer_ops = {
	.transfer_out = enqueue_transfer_out,
	.transfer_in = enqueue_transfer_in,
	.transfer_setup = enqueue_transfer_setup
};

/**
 * @}
 */
