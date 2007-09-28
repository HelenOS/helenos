/*
 * Copyright (c) 2007 Josef Cejka
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

/**
 * @defgroup devmap Device mapper.
 * @brief	HelenOS device mapper.
 * @{
 */ 

/** @file
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>

#include "devmap.h"

/** Initialize device mapper. 
 *
 *
 */
static int devmap_init()
{
	/* */

	return 0;
}


/** Function for handling connections to devmap 
 *
 */
static void
devmap_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	ipc_answer_fast(iid, EOK, 0, 0); /* Accept connection */

	while (1) {
		callid = async_get_call(&call);

 		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
				/* TODO: if its a device connection, remove it from table */
			return; /* Exit thread */

		case DEVMAP_REGISTER:
			/* TODO: */
		case DEVMAP_UNREGISTER:
			/* TODO: */
		case DEVMAP_CONNECT_TO_DEVICE:
			/* TODO: */
			retval = 0;
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_fast(callid, retval, 0, 0);
	}
 		
}


int main(int argc, char *argv[])
{

	ipcarg_t phonead;

	printf("DevMap: HelenOS device mapper.\n");

	if (devmap_init() != 0) {
		printf("Error while initializing DevMap service.");
		return -1;
	}

		/* Set a handler of incomming connections */
	async_set_client_connection(devmap_client_connection);

		/* Register device mapper at naming service */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAP, 0, &phonead) != 0) 
		return -1;
	
	async_manager();
	/* Never reached */
	return 0;
}

/** 
 * @}
 */

