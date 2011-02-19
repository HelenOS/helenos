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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief Connection handling of calls from host (implementation).
 */
#include <assert.h>
#include <errno.h>
#include <usb/usb.h>
#include <usb/addrkeep.h>

#include "vhcd.h"
#include "conn.h"
#include "hc.h"


typedef struct {
	usb_direction_t direction;
	usbhc_iface_transfer_out_callback_t out_callback;
	usbhc_iface_transfer_in_callback_t in_callback;
	device_t *dev;
	size_t reported_size;
	void *arg;
} transfer_info_t;

typedef struct {
	usb_direction_t direction;
	usb_target_t target;
	usbhc_iface_transfer_out_callback_t out_callback;
	usbhc_iface_transfer_in_callback_t in_callback;
	device_t *dev;
	void *arg;
	void *data_buffer;
	size_t data_buffer_size;
} control_transfer_info_t;

static void universal_callback(void *buffer, size_t size,
    int outcome, void *arg)
{
	transfer_info_t *transfer = (transfer_info_t *) arg;

	if (transfer->reported_size != (size_t) -1) {
		size = transfer->reported_size;
	}

	switch (transfer->direction) {
		case USB_DIRECTION_IN:
			transfer->in_callback(transfer->dev,
			    outcome, size,
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
	transfer->reported_size = (size_t) -1;

	return transfer;
}

static void control_abort_prematurely(control_transfer_info_t *transfer,
    size_t size, int outcome)
{
	switch (transfer->direction) {
		case USB_DIRECTION_IN:
			transfer->in_callback(transfer->dev,
			    outcome, size,
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
}

static void control_callback_two(void *buffer, size_t size,
    int outcome, void *arg)
{
	control_transfer_info_t *ctrl_transfer = (control_transfer_info_t *) arg;

	if (outcome != EOK) {
		control_abort_prematurely(ctrl_transfer, outcome, size);
		free(ctrl_transfer);
		return;
	}

	transfer_info_t *transfer  = create_transfer_info(ctrl_transfer->dev,
	    ctrl_transfer->direction, ctrl_transfer->arg);
	transfer->out_callback = ctrl_transfer->out_callback;
	transfer->in_callback = ctrl_transfer->in_callback;
	transfer->reported_size = size;

	switch (ctrl_transfer->direction) {
		case USB_DIRECTION_IN:
			hc_add_transaction_to_device(false, ctrl_transfer->target,
			    USB_TRANSFER_CONTROL,
			    NULL, 0,
			    universal_callback, transfer);
			break;
		case USB_DIRECTION_OUT:
			hc_add_transaction_from_device(ctrl_transfer->target,
			    USB_TRANSFER_CONTROL,
			    NULL, 0,
			    universal_callback, transfer);
			break;
		default:
			assert(false && "unreachable");
			break;
	}

	free(ctrl_transfer);
}

static void control_callback_one(void *buffer, size_t size,
    int outcome, void *arg)
{
	control_transfer_info_t *transfer = (control_transfer_info_t *) arg;

	if (outcome != EOK) {
		control_abort_prematurely(transfer, outcome, size);
		free(transfer);
		return;
	}

	switch (transfer->direction) {
		case USB_DIRECTION_IN:
			hc_add_transaction_from_device(transfer->target,
			    USB_TRANSFER_CONTROL,
			    transfer->data_buffer, transfer->data_buffer_size,
			    control_callback_two, transfer);
			break;
		case USB_DIRECTION_OUT:
			hc_add_transaction_to_device(false, transfer->target,
			    USB_TRANSFER_CONTROL,
			    transfer->data_buffer, transfer->data_buffer_size,
			    control_callback_two, transfer);
			break;
		default:
			assert(false && "unreachable");
			break;
	}
}

static control_transfer_info_t *create_control_transfer_info(device_t *dev,
    usb_direction_t direction, usb_target_t target,
    void *data_buffer, size_t data_buffer_size,
    void *arg)
{
	control_transfer_info_t *transfer
	    = malloc(sizeof(control_transfer_info_t));

	transfer->direction = direction;
	transfer->target = target;
	transfer->in_callback = NULL;
	transfer->out_callback = NULL;
	transfer->arg = arg;
	transfer->dev = dev;
	transfer->data_buffer = data_buffer;
	transfer->data_buffer_size = data_buffer_size;

	return transfer;
}

static int enqueue_transfer_out(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_log_debug2("Transfer OUT [%d.%d (%s); %zu].\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(dev, USB_DIRECTION_OUT, arg);
	transfer->out_callback = callback;

	hc_add_transaction_to_device(false, target, transfer_type, buffer, size,
	    universal_callback, transfer);

	return EOK;
}

static int enqueue_transfer_in(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	usb_log_debug2("Transfer IN [%d.%d (%s); %zu].\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	transfer_info_t *transfer
	    = create_transfer_info(dev, USB_DIRECTION_IN, arg);
	transfer->in_callback = callback;

	hc_add_transaction_from_device(target, transfer_type, buffer, size,
	    universal_callback, transfer);

	return EOK;
}


static int interrupt_out(device_t *dev, usb_target_t target,
    size_t max_packet_size,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return enqueue_transfer_out(dev, target, USB_TRANSFER_INTERRUPT,
	    data, size,
	    callback, arg);
}

static int interrupt_in(device_t *dev, usb_target_t target,
    size_t max_packet_size,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return enqueue_transfer_in(dev, target, USB_TRANSFER_INTERRUPT,
	    data, size,
	    callback, arg);
}

static int control_write(device_t *dev, usb_target_t target,
    size_t max_packet_size,
    void *setup_packet, size_t setup_packet_size,
    void *data, size_t data_size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	control_transfer_info_t *transfer
	    = create_control_transfer_info(dev, USB_DIRECTION_OUT, target,
	    data, data_size, arg);
	transfer->out_callback = callback;

	hc_add_transaction_to_device(true, target, USB_TRANSFER_CONTROL,
	    setup_packet, setup_packet_size,
	    control_callback_one, transfer);

	return EOK;
}

static int control_read(device_t *dev, usb_target_t target,
    size_t max_packet_size,
    void *setup_packet, size_t setup_packet_size,
    void *data, size_t data_size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	control_transfer_info_t *transfer
	    = create_control_transfer_info(dev, USB_DIRECTION_IN, target,
	    data, data_size, arg);
	transfer->in_callback = callback;

	hc_add_transaction_to_device(true, target, USB_TRANSFER_CONTROL,
	    setup_packet, setup_packet_size,
	    control_callback_one, transfer);

	return EOK;
}

static usb_address_keeping_t addresses;


static int reserve_default_address(device_t *dev, usb_speed_t ignored)
{
	usb_address_keeping_reserve_default(&addresses);
	return EOK;
}

static int release_default_address(device_t *dev)
{
	usb_address_keeping_release_default(&addresses);
	return EOK;
}

static int request_address(device_t *dev, usb_speed_t ignored,
    usb_address_t *address)
{
	usb_address_t addr = usb_address_keeping_request(&addresses);
	if (addr < 0) {
		return (int)addr;
	}

	*address = addr;
	return EOK;
}

static int release_address(device_t *dev, usb_address_t address)
{
	return usb_address_keeping_release(&addresses, address);
}

static int bind_address(device_t *dev, usb_address_t address,
    devman_handle_t handle)
{
	usb_address_keeping_devman_bind(&addresses, address, handle);
	return EOK;
}

static int tell_address(device_t *dev, devman_handle_t handle,
    usb_address_t *address)
{
	usb_address_t addr = usb_address_keeping_find(&addresses, handle);
	if (addr < 0) {
		return addr;
	}

	*address = addr;
	return EOK;
}

void address_init(void)
{
	usb_address_keeping_init(&addresses, 50);
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

	.control_write = control_write,
	.control_read = control_read
};

/**
 * @}
 */
