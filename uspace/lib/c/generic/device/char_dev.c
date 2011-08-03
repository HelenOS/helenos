/*
 * Copyright (c) 2010 Lenka Trochtova
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

 /** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ipc/dev_iface.h>
#include <device/char_dev.h>
#include <errno.h>
#include <async.h>
#include <malloc.h>
#include <stdio.h>

/** Read to or write from device.
 *
 * Helper function to read to or write from a device
 * using its character interface.
 *
 * @param sess Session to the device.
 * @param buf  Buffer for the data read from or written to the device.
 * @param size Maximum size of data (in bytes) to be read or written.
 * @param read Read from the device if true, write to it otherwise.
 *
 * @return Non-negative number of bytes actually read from or
 *         written to the device on success, negative error number
 *         otherwise.
 *
 */
static ssize_t char_dev_rw(async_sess_t *sess, void *buf, size_t size, bool read)
{
	ipc_call_t answer;
	aid_t req;
	int ret;
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	if (read) {
		req = async_send_1(exch, DEV_IFACE_ID(CHAR_DEV_IFACE),
		    CHAR_DEV_READ, &answer);
		ret = async_data_read_start(exch, buf, size);
	} else {
		req = async_send_1(exch, DEV_IFACE_ID(CHAR_DEV_IFACE),
		    CHAR_DEV_WRITE, &answer);
		ret = async_data_write_start(exch, buf, size);
	}
	
	async_exchange_end(exch);
	
	sysarg_t rc;
	if (ret != EOK) {
		async_wait_for(req, &rc);
		if (rc == EOK)
			return (ssize_t) ret;
		
		return (ssize_t) rc;
	}
	
	async_wait_for(req, &rc);
	
	ret = (int) rc;
	if (ret != EOK)
		return (ssize_t) ret;
	
	return (ssize_t) IPC_GET_ARG1(answer);
}

/** Read from character device.
 *
 * @param sess Session to the device.
 * @param buf  Output buffer for the data read from the device.
 * @param size Maximum size (in bytes) of the data to be read.
 *
 * @return Non-negative number of bytes actually read from the
 *         device on success, negative error number otherwise.
 *
 */
ssize_t char_dev_read(async_sess_t *sess, void *buf, size_t size)
{
	return char_dev_rw(sess, buf, size, true);
}

/** Write to character device.
 *
 * @param sess Session to the device.
 * @param buf  Input buffer containg the data to be written to the
 *             device.
 * @param size Maximum size (in bytes) of the data to be written.
 *
 * @return Non-negative number of bytes actually written to the
 *         device on success, negative error number otherwise.
 *
 */
ssize_t char_dev_write(async_sess_t *sess, void *buf, size_t size)
{
	return char_dev_rw(sess, buf, size, false);
}

/** @}
 */
