/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
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
 * @brief UHCI driver
 */
#include <driver.h>
#include <remote_usbhc.h>

#include <usb/debug.h>

#include <errno.h>

#include "iface.h"
#include "uhci.h"

static int get_address(device_t *dev, devman_handle_t handle,
    usb_address_t *address)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	*address = usb_address_keeping_find(&hc->address_manager, handle);
	if (*address <= 0)
	  return *address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int reserve_default_address(device_t *dev)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	usb_address_keeping_reserve_default(&hc->address_manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int release_default_address(device_t *dev)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	usb_address_keeping_release_default(&hc->address_manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int request_address(device_t *dev, usb_address_t *address)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	*address = usb_address_keeping_request(&hc->address_manager);
	if (*address <= 0)
	  return *address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int bind_address(
  device_t *dev, usb_address_t address, devman_handle_t handle)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	usb_address_keeping_devman_bind(&hc->address_manager, address, handle);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int release_address(device_t *dev, usb_address_t address)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
	assert(hc);
	usb_address_keeping_release_default(&hc->address_manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int interrupt_out(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	size_t max_packet_size = 8;
	dev_speed_t speed = FULL_SPEED;

	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_INTERRUPT,
	    max_packet_size, speed, data, size, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_read_data_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int interrupt_in(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	size_t max_packet_size = 8;
	dev_speed_t speed = FULL_SPEED;

	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_INTERRUPT,
	    max_packet_size, speed, data, size, callback, NULL, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_read_data_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_write(device_t *dev, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	size_t max_packet_size = 8;
	dev_speed_t speed = FULL_SPEED;

	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_write(tracker, setup_data, setup_size);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_read(device_t *dev, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	size_t max_packet_size = 8;
	dev_speed_t speed = FULL_SPEED;

	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, callback, NULL, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_read(tracker, setup_data, setup_size);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_write_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_log_warning("Using deprecated API control write setup.\n");
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    8, FULL_SPEED, data, size, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_setup_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_write_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    size, FULL_SPEED, data, size, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_write_data_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_write_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    0, FULL_SPEED, NULL, 0, callback, NULL, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_write_status_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_read_setup(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	usb_log_warning("Using deprecated API control read setup.\n");
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    8, FULL_SPEED, data, size, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_setup_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_read_data(device_t *dev, usb_target_t target,
    void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    size, FULL_SPEED, data, size, callback, NULL, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_read_data_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_read_status(device_t *dev, usb_target_t target,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	tracker_t *tracker = tracker_get(dev, target, USB_TRANSFER_CONTROL,
	    0, FULL_SPEED, NULL, 0, NULL, callback, arg);
	if (!tracker)
		return ENOMEM;
	tracker_control_read_status_old(tracker);
	return EOK;
}
/*----------------------------------------------------------------------------*/
usbhc_iface_t uhci_iface = {
	.tell_address = get_address,

	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.request_address = request_address,
	.bind_address = bind_address,
	.release_address = release_address,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.control_read = control_read,
	.control_write = control_write,

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
