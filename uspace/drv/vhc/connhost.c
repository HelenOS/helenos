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
	usbhc_iface_transfer_out_callback_t out_callback;
	usbhc_iface_transfer_in_callback_t in_callback;
	device_t *dev;
	void *arg;
} transfer_info_t;

static void universal_callback(void *buffer, size_t size,
    usb_transaction_outcome_t outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;

	switch (transfer->direction) {
		case USB_DIRECTION_IN:
			transfer->in_callback(transfer->dev,
			    size, outcome,
			    transfer->arg);
			break;
		case USB_DIRECTION_OUT:
			transfer->out_callback(transfer->dev,
			    outcome,
			    transfer->arg);
			break;
		default:
			assert(false && "unreachable");
			break;
	}

	free(transfer);
}

static transfer_info_t *create_transfer_info(device_t *dev,
    usb_direction_t direction, void *arg)
{
	transfer_info_t *transfer = malloc(sizeof(transfer_info_t));

	transfer->direction = direction;
	transfer->in_callback = NULL;
	transfer->out_callback = NULL;
	transfer->arg = arg;
	transfer->dev = dev;

	return transfer;
}

static int enqueue_transfer_out(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	printf(NAME ": transfer OUT [%d.%d (%s); %u]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(dev, USB_DIRECTION_OUT, arg);
	transfer->out_callback = callback;

	hc_add_transaction_to_device(false, target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}

static int enqueue_transfer_setup(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	printf(NAME ": transfer SETUP [%d.%d (%s); %u]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(dev, USB_DIRECTION_OUT, arg);
	transfer->out_callback = callback;

	hc_add_transaction_to_device(true, target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}

static int enqueue_transfer_in(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	printf(NAME ": transfer IN [%d.%d (%s); %u]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(dev, USB_DIRECTION_IN, arg);
	transfer->in_callback = callback;

	hc_add_transaction_from_device(target, buffer, size,
	    universal_callback, transfer);

	return EOK;
}


static int interrupt_out(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_out(dev, target, USB_TRANSFER_INTERRUPT,
	    data, size,
	    callback, arg);
}

static int interrupt_in(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return enqueue_transfer_in(dev, target, USB_TRANSFER_INTERRUPT,
	    data, size,
	    callback, arg);
}

static int control_write_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_setup(dev, target, USB_TRANSFER_CONTROL,
	    data, size,
	    callback, arg);
}

static int control_write_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_out(dev, target, USB_TRANSFER_CONTROL,
	    data, size,
	    callback, arg);
}

static int control_write_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return enqueue_transfer_in(dev, target, USB_TRANSFER_CONTROL,
	    NULL, 0,
	    callback, arg);
}

static int control_read_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_setup(dev, target, USB_TRANSFER_CONTROL,
	    data, size,
	    callback, arg);
}

static int control_read_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return enqueue_transfer_in(dev, target, USB_TRANSFER_CONTROL,
	    data, size,
	    callback, arg);
}

static int control_read_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_out(dev, target, USB_TRANSFER_CONTROL,
	    NULL, 0,
	    callback, arg);
}


usbhc_iface_t vhc_iface = {
	.tell_address = tell_address,

	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.request_address = request_address,
	.bind_address = bind_address,
	.release_address = release_address,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.control_write_setup = control_write_setup,
	.control_write_data = control_write_data,
	.control_write_status = control_write_status,

	.control_read_setup = control_read_setup,
	.control_read_data = control_read_data,
	.control_read_status = control_read_status
};

/**
 * @}
 */
