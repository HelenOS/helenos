/*
 * Copyright (C) 2006 Martin Decky
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


static void rd_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	ipc_answer_fast(iid, 0, 0, 0);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			ipc_answer_fast(callid, 0,0,0);
			return;
		default:
			retval = EINVAL;
		}
		ipc_answer_fast(callid, retval, 0, 0);
	}	
}


static bool rd_init(void)
{
	size_t rd_size = sysinfo_value("rd.size");
	void * rd_ph_addr = (void *) sysinfo_value("rd.address.physical");
	int rd_color = (int) sysinfo_value("rd.address.color");
	
	if (rd_size == 0)
		return false;
	
	void * rd_addr = as_get_mappable_page(rd_size, rd_color);
	
	physmem_map(rd_ph_addr, rd_addr, ALIGN_UP(rd_size, PAGE_SIZE) >> PAGE_WIDTH, AS_AREA_READ | AS_AREA_WRITE);
	
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
