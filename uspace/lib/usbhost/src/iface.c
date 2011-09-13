/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * @brief HCD DDF interface implementation
 */
#include <ddf/driver.h>
#include <errno.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>

static inline int send_batch(
    ddf_fun_t *fun, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name)
{
	assert(fun);
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);

	int ret;

	size_t res_bw;
	endpoint_t *ep = usb_endpoint_manager_get_ep(&hcd->ep_manager,
	    target.address, target.endpoint, direction, &res_bw);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.\n",
		    target.address, target.endpoint, name);
		return ENOENT;
	}

	usb_log_debug2("%s %d:%d %zu(%zu).\n",
	    name, target.address, target.endpoint, size, ep->max_packet_size);

	const size_t bw = bandwidth_count_usb11(
	    ep->speed, ep->transfer_type, size, ep->max_packet_size);
	if (res_bw < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    target.address, target.endpoint, name, bw, res_bw);
		return ENOSPC;
	}
	if (!hcd->schedule) {
		usb_log_error("HCD does not implement scheduler.\n");
		return ENOTSUP;
	}

	/* No private data and no private data dtor */
	usb_transfer_batch_t *batch =
	    usb_transfer_batch_get(ep, data, size, setup_data,
	    in, out, arg, fun, NULL, NULL);
	if (!batch) {
		return ENOMEM;
	}

	ret = hcd->schedule(hcd, batch);
	if (ret != EOK)
		usb_transfer_batch_dispose(batch);

	return ret;
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
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);
	assert(address);

	usb_log_debug("Address request speed: %s.\n", usb_str_speed(speed));
	*address = device_keeper_get_free_address(&hcd->dev_manager, speed);
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
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);

	usb_log_debug("Address bind %d-%" PRIun ".\n", address, handle);
	usb_device_keeper_bind(&hcd->dev_manager, address, handle);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Find device handle by address interface function.
 *
 * @param[in] fun DDF function that was called.
 * @param[in] address Address in question.
 * @param[out] handle Where to store device handle if found.
 * @return Error code.
 */
static int find_by_address(ddf_fun_t *fun, usb_address_t address,
    devman_handle_t *handle)
{
	assert(fun);
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);
	const bool found =
	    usb_device_keeper_find_by_address(&hcd->dev_manager, address, handle);
	return found ? EOK : ENOENT;
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
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);
	usb_log_debug("Address release %d.\n", address);
	usb_device_keeper_release(&hcd->dev_manager, address);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int register_endpoint(
    ddf_fun_t *fun, usb_address_t address, usb_speed_t ep_speed,
    usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned int interval)
{
	assert(fun);
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);
	const size_t size = max_packet_size;
	/* Default address is not bound or registered,
	 * thus it does not provide speed info. */
	const usb_speed_t speed = (address == 0) ? ep_speed :
	    usb_device_keeper_get_speed(&hcd->dev_manager, address);

	usb_log_debug("Register endpoint %d:%d %s-%s %s %zuB %ums.\n",
	    address, endpoint, usb_str_transfer_type(transfer_type),
	    usb_str_direction(direction), usb_str_speed(speed),
	    max_packet_size, interval);

	endpoint_t *ep = endpoint_get(
	    address, endpoint, direction, transfer_type, speed, max_packet_size);
	if (!ep)
		return ENOMEM;
	int ret = EOK;

	if (hcd->ep_add_hook) {
		ret = hcd->ep_add_hook(hcd, ep);
	}
	if (ret != EOK) {
		endpoint_destroy(ep);
		return ret;
	}

	ret = usb_endpoint_manager_register_ep(&hcd->ep_manager, ep, size);
	if (ret != EOK) {
		endpoint_destroy(ep);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
static int unregister_endpoint(
    ddf_fun_t *fun, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(fun);
	hcd_t *hcd = fun_to_hcd(fun);
	assert(hcd);
	usb_log_debug("Unregister endpoint %d:%d %s.\n",
	    address, endpoint, usb_str_direction(direction));
	return usb_endpoint_manager_unregister_ep(&hcd->ep_manager, address,
	    endpoint, direction);
}
/*----------------------------------------------------------------------------*/
/** Interrupt out transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int interrupt_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_OUT, data, size,
	    0, NULL, callback, arg, "Interrupt OUT");
}
/*----------------------------------------------------------------------------*/
/** Interrupt in transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[out] data Data destination.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int interrupt_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_IN, data, size,
	    0, callback, NULL, arg, "Interrupt IN");
}
/*----------------------------------------------------------------------------*/
/** Bulk out transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int bulk_out(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_OUT, data, size,
	    0, NULL, callback, arg, "Bulk OUT");
}
/*----------------------------------------------------------------------------*/
/** Bulk in transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[out] data Data destination.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int bulk_in(
    ddf_fun_t *fun, usb_target_t target, void *data,
    size_t size, usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_IN, data, size,
	    0, callback, NULL, arg, "Bulk IN");
}
/*----------------------------------------------------------------------------*/
/** Control write transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] setup_data Data to send with SETUP transfer.
 * @param[in] setup_size Size of data to send with SETUP transfer (always 8B).
 * @param[in] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion.
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int control_write(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	uint64_t setup_buffer;
	memcpy(&setup_buffer, setup_data, 8);
	return send_batch(fun, target, USB_DIRECTION_BOTH, data, size,
	    setup_buffer, NULL, callback, arg, "Control WRITE");
}
/*----------------------------------------------------------------------------*/
/** Control read transaction interface function
 *
 * @param[in] fun DDF function that was called.
 * @param[in] target USB device to write to.
 * @param[in] setup_data Data to send with SETUP packet.
 * @param[in] setup_size Size of data to send with SETUP packet (should be 8B).
 * @param[out] data Source of data.
 * @param[in] size Size of data source.
 * @param[in] callback Function to call on transaction completion.
 * @param[in] arg Additional for callback function.
 * @return Error code.
 */
static int control_read(
    ddf_fun_t *fun, usb_target_t target,
    void *setup_data, size_t setup_size, void *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	uint64_t setup_buffer;
	memcpy(&setup_buffer, setup_data, 8);
	return send_batch(fun, target, USB_DIRECTION_BOTH, data, size,
	    setup_buffer, callback, NULL, arg, "Control READ");
}
/*----------------------------------------------------------------------------*/
static int usb_read(ddf_fun_t *fun, usb_target_t target, uint64_t setup_data,
    uint8_t *data, size_t size, usbhc_iface_transfer_in_callback_t callback,
    void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_IN, data, size,
	    setup_data, callback, NULL, arg, "READ");
}
/*----------------------------------------------------------------------------*/
static int usb_write(ddf_fun_t *fun, usb_target_t target, uint64_t setup_data,
    const uint8_t *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	return send_batch(fun, target, USB_DIRECTION_OUT, (uint8_t*)data, size,
	    setup_data, NULL, callback, arg, "WRITE");
}
/*----------------------------------------------------------------------------*/
usbhc_iface_t hcd_iface = {
	.request_address = request_address,
	.bind_address = bind_address,
	.find_by_address = find_by_address,
	.release_address = release_address,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.interrupt_out = interrupt_out,
	.interrupt_in = interrupt_in,

	.bulk_out = bulk_out,
	.bulk_in = bulk_in,

	.control_write = control_write,
	.control_read = control_read,

	.read = usb_read,
	.write = usb_write,
};
/**
 * @}
 */
