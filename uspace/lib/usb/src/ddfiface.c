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
#include <ipc/devman.h>
#include <devman.h>
#include <async.h>
#include <usb/ddfiface.h>
#include <usb/hc.h>
#include <usb/debug.h>
#include <errno.h>
#include <assert.h>

/** DDF interface for USB device, implementation for typical hub. */
usb_iface_t  usb_iface_hub_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_device_impl,
	.get_address = usb_iface_get_address_forward_impl,
};

/** DDF interface for USB device, implementation for child of a typical hub. */
usb_iface_t  usb_iface_hub_child_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_device_impl,
	.get_address = usb_iface_get_address_set_my_handle_impl
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
	return usb_hc_find(fun->handle, handle);
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
		*handle = fun->handle;
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
int usb_iface_get_address_forward_impl(ddf_fun_t *fun, devman_handle_t handle,
    usb_address_t *address)
{
	assert(fun);

	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, fun->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	async_exch_t *exch = async_exchange_begin(parent_sess);

	sysarg_t addr;
	int rc = async_req_2_1(exch, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_ADDRESS, handle, &addr);

	async_exchange_end(exch);
	async_hangup(parent_sess);

	if (rc != EOK)
		return rc;

	if (address != NULL)
		*address = (usb_address_t) addr;

	return EOK;
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
int usb_iface_get_address_set_my_handle_impl(ddf_fun_t *fun,
    devman_handle_t handle, usb_address_t *address)
{
	if (handle == 0) {
		handle = fun->handle;
	}
	return usb_iface_get_address_forward_impl(fun, handle, address);
}

/**
 * @}
 */
