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

/**
 * @defgroup devman Device manager.
 * @brief HelenOS device manager.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <ipc/devman.h>

#include "devman.h"

#define DRIVER_DEFAULT_STORE  "/srv/drivers"

LIST_INITIALIZE(drivers_list);
static dev_tree_t device_tree;


/** Function for handling connections to device manager.
 *
 */
static void devman_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	while (1) {
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case DEVMAN_DRIVER_REGISTER:
			// TODO register running driver instance - remember driver's phone and lookup devices to which it has been assigned
			break;
		case DEVMAN_ADD_CHILD_DEVICE:
			// TODO add new device node to the device tree
			break;
		default:
			ipc_answer_0(callid, EINVAL); 
			break;
		}
	}
}

/** Initialize device manager internal structures.
 */
static bool devman_init()
{
	printf(NAME ": devman_init - looking for available drivers. \n");
	
	// initialize list of available drivers
	if (0 == lookup_available_drivers(&drivers_list, DRIVER_DEFAULT_STORE)) {
		printf(NAME " no drivers found.");
		return false;
	}
	printf(NAME ": devman_init  - list of drivers has been initialized. \n");

	// create root device node 
	if (!init_device_tree(&device_tree, &drivers_list)) {
		printf(NAME " failed to initialize device tree.");
		return false;		
	}

	return true;
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Manager\n");

	if (!devman_init()) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}
	
	// Set a handler of incomming connections
	async_set_client_connection(devman_connection);

	// Register device manager at naming service
	ipcarg_t phonead;
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAN, 0, 0, &phonead) != 0)
		return -1;

	printf(NAME ": Accepting connections\n");
	async_manager();

	// Never reached
	return 0;
}
