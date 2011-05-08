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
 *
 */
#include <devman.h>
#include <dev_iface.h>
#include <usb_iface.h>
#include <usb/driver.h>
#include <errno.h>


/** Find host controller handle that is ancestor of given device.
 *
 * @param[in] device_handle Device devman handle.
 * @param[out] hc_handle Where to store handle of host controller
 *	controlling device with @p device_handle handle.
 * @return Error code.
 */
int usb_hc_find(devman_handle_t device_handle, devman_handle_t *hc_handle)
{
	int parent_phone = devman_parent_device_connect(device_handle,
	    IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	devman_handle_t h;
	int rc = async_req_1_1(parent_phone, DEV_IFACE_ID(USB_DEV_IFACE),
	    IPC_M_USB_GET_HOST_CONTROLLER_HANDLE, &h);

	async_hangup(parent_phone);

	if (rc != EOK) {
		return rc;
	}

	if (hc_handle != NULL) {
		*hc_handle = h;
	}

	return EOK;
}

/**
 * @}
 */
