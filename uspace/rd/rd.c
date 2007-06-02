/*
 * Copyright (c) 2007 Michal Konopa
 * Copyright (c) 2007 Martin Jelen
 * Copyright (c) 2007 Peter Majer
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

/** @addtogroup rd
 * @{
 */ 

/**
 * @file	rd.c
 * @brief	Initial RAM disk for HelenOS.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <align.h>
#include <bool.h>
#include <errno.h>
#include <async.h>
#include <align.h>
#include <async.h>
#include "rd.h"

static void *rd_addr;
static void *fs_addr;

static void rd_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	ipc_answer_fast(iid, 0, 0, 0);
	ipcarg_t offset;

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				ipc_answer_fast(callid, 0,0,0);
				return;
			case IPC_M_AS_AREA_SEND:
				ipc_answer_fast(callid, 0, (uintptr_t)fs_addr, 0);
				continue;
			case RD_READ_BLOCK:			
				offset = IPC_GET_ARG1(call);
				memcpy((void *)fs_addr, rd_addr+offset, BLOCK_SIZE);
				retval = EOK;
				break;
			default:
				retval = EINVAL;
		}
		ipc_answer_fast(callid, retval, 0, 0);
	}	
}


static bool rd_init(void)
{
	int retval, flags;

	size_t rd_size = sysinfo_value("rd.size");
	void * rd_ph_addr = (void *) sysinfo_value("rd.address.physical");
	
	if (rd_size == 0)
		return false;
	
	rd_addr = as_get_mappable_page(rd_size);
	
	flags = AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE;
	retval = physmem_map(rd_ph_addr, rd_addr, ALIGN_UP(rd_size, PAGE_SIZE) >> PAGE_WIDTH, flags);

	if (retval < 0)
		return false;
	
	size_t fs_size = ALIGN_UP(BLOCK_SIZE * sizeof(char), PAGE_SIZE);
	fs_addr = as_get_mappable_page(fs_size);

	return true;
}

int main(int argc, char **argv)
{
	if (rd_init()) {
		ipcarg_t phonead;
		
		async_set_client_connection(rd_connection);
		
		/* Register service at nameserver */
		if (ipc_connect_to_me(PHONE_NS, SERVICE_RD, 0, &phonead) != 0)
			return -1;
		
		async_manager();
		
		/* Never reached */
		return 0;
	}
	
	return -1;
}

/**
 * @}
 */ 
