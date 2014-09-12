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

/** @addtogroup obio
 * @{
 */

/**
 * @file obio.c
 * @brief OBIO driver.
 *
 * OBIO is a short for on-board I/O. On UltraSPARC IIi and systems with U2P,
 * there is a piece of the root PCI bus controller address space, which
 * contains interrupt mapping and clear registers for all on-board devices.
 * Although UltraSPARC IIi and U2P are different in general, these registers can
 * be found at the same addresses.
 */

#include <ipc/services.h>
#include <ipc/irc.h>
#include <ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <align.h>
#include <stdbool.h>
#include <errno.h>
#include <async.h>
#include <align.h>
#include <async.h>
#include <stdio.h>
#include <ipc/loc.h>

#define NAME "obio"

#define OBIO_SIZE	0x1898	

#define OBIO_IMR_BASE	0x200
#define OBIO_IMR(ino)	(OBIO_IMR_BASE + ((ino) & INO_MASK))

#define OBIO_CIR_BASE	0x300
#define OBIO_CIR(ino)	(OBIO_CIR_BASE + ((ino) & INO_MASK))

#define INO_MASK	0x1f

static uintptr_t base_phys;
static volatile uint64_t *base_virt = (volatile uint64_t *) AS_AREA_ANY;

/** Handle one connection to obio.
 *
 * @param iid		Hash of the request that opened the connection.
 * @param icall		Call data of the request that opened the connection.
 * @param arg		Local argument.
 */
static void obio_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);

	while (1) {
		int inr;
	
		callid = async_get_call(&call);
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			/* Noop */
			async_answer_0(callid, EOK);
			break;
		case IRC_CLEAR_INTERRUPT:
			inr = IPC_GET_ARG1(call);
			base_virt[OBIO_CIR(inr & INO_MASK)] = 0;
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Initialize the OBIO driver.
 *
 * So far, the driver heavily depends on information provided by the kernel via
 * sysinfo. In the future, there should be a standalone OBIO driver.
 */
static bool obio_init(void)
{
	sysarg_t paddr;
	
	if (sysinfo_get_value("obio.base.physical", &paddr) != EOK) {
		printf("%s: No OBIO registers found\n", NAME);
		return false;
	}
	
	base_phys = (uintptr_t) paddr;
	
	int flags = AS_AREA_READ | AS_AREA_WRITE;
	int retval = physmem_map(base_phys,
	    ALIGN_UP(OBIO_SIZE, PAGE_SIZE) >> PAGE_WIDTH, flags,
	    (void *) &base_virt);
	
	if (retval < 0) {
		printf("%s: Error mapping OBIO registers\n", NAME);
		return false;
	}
	
	printf("%s: OBIO registers with base at %zu\n", NAME, base_phys);
	
	async_set_client_connection(obio_connection);
	service_register(SERVICE_IRC);
	
	return true;
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS OBIO driver\n", NAME);
	
	if (!obio_init())
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
