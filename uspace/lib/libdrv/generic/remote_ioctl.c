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

#include "ioctl.h"
#include "driver.h"

static void remote_ioctl(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call);

/** Remote ioctl interface operations. 
 */
static remote_iface_func_ptr_t remote_ioctl_iface_ops [] = {
	&remote_ioctl
}; 
 
/** Remote ioctl interface structure. 
 * Interface for processing request from remote clients addressed to the ioctl interface.
 */
remote_iface_t remote_char_iface = {
	.method_count = sizeof(remote_ioctl_iface_ops) / sizeof(remote_iface_func_ptr_t),
	.methods = remote_ioctl_iface_ops
};

static void remote_ioctl(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	ioctl_iface_t *ioctl_iface = (ioctl_iface_t *)iface;
	
	ipc_callid_t cidin, cidout;	
	int ctlcode;
	size_t inlen = 0, outlen = 0, retlen = 0;
	void *inbuf, *outbuf;
	int ret;
	
	
	if (!async_data_read_receive(&cidin, &inlen)) {
		// TODO handle protocol error
		ipc_answer_0(callid, EINVAL);
		return;
	}
	
	if (!async_data_write_receive(&cidout, &outlen)) {
		// TODO handle protocol error
		ipc_answer_0(callid, EINVAL);
		return;
    }
	
	if (NULL == ioctl_iface->ioctl) {
		async_data_read_finalize(cidin, NULL, 0);
		async_data_write_finalize(cidout, NULL, 0);
		ipc_answer_0(callid, ENOTSUP);
		return;
	}	
	
	inbuf = malloc(inlen);
	if (NULL == inbuf) {
		async_data_read_finalize(cidin, NULL, 0);
		async_data_write_finalize(cidout, NULL, 0);
		ipc_answer_0(callid, ENOMEM);
		return;
	}
	
	outbuf = malloc(outlen);
	if (NULL == outbuf) {
		free(inbuf);
		async_data_read_finalize(cidin, NULL, 0);
		async_data_write_finalize(cidout, NULL, 0);
		ipc_answer_0(callid, ENOMEM);
	}
	
	ctlcode = (int)IPC_GET_ARG1(*call);
	
	async_data_read_finalize(cidin, inbuf, inlen);
	ret = (*ioctl_iface->ioctl)(dev, ctlcode, inbuf, inlen, outbuf, outlen, &retlen);
	async_data_write_finalize(cidout, outbuf, retlen);
	ipc_answer_1(callid, ret, retlen);	
	free(outbuf);
	free(inbuf);
}

 /**
 * @}
 */