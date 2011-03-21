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
/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver hc interface implementation
 */
#include <ddf/driver.h>
#include <remote_usbhc.h>

#include <usb/debug.h>

#include <errno.h>

#include "iface.h"
#include "uhci_hc.h"

/** Reserve default address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] speed Speed to associate with the new default address.
 * @return Error code.
 */
/*----------------------------------------------------------------------------*/
static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_log_debug("Default address request with speed %d.\n", speed);
	device_keeper_reserve_default(&hc->device_manager, speed);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Release default address interface function
 *
 * @param[in] fun DDF function that was called.
 * @return Error code.
 */
static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_log_debug("Default address release.\n");
	device_keeper_release_default(&hc->device_manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Request address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] speed Speed to associate with the new default address.
 * @param[out] address Place to write a new address.
 * @return Error code.
 */
static int request_address(ddf_fun_t *fun, usb_speed_t speed,
    usb_address_t *address)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
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
/** Bind address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address Address of the device
 * @param[in] handle Devman handle of the device driver.
 * @return Error code.
 */
static int bind_address(
  ddf_fun_t *fun, usb_address_t address, devman_handle_t handle)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_log_debug("Address bind %d-%d.\n", address, handle);
	device_keeper_bind(&hc->device_manager, address, handle);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Release address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address USB address to be released.
 * @return Error code.
 */
static int release_address(ddf_fun_t *fun, usb_address_t address)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_log_debug("Address release %d.\n", address);
	device_keeper_release(&hc->device_manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Interrupt out transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int interrupt_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
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
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Interrupt in transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts
 * @param[out] data Data destination.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int interrupt_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
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
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Bulk out transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int bulk_out(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
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
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Bulk in transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts
 * @param[out] data Data destination.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int bulk_in(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
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
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Control write transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts.
 * @param[in] setup_data Data to send with SETUP packet.
 * @param[in] setup_size Size of data to send with SETUP packet (should be 8B).
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion.
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int control_write(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);
	usb_log_debug("Control WRITE (%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);

	if (setup_size != 8)
		return EINVAL;

	batch_t *batch = batch_get(fun, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, setup_data, setup_size,
	    NULL, callback, arg, &hc->device_manager);
	if (!batch)
		return ENOMEM;
	device_keeper_reset_if_need(&hc->device_manager, target, setup_data);
	batch_control_write(batch);
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Control read transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts.
 * @param[in] setup_data Data to send with SETUP packet.
 * @param[in] setup_size Size of data to send with SETUP packet (should be 8B).
 * @param[out] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion.
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int control_read(ddf_fun_t *fun, usb_target_t target,
    size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	uhci_hc_t *hc = fun_to_uhci_hc(fun);
	assert(hc);
	usb_speed_t speed = device_keeper_speed(&hc->device_manager, target.address);

	usb_log_debug("Control READ(%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);
	batch_t *batch = batch_get(fun, target, USB_TRANSFER_CONTROL,
	    max_packet_size, speed, data, size, setup_data, setup_size, callback,
	    NULL, arg, &hc->device_manager);
	if (!batch)
		return ENOMEM;
	batch_control_read(batch);
	const int ret = uhci_hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
		return ret;
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
usbhc_iface_t uhci_hc_iface = {
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
