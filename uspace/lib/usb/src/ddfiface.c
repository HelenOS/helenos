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
#include <usb/ddfiface.h>
#include <errno.h>

/** DDF interface for USB device, implementation for typical hub. */
usb_iface_t  usb_iface_hub_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_hub_impl,
	.get_address = usb_iface_get_address_hub_impl
};

/** DDF interface for USB device, implementation for child of a typical hub. */
usb_iface_t  usb_iface_hub_child_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_hub_child_impl,
	.get_address = usb_iface_get_address_hub_child_impl
};


/** Get host controller handle, interface implementation for hub driver.
 *
 * @param[in] device Device the operation is running on.
 * @param[out] handle Storage for the host controller handle.
 * @return Error code.
 */
int usb_iface_get_hc_handle_hub_impl(device_t *device, devman_handle_t *handle)
{
	assert(device);
	return usb_hc_find(device->handle, handle);
}

/** Get host controller handle, interface implementation for child of
 * a hub driver.
 *
 * @param[in] device Device the operation is running on.
 * @param[out] handle Storage for the host controller handle.
 * @return Error code.
 */
int usb_iface_get_hc_handle_hub_child_impl(device_t *device,
    devman_handle_t *handle)
{
	assert(device);
	device_t *parent = device->parent;

	/* Default error, device does not support this operation. */
	int rc = ENOTSUP;

	if (parent && parent->ops && parent->ops->interfaces[USB_DEV_IFACE]) {
		usb_iface_t *usb_iface
		    = (usb_iface_t *) parent->ops->interfaces[USB_DEV_IFACE];
		assert(usb_iface != NULL);

		if (usb_iface->get_hc_handle) {
			rc = usb_iface->get_hc_handle(parent, handle);
		}
	}

	return rc;
}

/** Get host controller handle, interface implementation for HC driver.
 *
 * @param[in] device Device the operation is running on.
 * @param[out] handle Storage for the host controller handle.
 * @return Always EOK.
 */
int usb_iface_get_hc_handle_hc_impl(device_t *device, devman_handle_t *handle)
{
	assert(device);

	if (handle != NULL) {
		*handle = device->handle;
	}

	return EOK;
}

/** Get USB device address, interface implementation for hub driver.
 *
 * @param[in] device Device the operation is running on.
 * @param[in] handle Devman handle of USB device we want address of.
 * @param[out] address Storage for USB address of device with handle @p handle.
 * @return Error code.
 */
int usb_iface_get_address_hub_impl(device_t *device, devman_handle_t handle,
    usb_address_t *address)
{
	assert(device);
	int parent_phone = devman_parent_device_connect(device->handle,
	    IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	sysarg_t addr;
	int rc = async_req_2_1(parent_phone, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_ADDRESS, handle, &addr);

	async_hangup(parent_phone);

	if (rc != EOK) {
		return rc;
	}

	if (address != NULL) {
		*address = (usb_address_t) addr;
	}

	return EOK;
}

/** Get USB device address, interface implementation for child of
 * a hub driver.
 *
 * @param[in] device Device the operation is running on.
 * @param[in] handle Devman handle of USB device we want address of.
 * @param[out] address Storage for USB address of device with handle @p handle.
 * @return Error code.
 */
int usb_iface_get_address_hub_child_impl(device_t *device,
    devman_handle_t handle, usb_address_t *address)
{
	assert(device);
	device_t *parent = device->parent;

	/* Default error, device does not support this operation. */
	int rc = ENOTSUP;

	if (parent && parent->ops && parent->ops->interfaces[USB_DEV_IFACE]) {
		usb_iface_t *usb_iface
		    = (usb_iface_t *) parent->ops->interfaces[USB_DEV_IFACE];
		assert(usb_iface != NULL);

		if (usb_iface->get_address) {
			rc = usb_iface->get_address(parent, handle, address);
		}
	}

	return rc;
}

/**
 * @}
 */
