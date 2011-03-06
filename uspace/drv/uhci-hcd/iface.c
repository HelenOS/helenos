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
#include <ddf/driver.h>
#include <remote_usbhc.h>

#include <usb/debug.h>

#include <errno.h>

#include "iface.h"
#include "uhci.h"
#include "utils/device_keeper.h"

/*----------------------------------------------------------------------------*/
static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_log_debug("Default address request with speed %d.\n", speed);
	device_keeper_reserve_default(&hc->device_manager, speed);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_log_debug("Default address release.\n");
	device_keeper_release_default(&hc->device_manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int request_address(ddf_fun_t *fun, usb_speed_t speed,
    usb_address_t *address)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	assert(address);

	usb_log_debug("Address request with speed %d.\n", speed);
	*address = device_keeper_request(&hc->device_manager, speed);
	usb_log_debug("Address request with result: %d.\n", *address);
	if (*address <= 0)
	  return *address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int bind_address(
  ddf_fun_t *fun, usb_address_t address, devman_handle_t handle)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_log_debug("Address bind %d-%d.\n", address, handle);
	device_keeper_bind(&hc->device_manager, address, handle);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int release_address(ddf_fun_t *fun, usb_address_t address)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_log_debug("Address release %d.\n", address);
	device_keeper_release(&hc->device_manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int interrupt_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);

	usb_log_debug("Interrupt OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_INTERRUPT,
	    max_packet_size, speed, data, size, NULL, 0, NULL, callback, arg,
	    &hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_interrupt_out(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int interrupt_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);
	usb_log_debug("Interrupt IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_INTERRUPT,
	    max_packet_size, speed, data, size, NULL, 0, callback, NULL, arg,
			&hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_interrupt_in(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int bulk_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);

	usb_log_debug("Bulk OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_BULK,
	    max_packet_size, speed, data, size, NULL, 0, NULL, callback, arg,
	    &hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_bulk_out(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int bulk_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);
	usb_log_debug("Bulk IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_BULK,
	    max_packet_size, speed, data, size, NULL, 0, callback, NULL, arg,
	    &hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_bulk_in(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_write(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);
	usb_log_debug("Control WRITE %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	if (setup_size != 8)
		return EINVAL;

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, setup_data, setup_size,
	    NULL, callback, arg, &hc->device_manager);
	if (!batch)
		return ENOMEM;
	device_keeper_reset_if_need(&hc->device_manager, target, setup_data);
	batch_control_write(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int control_read(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_t *hc = fun_to_uhci(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);

	usb_log_debug("Control READ %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);
	batch_t *batch = batch_get(fun, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, setup_data, setup_size, callback,
	    NULL, arg, &hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_control_read(batch);
	return EOK;
}
/*----------------------------------------------------------------------------*/
usbhc_iface_t uhci_iface = {
	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.request_address = request_address,
	.bind_address = bind_address,
	.release_address = release_address,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.bulk_in = bulk_in,
	.bulk_out = bulk_out,

	.control_read = control_read,
	.control_write = control_write,
};
/**
 * @}
 */
