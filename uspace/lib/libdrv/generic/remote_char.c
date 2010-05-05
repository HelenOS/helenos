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

static remote_iface_func_ptr_t remote_char_iface_ops [] = {
	&remote_char_read,
	&remote_char_write	
}; 
 
remote_iface_t remote_char_iface = {
	.method_count = sizeof(remote_char_iface_ops) / sizeof(remote_iface_func_ptr_t),
	.methods = remote_char_iface_ops
};

static void remote_char_read(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{	
	char_iface_t *char_iface = (char_iface_t *)iface;
	
	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		// TODO handle protocol error
		ipc_answer_0(callid, EINVAL);
		return;
	}
	
	if (!char_iface->read) {
		async_data_read_finalize(callid, NULL, 0);
		ipc_answer_0(callid, ENOTSUP);
		return;
	}
	
	if (len > MAX_CHAR_RW_COUNT) {
		len = MAX_CHAR_RW_COUNT;
	}
	
	char buf[MAX_CHAR_RW_COUNT];
	int ret = (*char_iface->read)(dev, buf, len);
	
	if (ret < 0) { // some error occured
		async_data_read_finalize(callid, buf, 0);
		ipc_answer_0(callid, ret);
		return;
	}
	
	printf("remote_char_read - async_data_read_finalize\n");
	async_data_read_finalize(callid, buf, ret);
	printf("remote_char_read - ipc_answer_0(callid, EOK);\n");
	ipc_answer_0(callid, EOK);	
}

static void remote_char_write(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	char_iface_t *char_iface = (char_iface_t *)iface;
	if (!char_iface->write) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}	
	
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		// TODO handle protocol error
		return;
    }
	
	if (len > MAX_CHAR_RW_COUNT) {
		len = MAX_CHAR_RW_COUNT;
	}
	
	char buf[MAX_CHAR_RW_COUNT];
	
	async_data_write_finalize(callid, buf, len);
	
	int ret = (*char_iface->write)(dev, buf, len);
	if (ret < 0) { // some error occured
		ipc_answer_0(callid, ret);
	} else {
		ipc_answer_0(callid, EOK);
	}
}

 /**
 * @}
 */