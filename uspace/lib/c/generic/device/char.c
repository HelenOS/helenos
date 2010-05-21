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
#include <device/char.h>
#include <errno.h>
#include <async.h>
#include <malloc.h>
#include <stdio.h>

/** Read to or write from the device using its character interface.
 * 
 * Helper function.
 * 
 * @param dev_phone phone to the device.
 * @param buf the buffer for the data read from or written to the device.
 * @param len the maximum length of the data to be read or written.
 * @param read read from the device if true, write to it otherwise.
 * 
 * @return non-negative number of bytes actually read from or written to the device on success,
 * negative error number otherwise.
 * 
 */
static int rw_dev(int dev_phone, void *buf, size_t len, bool read) 
{
	ipc_call_t answer;
	
	async_serialize_start();
	
	aid_t req;
	int ret;
	
	if (read) {
		req = async_send_1(dev_phone, DEV_IFACE_ID(CHAR_DEV_IFACE), CHAR_READ_DEV, &answer);
		ret = async_data_read_start(dev_phone, buf, len);		
	} else {
		req = async_send_1(dev_phone, DEV_IFACE_ID(CHAR_DEV_IFACE), CHAR_WRITE_DEV, &answer);
		ret = async_data_write_start(dev_phone, buf, len);
	}
	
	ipcarg_t rc;
	if (ret != EOK) {		
		async_wait_for(req, &rc);
		async_serialize_end();
		if (rc == EOK) {
			return ret;
		}
		else {
			return (int) rc;
		}
	}
	
	async_wait_for(req, &rc);
	async_serialize_end();
	
	ret = (int)rc;
	if (EOK != ret) {
		return ret;
	}
	
	return IPC_GET_ARG1(answer);	
}

/** Read from the device using its character interface.
 * 
 * @param dev_phone phone to the device.
 * @param buf the output buffer for the data read from the device.
 * @param len the maximum length of the data to be read.
 * 
 * @return non-negative number of bytes actually read from the device on success, negative error number otherwise.
 */
int read_dev(int dev_phone, void *buf, size_t len)
{	
	return rw_dev(dev_phone, buf, len, true);
}

/** Write to the device using its character interface.
 * 
 * @param dev_phone phone to the device.
 * @param buf the input buffer containg the data to be written to the device.
 * @param len the maximum length of the data to be written.
 * 
 * @return non-negative number of bytes actually written to the device on success, negative error number otherwise.
 */
int write_dev(int dev_phone, void *buf, size_t len)
{
	return rw_dev(dev_phone, buf, len, false);
}

  
 /** @}
 */