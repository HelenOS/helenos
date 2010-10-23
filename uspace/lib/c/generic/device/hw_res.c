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
 
#include <device/hw_res.h>
#include <errno.h>
#include <async.h>
#include <malloc.h>

bool get_hw_resources(int dev_phone, hw_resource_list_t *hw_resources)
{
	ipcarg_t count = 0;
	int rc = async_req_1_1(dev_phone, DEV_IFACE_ID(HW_RES_DEV_IFACE), GET_RESOURCE_LIST, &count);
	hw_resources->count = count;
	if (EOK != rc) {
		return false;
	}
	
	size_t size = count * sizeof(hw_resource_t);
	hw_resources->resources = (hw_resource_t *)malloc(size);
	if (NULL == hw_resources->resources) {
		return false;
	}
	
	rc = async_data_read_start(dev_phone, hw_resources->resources, size);
	if (EOK != rc) {
		free(hw_resources->resources);
		hw_resources->resources = NULL;
		return false;
	}
	 	 
	return true;	 
}

bool enable_interrupt(int dev_phone)
{
	int rc = async_req_1_0(dev_phone, DEV_IFACE_ID(HW_RES_DEV_IFACE), ENABLE_INTERRUPT);
	return rc == EOK;
}
 
 
 
 /** @}
 */