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

#include <usb/dev.h>
#include <errno.h>
#include <usb_iface.h>

/** Find host controller handle, address and iface number for the device.
 *
 * @param[in] device_handle Device devman handle.
 * @param[out] hc_handle Where to store handle of host controller
 *	controlling device with @p device_handle handle.
 * @param[out] address Place to store the device's address
 * @param[out] iface Place to stoer the assigned USB interface number.
 * @return Error code.
 */
int usb_get_info_by_handle(devman_handle_t device_handle,
    devman_handle_t *hc_handle, usb_address_t *address, int *iface)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_ATOMIC, device_handle,
	        IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	async_exch_t *exch = async_exchange_begin(parent_sess);
	if (!exch) {
		async_hangup(parent_sess);
		return ENOMEM;
	}

	usb_address_t tmp_address;
	devman_handle_t tmp_handle;
	int tmp_iface;

	if (address) {
		const int ret = usb_get_my_address(exch, &tmp_address);
		if (ret != EOK) {
			async_exchange_end(exch);
			async_hangup(parent_sess);
			return ret;
		}
	}

	if (hc_handle) {
		const int ret = usb_get_hc_handle(exch, &tmp_handle);
		if (ret != EOK) {
			async_exchange_end(exch);
			async_hangup(parent_sess);
			return ret;
		}
	}

	if (iface) {
		const int ret = usb_get_my_interface(exch, &tmp_iface);
		switch (ret) {
		case ENOTSUP:
			/* Implementing GET_MY_INTERFACE is voluntary. */
			tmp_iface = -1;
		case EOK:
			break;
		default:
			async_exchange_end(exch);
			async_hangup(parent_sess);
			return ret;
		}
	}

	if (hc_handle)
		*hc_handle = tmp_handle;

	if (address)
		*address = tmp_address;

	if (iface)
		*iface = tmp_iface;

	async_exchange_end(exch);
	async_hangup(parent_sess);

	return EOK;
}
