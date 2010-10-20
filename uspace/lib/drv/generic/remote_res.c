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

#include "driver.h"
#include "resource.h"

 
static void remote_res_get_resources(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call);
static void remote_res_enable_interrupt(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call);

static remote_iface_func_ptr_t remote_res_iface_ops [] = {
	&remote_res_get_resources,
	&remote_res_enable_interrupt	
}; 
 
remote_iface_t remote_res_iface = {
	.method_count = sizeof(remote_res_iface_ops) / sizeof(remote_iface_func_ptr_t),
	.methods = remote_res_iface_ops
};

static void remote_res_enable_interrupt(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	resource_iface_t *ires = (resource_iface_t *)iface;
	
	if (NULL == ires->enable_interrupt) {
		ipc_answer_0(callid, ENOTSUP);
	} else if (ires->enable_interrupt(dev)) {
		ipc_answer_0(callid, EOK);
	} else {
		ipc_answer_0(callid, EREFUSED);
	}	
}

static void remote_res_get_resources(device_t *dev, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	resource_iface_t *ires = (resource_iface_t *)iface;	
	if (NULL == ires->get_resources) {
		ipc_answer_0(callid, ENOTSUP);
		return;
	}
	
	hw_resource_list_t *hw_resources = ires->get_resources(dev);	
	if (NULL == hw_resources){
		ipc_answer_0(callid, ENOENT);
		return;
	}	
	
	ipc_answer_1(callid, EOK, hw_resources->count);	

	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		// protocol error - the recipient is not accepting data
		return;
	}
	async_data_read_finalize(callid, hw_resources->resources, len);
}
 
 
 /**
 * @}
 */