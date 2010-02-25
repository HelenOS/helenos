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
 * @defgroup root device driver.
 * @brief HelenOS root device driver.
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
#include <ctype.h>

#include <devman.h>
#include <ipc/devman.h>


////////////////////////////////////////
// rudiments of generic driver support
// TODO - create library(ies) for this functionality
////////////////////////////////////////

typedef enum {
	DRIVER_DEVMAN = 1,
	DRIVER_CLIENT,
	DRIVER_DRIVER
} driver_interface_t;

typedef struct device {
	int parent_handle;
	ipcarg_t parent_phone;	
	// TODO add more items - parent bus type etc.
	int handle;	
} device_t;

typedef struct driver_ops {	
	bool (*add_device)(device_t *dev);
	// TODO add other generic driver operations
} driver_ops_t;

typedef struct driver {
	const char *name;
	driver_ops_t *driver_ops;
} driver_t;


static driver_t *driver;


static void driver_connection_devman(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	ipc_answer_0(iid, EOK);
	
	bool cont = true;
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			continue;
		case DRIVER_ADD_DEVICE:
			// TODO
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
		}
	}
	
}

static void driver_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	// TODO later
}

static void driver_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	// TODO later
}




/** Function for handling connections to device driver.
 *
 */
static void driver_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	/* Select interface */
	switch ((ipcarg_t) (IPC_GET_ARG1(*icall))) {
	case DRIVER_DEVMAN:
		// handle PnP events from device manager
		driver_connection_devman(iid, icall);
		break;
	case DRIVER_DRIVER:
		// handle request from drivers of child devices
		driver_connection_driver(iid, icall);
		break;
	case DRIVER_CLIENT:
		// handle requests from client applications 
		driver_connection_client(iid, icall);
		break;

	default:
		/* No such interface */ 
		ipc_answer_0(iid, ENOENT);
	}
}



int driver_main(driver_t *drv) 
{
	// remember driver structure - driver_ops will be called by generic handler for incoming connections
	driver = drv;
	
	// register driver by device manager with generic handler for incoming connections
	printf("%s: sending registration request to devman.\n", driver->name);
	devman_driver_register(driver->name, driver_connection);		

	async_manager();

	// Never reached
	return 0;
	
}

////////////////////////////////////////
// ROOT driver 
////////////////////////////////////////

#define NAME "root"



bool root_add_device(device_t *dev) 
{
	// TODO add root device and register its children
	return true;
}

static driver_ops_t root_ops = {
	.add_device = &root_add_device
};

static driver_t root_driver = {
	.name = NAME,
	.driver_ops = &root_ops
};

bool root_init() 
{
	// TODO  driver initialization	
	return true;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS root device driver\n");
	if (!root_init()) {
		printf(NAME ": Error while initializing driver.\n");
		return -1;
	}
	
	return driver_main(&root_driver);
}
