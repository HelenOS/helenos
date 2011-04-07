/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup apic
 * @{
 */

/**
 * @file apic.c
 * @brief APIC driver.
 */

#include <ipc/services.h>
#include <ipc/irc.h>
#include <ipc/ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <align.h>
#include <bool.h>
#include <errno.h>
#include <async.h>
#include <align.h>
#include <async.h>
#include <stdio.h>
#include <ipc/devmap.h>

#define NAME  "apic"

static int apic_enable_irq(sysarg_t irq)
{
	// FIXME: TODO
	return ENOTSUP;
}

/** Handle one connection to APIC.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 *
 */
static void apic_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	while (true) {
		callid = async_get_call(&call);
		
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(callid, apic_enable_irq(IPC_GET_ARG1(call)));
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(callid, EOK);
			break;
		case IPC_M_PHONE_HUNGUP:
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Initialize the APIC driver.
 *
 */
static bool apic_init(void)
{
	sysarg_t apic;
	
	if ((sysinfo_get_value("apic", &apic) != EOK) || (!apic)) {
		printf(NAME ": No APIC found\n");
		return false;
	}
	
	async_set_client_connection(apic_connection);
	service_register(SERVICE_IRC);
	
	return true;
}

int main(int argc, char **argv)
{
	printf(NAME ": HelenOS APIC driver\n");
	
	if (!apic_init())
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
