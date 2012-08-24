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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Implementations of DDF interfaces functions (actual implementation).
 */

#include <devman.h>
#include <async.h>
#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/hc.h>
#include <usb/debug.h>
#include <usb/dev/hub.h>
#include <errno.h>
#include <assert.h>

#include <usb/dev.h>

/** DDF interface for USB device, implementation for typical hub. */
usb_iface_t usb_iface_hub_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_device_impl,
	.get_my_address = usb_iface_get_my_address_forward_impl,
};

/** DDF interface for USB device, implementation for child of a typical hub. */
usb_iface_t usb_iface_hub_child_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_device_impl,
	.get_my_address = usb_iface_get_my_address_from_device_data,
};


/** Get host controller handle, interface implementation for hub driver.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[out] handle Storage for the host controller handle.
 * @return Error code.
 */
int usb_iface_get_hc_handle_device_impl(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	return usb_get_hc_by_handle(ddf_fun_get_handle(fun), handle);
}

/** Get host controller handle, interface implementation for HC driver.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[out] handle Storage for the host controller handle.
 * @return Always EOK.
 */
int usb_iface_get_hc_handle_hc_impl(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);

	if (handle != NULL) {
		*handle = ddf_fun_get_handle(fun);
	}

	return EOK;
}

/** Get USB device address, interface implementation for hub driver.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[in] handle Devman handle of USB device we want address of.
 * @param[out] address Storage for USB address of device with handle @p handle.
 * @return Error code.
 */
int usb_iface_get_my_address_forward_impl(ddf_fun_t *fun,
    usb_address_t *address)
{
	assert(fun);
	return usb_get_address_by_handle(ddf_fun_get_handle(fun), address);
}

/** Get USB device address, interface implementation for child of
 * a hub driver.
 *
 * This implementation eccepts 0 as valid handle and replaces it with fun's
 * handle.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[in] handle Devman handle of USB device we want address of.
 * @param[out] address Storage for USB address of device with handle @p handle.
 * @return Error code.
 */
int usb_iface_get_my_address_from_device_data(ddf_fun_t *fun,
    usb_address_t *address)
{
	const usb_hub_attached_device_t *device = ddf_fun_data_get(fun);
	assert(device->fun == fun);
	if (address)
		*address = device->address;
	return EOK;
}

/**
 * @}
 */
