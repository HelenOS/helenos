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

#include "iface.h"
#include "uhci.h"

static int get_address(device_t *dev, devman_handle_t handle,
    usb_address_t *address)
{
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
static int interrupt_out(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_INTERRUPT, 0, USB_PID_OUT,
		data, size, callback, NULL, arg);
}
/*----------------------------------------------------------------------------*/
static int interrupt_in(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_INTERRUPT, 0, USB_PID_IN,
		data, size, NULL, callback, arg);
}
/*----------------------------------------------------------------------------*/
static int control_write_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 0, USB_PID_SETUP,
		data, size, callback, NULL, arg);
}
/*----------------------------------------------------------------------------*/
static int control_write_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 1, USB_PID_OUT,
		data, size, callback, NULL, arg);
}
/*----------------------------------------------------------------------------*/
static int control_write_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 0, USB_PID_IN,
		NULL, 0, NULL, callback, arg);
}
/*----------------------------------------------------------------------------*/
static int control_read_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 0, USB_PID_SETUP,
		data, size, callback, NULL, arg);
}
/*----------------------------------------------------------------------------*/
static int control_read_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 1, USB_PID_IN,
		data, size, NULL, callback, arg);
}
/*----------------------------------------------------------------------------*/
static int control_read_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return uhci_transfer(dev, target, USB_TRANSFER_CONTROL, 0, USB_PID_OUT,
		NULL, 0, callback, NULL, arg);
}


usbhc_iface_t uhci_iface = {
	.tell_address = get_address,

	.reserve_default_address = NULL,
	.release_default_address = NULL,
	.request_address = NULL,
	.bind_address = NULL,
	.release_address = NULL,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.control_write_setup = control_write_setup,
	.control_write_data = control_write_data,
	.control_write_status = control_write_status,

	.control_read_setup = control_read_setup,
	.control_read_data = control_read_data,
	.control_read_status = control_read_status
};
