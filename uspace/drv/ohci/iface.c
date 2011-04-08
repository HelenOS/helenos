/*
 * Copyright (c) 2011 Vojtech Horky
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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * USB-HC interface implementation.
 */
#include <ddf/driver.h>
#include <errno.h>

#include <usb/debug.h>

#include "iface.h"
#include "hc.h"

#define UNSUPPORTED(methodname) \
	usb_log_warning("Unsupported interface method `%s()' in %s:%d.\n", \
	    methodname, __FILE__, __LINE__)

/** Reserve default address.
 *
 * This function may block the caller.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] speed Speed of the device for which the default address is
 *	reserved.
 * @return Error code.
 */
static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Default address request with speed %d.\n", speed);
	usb_device_keeper_reserve_default_address(&hc->manager, speed);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Release default address.
 *
 * @param[in] fun Device function the action was invoked on.
 * @return Error code.
 */
static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Default address release.\n");
	usb_device_keeper_release_default_address(&hc->manager);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Found free USB address.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] speed Speed of the device that will get this address.
 * @param[out] address Non-null pointer where to store the free address.
 * @return Error code.
 */
static int request_address(
    ddf_fun_t *fun, usb_speed_t speed, usb_address_t *address)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	assert(address);

	usb_log_debug("Address request with speed %d.\n", speed);
	*address = device_keeper_get_free_address(&hc->manager, speed);
	usb_log_debug("Address request with result: %d.\n", *address);
	if (*address <= 0)
		return *address;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Bind USB address with device devman handle.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] address USB address of the device.
 * @param[in] handle Devman handle of the device.
 * @return Error code.
 */
static int bind_address(
    ddf_fun_t *fun, usb_address_t address, devman_handle_t handle)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address bind %d-%d.\n", address, handle);
	usb_device_keeper_bind(&hc->manager, address, handle);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Release previously requested address.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] address USB address to be released.
 * @return Error code.
 */
static int release_address(ddf_fun_t *fun, usb_address_t address)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address release %d.\n", address);
	usb_device_keeper_release(&hc->manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Register endpoint for bandwidth reservation.
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] address USB address of the device.
 * @param[in] endpoint Endpoint number.
 * @param[in] transfer_type USB transfer type.
 * @param[in] direction Endpoint data direction.
 * @param[in] max_packet_size Max packet size of the endpoint.
 * @param[in] interval Polling interval.
 * @return Error code.
 */
static int register_endpoint(
    ddf_fun_t *fun, usb_address_t address, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned int interval)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	if (address == hc->rh.address)
		return EOK;
	const usb_speed_t speed =
		usb_device_keeper_get_speed(&hc->manager, address);
	const size_t size = max_packet_size;
	usb_log_debug("Register endpoint %d:%d %s %s(%d) %zu(%zu) %u.\n",
	    address, endpoint, usb_str_transfer_type(transfer_type),
	    usb_str_speed(speed), direction, size, max_packet_size, interval);
	// TODO use real endpoint here!
	return usb_endpoint_manager_register_ep(&hc->ep_manager,NULL, 0);
}
/*----------------------------------------------------------------------------*/
/** Unregister endpoint (free some bandwidth reservation).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] address USB address of the device.
 * @param[in] endpoint Endpoint number.
 * @param[in] direction Endpoint data direction.
 * @return Error code.
 */
static int unregister_endpoint(
    ddf_fun_t *fun, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Unregister endpoint %d:%d %d.\n",
	    address, endpoint, direction);
	return usb_endpoint_manager_unregister_ep(&hc->ep_manager, address,
	    endpoint, direction);
}
/*----------------------------------------------------------------------------*/
/** Schedule interrupt out transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Data to be sent (in USB endianess, allocated and deallocated
 *	by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int interrupt_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);

	usb_log_debug("Interrupt OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_INTERRUPT, max_packet_size,
	        speed, data, size, NULL, 0, NULL, callback, arg, &hc->manager);
	if (!batch)
		return ENOMEM;
	batch_interrupt_out(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule interrupt in transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int interrupt_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);
	usb_log_debug("Interrupt IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_INTERRUPT, max_packet_size,
	        speed, data, size, NULL, 0, callback, NULL, arg, &hc->manager);
	if (!batch)
		return ENOMEM;
	batch_interrupt_in(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule bulk out transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Data to be sent (in USB endianess, allocated and deallocated
 *	by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int bulk_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);

	usb_log_debug("Bulk OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_BULK, max_packet_size, speed,
	        data, size, NULL, 0, NULL, callback, arg, &hc->manager);
	if (!batch)
		return ENOMEM;
	batch_bulk_out(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule bulk in transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] data Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] size Size of the @p data buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int bulk_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);
	usb_log_debug("Bulk IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_BULK, max_packet_size, speed,
	        data, size, NULL, 0, callback, NULL, arg, &hc->manager);
	if (!batch)
		return ENOMEM;
	batch_bulk_in(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule control write transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] setup_packet Setup packet buffer (in USB endianess, allocated
 *	and deallocated by the caller).
 * @param[in] setup_packet_size Size of @p setup_packet buffer in bytes.
 * @param[in] data_buffer Data buffer (in USB endianess, allocated and
 *	deallocated by the caller).
 * @param[in] data_buffer_size Size of @p data_buffer buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int control_write(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);
	usb_log_debug("Control WRITE (%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);

	if (setup_size != 8)
		return EINVAL;

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_CONTROL, max_packet_size,
	        speed, data, size, setup_data, setup_size, NULL, callback, arg,
		&hc->manager);
	if (!batch)
		return ENOMEM;
	usb_device_keeper_reset_if_need(&hc->manager, target, setup_data);
	batch_control_write(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Schedule control read transfer.
 *
 * The callback is supposed to be called once the transfer (on the wire) is
 * complete regardless of the outcome.
 * However, the callback could be called only when this function returns
 * with success status (i.e. returns EOK).
 *
 * @param[in] fun Device function the action was invoked on.
 * @param[in] target Target pipe (address and endpoint number) specification.
 * @param[in] setup_packet Setup packet buffer (in USB endianess, allocated
 *	and deallocated by the caller).
 * @param[in] setup_packet_size Size of @p setup_packet buffer in bytes.
 * @param[in] data_buffer Buffer where to store the data (in USB endianess,
 *	allocated and deallocated by the caller).
 * @param[in] data_buffer_size Size of @p data_buffer buffer in bytes.
 * @param[in] callback Callback to be issued once the transfer is complete.
 * @param[in] arg Pass-through argument to the callback.
 * @return Error code.
 */
static int control_read(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);

	// FIXME: get from endpoint manager
	size_t max_packet_size = 8;

	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);

	usb_log_debug("Control READ(%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);
	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_CONTROL, max_packet_size,
	        speed, data, size, setup_data, setup_size, callback, NULL, arg,
		&hc->manager);
	if (!batch)
		return ENOMEM;
	batch_control_read(batch);
	const int ret = hc_schedule(hc, batch);
	if (ret != EOK) {
		batch_dispose(batch);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
/** Host controller interface implementation for OHCI. */
usbhc_iface_t hc_iface = {
	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.request_address = request_address,
	.bind_address = bind_address,
	.release_address = release_address,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.bulk_out = bulk_out,
	.bulk_in = bulk_in,

	.control_write = control_write,
	.control_read = control_read,
};

/**
 * @}
 */
