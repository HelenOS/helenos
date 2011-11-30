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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB endpoint pipes miscellaneous functions.
 */
#include <usb/usb.h>
#include <usb/dev/pipes.h>
#include <usb/debug.h>
#include <usb/hc.h>
#include <usbhc_iface.h>
#include <usb_iface.h>
#include <devman.h>
#include <errno.h>
#include <assert.h>
#include "pipepriv.h"

#define IPC_AGAIN_DELAY (1000 * 2) /* 2ms */

/** Tell USB address assigned to given device.
 *
 * @param sess Session to parent device.
 * @param dev Device in question.
 * @return USB address or error code.
 */
static usb_address_t get_my_address(async_sess_t *sess, const ddf_dev_t *dev)
{
	assert(sess);
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;

	usb_address_t address;
	const int ret = usb_get_my_address(exch, &address);

	async_exchange_end(exch);

	return (ret == EOK) ? address : ret;
}

/** Tell USB interface assigned to given device.
 *
 * @param device Device in question.
 * @return Error code (ENOTSUP means any).
 */
int usb_device_get_assigned_interface(const ddf_dev_t *device)
{
	assert(device);
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_ATOMIC, device->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	async_exch_t *exch = async_exchange_begin(parent_sess);
	if (!exch) {
		async_hangup(parent_sess);
		return ENOMEM;
	}

	int iface_no;
	const int ret = usb_get_my_interface(exch, &iface_no);

	return ret == EOK ? iface_no : ret;
}

/** Initialize connection to USB device.
 *
 * @param connection Connection structure to be initialized.
 * @param dev Generic device backing the USB device.
 * @return Error code.
 */
int usb_device_connection_initialize_from_device(
    usb_device_connection_t *connection, const ddf_dev_t *dev)
{
	assert(connection);
	assert(dev);
	
	int rc;
	devman_handle_t hc_handle;
	usb_address_t my_address;
	
	rc = usb_hc_find(dev->handle, &hc_handle);
	if (rc != EOK)
		return rc;
	
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_ATOMIC, dev->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	/*
	 * Asking for "my" address may require several attempts.
	 * That is because following scenario may happen:
	 *  - parent driver (i.e. driver of parent device) announces new device
	 *    and devman launches current driver
	 *  - parent driver is preempted and thus does not send address-handle
	 *    binding to HC driver
	 *  - this driver gets here and wants the binding
	 *  - the HC does not know the binding yet and thus it answers ENOENT
	 *  So, we need to wait for the HC to learn the binding.
	 */
	
	do {
		my_address = get_my_address(parent_sess, dev);
		
		if (my_address == ENOENT) {
			/* Be nice, let other fibrils run and try again. */
			async_usleep(IPC_AGAIN_DELAY);
		} else if (my_address < 0) {
			/* Some other problem, no sense trying again. */
			rc = my_address;
			goto leave;
		}
	
	} while (my_address < 0);
	
	rc = usb_device_connection_initialize(connection,
	    hc_handle, my_address);
	
leave:
	async_hangup(parent_sess);
	return rc;
}

/** Initialize connection to USB device.
 *
 * @param connection Connection structure to be initialized.
 * @param host_controller_handle Devman handle of host controller device is
 * 	connected to.
 * @param device_address Device USB address.
 * @return Error code.
 */
int usb_device_connection_initialize(usb_device_connection_t *connection,
    devman_handle_t host_controller_handle, usb_address_t device_address)
{
	assert(connection);

	if ((device_address < 0) || (device_address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}

	connection->hc_handle = host_controller_handle;
	connection->address = device_address;

	return EOK;
}

/** Initialize connection to USB device on default address.
 *
 * @param dev_connection Device connection structure to be initialized.
 * @param hc_connection Initialized connection to host controller.
 * @return Error code.
 */
int usb_device_connection_initialize_on_default_address(
    usb_device_connection_t *dev_connection,
    usb_hc_connection_t *hc_connection)
{
	assert(dev_connection);

	if (hc_connection == NULL) {
		return EBADMEM;
	}

	return usb_device_connection_initialize(dev_connection,
	    hc_connection->hc_handle, (usb_address_t) 0);
}

/** Prepare pipe for a long transfer.
 *
 * By a long transfer is mean transfer consisting of several
 * requests to the HC.
 * Calling such function is optional and it has positive effect of
 * improved performance because IPC session is initiated only once.
 *
 * @param pipe Pipe over which the transfer will happen.
 * @return Error code.
 */
void usb_pipe_start_long_transfer(usb_pipe_t *pipe)
{
	(void) pipe_add_ref(pipe, true);
}

/** Terminate a long transfer on a pipe.
 *
 * @see usb_pipe_start_long_transfer
 *
 * @param pipe Pipe where to end the long transfer.
 */
void usb_pipe_end_long_transfer(usb_pipe_t *pipe)
{
	pipe_drop_ref(pipe);
}

/**
 * @}
 */
