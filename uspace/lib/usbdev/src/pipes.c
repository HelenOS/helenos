/*
 * Copyright (c) 2011 Vojtech Horky
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
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB endpoint pipes miscellaneous functions.
 */
#include <usb_iface.h>
#include <usb/dev/pipes.h>
#include <errno.h>
#include <assert.h>

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
}

/** Terminate a long transfer on a pipe.
 *
 * @see usb_pipe_start_long_transfer
 *
 * @param pipe Pipe where to end the long transfer.
 */
void usb_pipe_end_long_transfer(usb_pipe_t *pipe)
{
}

/**
 * @}
 */
