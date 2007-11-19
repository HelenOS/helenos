/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup klog KLog
 * @brief	HelenOS KLog
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <ipc/ipc.h>
#include <async.h>
#include <ipc/services.h>
#include <as.h>
#include <sysinfo.h>

/* Pointer to klog area */
static char *klog;

static void interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
	int i;
	
	async_serialize_start();
	for (i=0; klog[i + IPC_GET_ARG1(*call)] && i < IPC_GET_ARG2(*call); i++)
		putchar(klog[i + IPC_GET_ARG1(*call)]);
	putchar('\n');
	async_serialize_end();
}

int main(int argc, char *argv[])
{
	int res;
	void *mapping;

	printf("Kernel console output.\n");
	
	mapping = as_get_mappable_page(PAGE_SIZE);
	res = ipc_call_sync_3_0(PHONE_NS, IPC_M_AS_AREA_RECV,
	    (sysarg_t) mapping, PAGE_SIZE, SERVICE_MEM_KLOG);
	if (res) {
		printf("Failed to initialize klog memarea\n");
		_exit(1);
	}
	klog = mapping;

	int inr = sysinfo_value("klog.inr");
	int devno = sysinfo_value("klog.devno");
	if (ipc_register_irq(inr, devno, 0, NULL)) {
		printf("Error registering for klog service.\n");
		return 0;
	}

	async_set_interrupt_received(interrupt_received);

	async_manager();

	return 0;
}

/** @}
 */
