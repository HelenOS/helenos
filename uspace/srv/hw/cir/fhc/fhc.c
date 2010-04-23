/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup fhc
 * @{
 */ 

/**
 * @file	fhc.c
 * @brief	FHC bus controller driver.
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/bus.h>
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
#include <stdio.h>
#include <ipc/devmap.h>

#define NAME "fhc"

#define FHC_UART_INR	0x39	

#define FHC_UART_IMAP	0x0
#define FHC_UART_ICLR	0x4

static void *fhc_uart_phys;
static volatile uint32_t *fhc_uart_virt;
static size_t fhc_uart_size;

/** Handle one connection to fhc.
 *
 * @param iid		Hash of the request that opened the connection.
 * @param icall		Call data of the request that opened the connection.
 */
static void fhc_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0(iid, EOK);

	while (1) {
		int inr;
	
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case BUS_CLEAR_INTERRUPT:
			inr = IPC_GET_ARG1(call);
			switch (inr) {
			case FHC_UART_INR:
				fhc_uart_virt[FHC_UART_ICLR] = 0;
				ipc_answer_0(callid, EOK);
				break;
			default:
				ipc_answer_0(callid, ENOTSUP);
				break;
			}
			break;
		default:
			ipc_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Initialize the FHC driver.
 *
 * So far, the driver heavily depends on information provided by the kernel via
 * sysinfo. In the future, there should be a standalone FHC driver.
 */
static bool fhc_init(void)
{
	sysarg_t paddr;

	if ((sysinfo_get_value("fhc.uart.physical", &paddr) != EOK)
	    || (sysinfo_get_value("fhc.uart.size", &fhc_uart_size) != EOK)) {
		printf(NAME ": no FHC UART registers found\n");
		return false;
	}
	
	fhc_uart_phys = (void *) paddr;
	fhc_uart_virt = as_get_mappable_page(fhc_uart_size);
	
	int flags = AS_AREA_READ | AS_AREA_WRITE;
	int retval = physmem_map(fhc_uart_phys, (void *) fhc_uart_virt,
	    ALIGN_UP(fhc_uart_size, PAGE_SIZE) >> PAGE_WIDTH, flags);
	
	if (retval < 0) {
		printf(NAME ": Error mapping FHC UART registers\n");
		return false;
	}
	
	printf(NAME ": FHC UART registers at %p, %d bytes\n", fhc_uart_phys,
	    fhc_uart_size);
	
	async_set_client_connection(fhc_connection);
	ipcarg_t phonead;
	ipc_connect_to_me(PHONE_NS, SERVICE_FHC, 0, 0, &phonead);
	
	return true;
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS FHC bus controller driver\n");
	
	if (!fhc_init())
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Never reached */
	return 0;
}

/**
 * @}
 */ 
