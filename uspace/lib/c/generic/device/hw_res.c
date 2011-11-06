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

int hw_res_get_resource_list(async_sess_t *sess,
    hw_resource_list_t *hw_resources)
{
	sysarg_t count = 0;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_1(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_GET_RESOURCE_LIST, &count);
	
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}
	
	size_t size = count * sizeof(hw_resource_t);
	hw_resource_t *resources = (hw_resource_t *) malloc(size);
	if (resources == NULL) {
		// FIXME: This is protocol violation
		async_exchange_end(exch);
		return ENOMEM;
	}
	
	rc = async_data_read_start(exch, resources, size);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		free(resources);
		return rc;
	}
	
	hw_resources->resources = resources;
	hw_resources->count = count;
	
	return EOK;
}

bool hw_res_enable_interrupt(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_0(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_ENABLE_INTERRUPT);
	async_exchange_end(exch);
	
	return (rc == EOK);
}

/** @}
 */
