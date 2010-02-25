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

static driver_list_t drivers_list;
static dev_tree_t device_tree;

/**
 * 
 * 
 * Driver's mutex must be locked.
 */
static void pass_devices_to_driver(driver_t *driver)
{
	

	
	
}

static void init_running_driver(driver_t *driver) 
{
	fibril_mutex_lock(&driver->driver_mutex);
	
	// pass devices which have been already assigned to the driver to the driver
	pass_devices_to_driver(driver);	
	
	// change driver's state to running
	driver->state = DRIVER_RUNNING;	
	
	fibril_mutex_unlock(&driver->driver_mutex);
}

/**
 * Register running driver.
 */
static driver_t * devman_driver_register(void)
{	
	printf(NAME ": devman_driver_register \n");
	
	ipc_call_t icall;
	ipc_callid_t iid = async_get_call(&icall);
	driver_t *driver = NULL;
	
	if (IPC_GET_METHOD(icall) != DEVMAN_DRIVER_REGISTER) {
		ipc_answer_0(iid, EREFUSED);
		return NULL;
	}
	
	char *drv_name = NULL;
	
	// Get driver name
	int rc = async_string_receive(&drv_name, DEVMAN_NAME_MAXLEN, NULL);	
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return NULL;
	}
	printf(NAME ": the %s driver is trying to register by the service.\n", drv_name);
	
	// Find driver structure
	driver = find_driver(&drivers_list, drv_name);
	
	free(drv_name);
	drv_name = NULL;
	
	if (NULL == driver) {
		printf(NAME ": no driver named %s was found.\n", drv_name);
		ipc_answer_0(iid, ENOENT);
		return NULL;
	}
	printf(NAME ": registering the running instance of the %s driver.\n", driver->name);
	
	// Create connection to the driver
	printf(NAME ":  creating connection to the %s driver.\n", driver->name);	
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);		
	if (IPC_GET_METHOD(call) != IPC_M_CONNECT_TO_ME) {
		ipc_answer_0(callid, ENOTSUP);
		ipc_answer_0(iid, ENOTSUP);
		return NULL;
	}
	
	fibril_mutex_lock(&driver->driver_mutex);
	assert(DRIVER_STARTING == driver->state);
	driver->phone = IPC_GET_ARG5(call);	
	fibril_mutex_unlock(&driver->driver_mutex);	
	
	printf(NAME ":  the %s driver was successfully registered as running.\n", driver->name);
	
	ipc_answer_0(callid, EOK);	
	
	ipc_answer_0(iid, EOK);
	
	return driver;
}

/** Function for handling connections to device manager.
 *
 */
static void devman_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	driver_t *driver = devman_driver_register();
	if (driver == NULL)
		return;
		
	init_running_driver(driver);
	
	ipc_callid_t callid;
	ipc_call_t call;
	bool cont = true;
	while (cont) {
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			continue;
		case DEVMAN_ADD_CHILD_DEVICE:
			// TODO add new device node to the device tree
			break;
		default:
			ipc_answer_0(callid, EINVAL); 
			break;
		}
	}
}

/** Function for handling connections to device manager.
 *
 */
static void devman_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	// Select interface 
	switch ((ipcarg_t) (IPC_GET_ARG1(*icall))) {
	case DEVMAN_DRIVER:
		devman_connection_driver(iid, icall);
		break;
	/*case DEVMAN_CLIENT:
		devmap_connection_client(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_DEVICE:
		// Connect client to selected device
		devmap_forward(iid, icall);
		break;*/
	default:
		/* No such interface */
		ipc_answer_0(iid, ENOENT);
	}
}

/** Initialize device manager internal structures.
 */
static bool devman_init()
{
	printf(NAME ": devman_init - looking for available drivers. \n");	
	
	// initialize list of available drivers
	init_driver_list(&drivers_list);
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
