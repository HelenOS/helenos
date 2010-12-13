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
#include <usb/hcdhubd.h>
#include <errno.h>

#include "uhci.h"

static int enqueue_transfer_out(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	printf(NAME ": transfer OUT [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	return ENOTSUP;
}

static int enqueue_transfer_setup(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	printf(NAME ": transfer SETUP [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	return ENOTSUP;
}

static int enqueue_transfer_in(device_t *dev,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	printf(NAME ": transfer IN [%d.%d (%s); %zu]\n",
	    target.address, target.endpoint,
	    usb_str_transfer_type(transfer_type),
	    size);

	return ENOTSUP;
}


static int get_address(device_t *dev, devman_handle_t handle,
    usb_address_t *address)
{
	return ENOTSUP;
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


usbhc_iface_t uhci_iface = {
	.tell_address = get_address,
	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,
	.control_write_setup = control_write_setup,
	.control_write_data = control_write_data,
	.control_write_status = control_write_status,
	.control_read_setup = control_read_setup,
	.control_read_data = control_read_data,
	.control_read_status = control_read_status
};
