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

/** Tell USB address assigned to device with given handle.
 *
 * @param dev_handle Devman handle of the USB device in question.
 * @return USB address or negative error code.
 */
usb_address_t usb_get_address_by_handle(devman_handle_t dev_handle)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_ATOMIC, dev_handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	async_exch_t *exch = async_exchange_begin(parent_sess);
	if (!exch) {
		async_hangup(parent_sess);
		return ENOMEM;
	}
	usb_address_t address;
	const int ret = usb_get_my_address(exch, &address);

	async_exchange_end(exch);
	async_hangup(parent_sess);

	if (ret != EOK)
		return ret;

	return address;
}
/*----------------------------------------------------------------------------*/
/** Find host controller handle for the device.
 *
 * @param[in] device_handle Device devman handle.
 * @param[out] hc_handle Where to store handle of host controller
 *	controlling device with @p device_handle handle.
 * @return Error code.
 */
int usb_get_hc_by_handle(devman_handle_t device_handle,
    devman_handle_t *hc_handle)
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
	const int ret = usb_get_hc_handle(exch, hc_handle);

	async_exchange_end(exch);
	async_hangup(parent_sess);

	return ret;
}
/*----------------------------------------------------------------------------*/
int usb_device_connection_initialize(usb_device_connection_t *connection,
    usb_hc_connection_t *hc_connection, usb_address_t address)
{
	assert(connection);

	if (hc_connection == NULL) {
		return EBADMEM;
	}

	if ((address < 0) || (address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}

	connection->hc_connection = hc_connection;
	connection->address = address;
	return EOK;
}
