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
#include <errno.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>

#include "iface.h"
#include "hc.h"

/** Reserve default address interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] speed Speed to associate with the new default address.
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
#if 0
	endpoint_t *ep = malloc(sizeof(endpoint_t));
	if (ep == NULL)
		return ENOMEM;
	const size_t max_packet_size = speed == USB_SPEED_LOW ? 8 : 64;
	endpoint_init(ep, USB_TRANSFER_CONTROL, speed, max_packet_size);
	int ret;
try_retgister:
	ret = usb_endpoint_manager_register_ep(&hc->ep_manager,
	    USB_ADDRESS_DEFAULT, 0, USB_DIRECTION_BOTH, ep, endpoint_destroy, 0);
	if (ret == EEXISTS) {
		async_usleep(1000);
		goto try_retgister;
	}
	if (ret != EOK) {
		endpoint_destroy(ep);
	}
	return ret;
#endif
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
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Default address release.\n");
//	return usb_endpoint_manager_unregister_ep(&hc->ep_manager,
//	    USB_ADDRESS_DEFAULT, 0, USB_DIRECTION_BOTH);
	usb_device_keeper_release_default_address(&hc->manager);
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
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address bind %d-%d.\n", address, handle);
	usb_device_keeper_bind(&hc->manager, address, handle);
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
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Address release %d.\n", address);
	usb_device_keeper_release(&hc->manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int register_endpoint(
    ddf_fun_t *fun, usb_address_t address, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned int interval)
{
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	const usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, address);
	const size_t size =
	    (transfer_type == USB_TRANSFER_INTERRUPT
	    || transfer_type == USB_TRANSFER_ISOCHRONOUS) ?
	    max_packet_size : 0;
	int ret;

	endpoint_t *ep = malloc(sizeof(endpoint_t));
	if (ep == NULL)
		return ENOMEM;
	ret = endpoint_init(ep, address, endpoint, direction,
	    transfer_type, speed, max_packet_size);
	if (ret != EOK) {
		free(ep);
		return ret;
	}

	usb_log_debug("Register endpoint %d:%d %s %s(%d) %zu(%zu) %u.\n",
	    address, endpoint, usb_str_transfer_type(transfer_type),
	    usb_str_speed(speed), direction, size, max_packet_size, interval);

	ret = usb_endpoint_manager_register_ep(&hc->ep_manager, ep, size);
	if (ret != EOK) {
		endpoint_destroy(ep);
	} else {
		usb_device_keeper_add_ep(&hc->manager, address, ep);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
static int unregister_endpoint(
    ddf_fun_t *fun, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Unregister endpoint %d:%d %d.\n",
	    address, endpoint, direction);
	return usb_endpoint_manager_unregister_ep(&hc->ep_manager, address,
	    endpoint, direction);
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
static int interrupt_out(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);

	usb_log_debug("Interrupt OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	size_t res_bw;
	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_OUT, &res_bw);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for INT OUT.\n",
			target.address, target.endpoint);
		return ENOENT;
	}
	const size_t bw = bandwidth_count_usb11(ep->speed, ep->transfer_type,
	    size, ep->max_packet_size);
	if (res_bw < bw)
	{
		usb_log_error("Endpoint(%d:%d) INT IN needs %zu bw "
		    "but only %zu is reserved.\n",
		    target.address, target.endpoint, bw, res_bw);
		return ENOENT;
	}
	assert(ep->speed ==
	    usb_device_keeper_get_speed(&hc->manager, target.address));
	assert(ep->max_packet_size == max_packet_size);
	assert(ep->transfer_type == USB_TRANSFER_INTERRUPT);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, ep->transfer_type, ep->max_packet_size,
	        ep->speed, data, size, NULL, 0, NULL, callback, arg, ep);
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
static int interrupt_in(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);

	usb_log_debug("Interrupt IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	size_t res_bw;
	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_IN, &res_bw);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for INT IN.\n",
		    target.address, target.endpoint);
		return ENOENT;
	}
	const size_t bw = bandwidth_count_usb11(ep->speed, ep->transfer_type,
	    size, ep->max_packet_size);
	if (res_bw < bw)
	{
		usb_log_error("Endpoint(%d:%d) INT IN needs %zu bw "
		    "but only %zu bw is reserved.\n",
		    target.address, target.endpoint, bw, res_bw);
		return ENOENT;
	}

	assert(ep->speed ==
	    usb_device_keeper_get_speed(&hc->manager, target.address));
	assert(ep->max_packet_size == max_packet_size);
	assert(ep->transfer_type == USB_TRANSFER_INTERRUPT);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, ep->transfer_type, ep->max_packet_size,
	        ep->speed, data, size, NULL, 0, callback, NULL, arg, ep);
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
static int bulk_out(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);

	usb_log_debug("Bulk OUT %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_OUT, NULL);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for BULK OUT.\n",
			target.address, target.endpoint);
		return ENOENT;
	}
	assert(ep->speed ==
	    usb_device_keeper_get_speed(&hc->manager, target.address));
	assert(ep->max_packet_size == max_packet_size);
	assert(ep->transfer_type == USB_TRANSFER_BULK);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, ep->transfer_type, ep->max_packet_size,
	        ep->speed, data, size, NULL, 0, NULL, callback, arg, ep);
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
static int bulk_in(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_log_debug("Bulk IN %d:%d %zu(%zu).\n",
	    target.address, target.endpoint, size, max_packet_size);

	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_IN, NULL);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for BULK IN.\n",
			target.address, target.endpoint);
		return ENOENT;
	}
	assert(ep->speed ==
	    usb_device_keeper_get_speed(&hc->manager, target.address));
	assert(ep->max_packet_size == max_packet_size);
	assert(ep->transfer_type == USB_TRANSFER_BULK);

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, ep->transfer_type, ep->max_packet_size,
	        ep->speed, data, size, NULL, 0, callback, NULL, arg, ep);
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
/** Control write transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] max_packet_size maximum size of data packet the device accepts.
 * @param[in] setup_data Data to send with SETUP transfer.
 * @param[in] setup_size Size of data to send with SETUP transfer (always 8B).
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion.
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int control_write(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);
	usb_log_debug("Control WRITE (%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);
	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_BOTH, NULL);
	if (ep == NULL) {
		usb_log_warning("Endpoint(%d:%d) not registered for CONTROL.\n",
			target.address, target.endpoint);
	}

	if (setup_size != 8)
		return EINVAL;

	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_CONTROL, max_packet_size, speed,
	        data, size, setup_data, setup_size, NULL, callback, arg, ep);
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
static int control_read(
    ddf_fun_t *fun, usb_target_t target, size_t max_packet_size,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	hc_t *hc = fun_to_hc(fun);
	assert(hc);
	usb_speed_t speed =
	    usb_device_keeper_get_speed(&hc->manager, target.address);

	usb_log_debug("Control READ(%d) %d:%d %zu(%zu).\n",
	    speed, target.address, target.endpoint, size, max_packet_size);
	endpoint_t *ep = usb_endpoint_manager_get_ep(&hc->ep_manager,
	    target.address, target.endpoint, USB_DIRECTION_BOTH, NULL);
	if (ep == NULL) {
		usb_log_warning("Endpoint(%d:%d) not registered for CONTROL.\n",
			target.address, target.endpoint);
	}
	usb_transfer_batch_t *batch =
	    batch_get(fun, target, USB_TRANSFER_CONTROL, max_packet_size, speed,
	        data, size, setup_data, setup_size, callback, NULL, arg, ep);
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
