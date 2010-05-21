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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>

#include "char.h"
#include "driver.h"

#define MAX_CHAR_RW_COUNT 256

static void remote_char_read(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call);
static void remote_char_write(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call);

/** Remote character interface operations. 
 */
static remote_iface_func_ptr_t remote_char_iface_ops [] = {
	&remote_char_read,
	&remote_char_write	
}; 
 
/** Remote character interface structure. 
 * Interface for processing request from remote clients addressed to the character interface.
 */
remote_iface_t remote_char_iface = {
	.method_count = sizeof(remote_char_iface_ops) / sizeof(remote_iface_func_ptr_t),
	.methods = remote_char_iface_ops
};

/** Process the read request from the remote client.
 * 
 * Receive the read request's parameters from the remote client and pass them to the local interface.
 * Return the result of the operation processed by the local interface to the remote client.
 * 
 * @param dev the device from which the data are read.
 * @param iface the local interface structure. 
 */
static void remote_char_read(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{	
	char_iface_t *char_iface = (char_iface_t *)iface;
	ipc_callid_t cid;
	
	size_t len;
	if (!async_data_read_receive(&cid, &len)) {
		// TODO handle protocol error
		ipc_answer_0(callid, EINVAL);
		return;
	}
	
	if (!char_iface->read) {
		async_data_read_finalize(cid, NULL, 0);
		ipc_answer_0(callid, ENOTSUP);
		return;
	}
	
	if (len > MAX_CHAR_RW_COUNT) {
		len = MAX_CHAR_RW_COUNT;
	}
	
	char buf[MAX_CHAR_RW_COUNT];
	int ret = (*char_iface->read)(dev, buf, len);
	
	if (ret < 0) { // some error occured
		async_data_read_finalize(cid, buf, 0);
		ipc_answer_0(callid, ret);
		return;
	}
	
	// the operation was successful, return the number of data read
	async_data_read_finalize(cid, buf, ret);
	ipc_answer_1(callid, EOK, ret);	
}

/** Process the write request from the remote client.
 * 
 * Receive the write request's parameters from the remote client and pass them to the local interface.
 * Return the result of the operation processed by the local interface to the remote client.
 * 
 * @param dev the device to which the data are written.
 * @param iface the local interface structure. 
 */
static void remote_char_write(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	char_iface_t *char_iface = (char_iface_t *)iface;
	ipc_callid_t cid;
	size_t len;
	
	if (!async_data_write_receive(&cid, &len)) {
		// TODO handle protocol error
		ipc_answer_0(callid, EINVAL);
		return;
    }
	
	if (!char_iface->write) {
		async_data_write_finalize(cid, NULL, 0);
		ipc_answer_0(callid, ENOTSUP);
		return;
	}	
	
	if (len > MAX_CHAR_RW_COUNT) {
		len = MAX_CHAR_RW_COUNT;
	}
	
	char buf[MAX_CHAR_RW_COUNT];
	
	async_data_write_finalize(cid, buf, len);
	
	int ret = (*char_iface->write)(dev, buf, len);
	if (ret < 0) { // some error occured
		ipc_answer_0(callid, ret);
	} else {
		// the operation was successful, return the number of data written
		ipc_answer_1(callid, EOK, ret);
	}
}

 /**
 * @}
 */