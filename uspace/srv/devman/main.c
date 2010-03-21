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
#include <thread.h>

#include "devman.h"

#define DRIVER_DEFAULT_STORE  "/srv/drivers"

static driver_list_t drivers_list;
static dev_tree_t device_tree;

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
	
	// Create connection to the driver
	printf(NAME ":  creating connection to the %s driver.\n", driver->name);	
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);		
	if (IPC_GET_METHOD(call) != IPC_M_CONNECT_TO_ME) {
		ipc_answer_0(callid, ENOTSUP);
		ipc_answer_0(iid, ENOTSUP);
		return NULL;
	}
	
	// remember driver's phone
	set_driver_phone(driver, IPC_GET_ARG5(call));
	
	printf(NAME ":  the %s driver was successfully registered as running.\n", driver->name);
	
	ipc_answer_0(callid, EOK);	
	
	ipc_answer_0(iid, EOK);
	
	return driver;
}

static void devman_add_child(ipc_callid_t callid, ipc_call_t *call, driver_t *driver)
{
	printf(NAME ": devman_add_child\n");

	device_handle_t parent_handle = IPC_GET_ARG1(*call);
	node_t *parent =  find_dev_node(&device_tree, parent_handle);
	
	if (NULL == parent) {
		ipc_answer_0(callid, ENOENT);
		return;
	}
	
	char *dev_name = NULL;
	int rc = async_string_receive(&dev_name, DEVMAN_NAME_MAXLEN, NULL);	
	if (rc != EOK) {
		ipc_answer_0(callid, rc);
		return;
	}
	printf(NAME ": newly added child device's name is '%s'.\n", dev_name);
	
	node_t *node = create_dev_node();
	if (!insert_dev_node(&device_tree, node, dev_name, parent)) {
		delete_dev_node(node);
		ipc_answer_0(callid, ENOMEM);
		return;
	}	
	
	// TODO match ids
	
	// return device handle to parent's driver
	ipc_answer_1(callid, EOK, node->handle);
	
	// try to find suitable driver and assign it to the device	
	assign_driver(node, &drivers_list);
}

static int init_running_drv(void *drv)
{
	driver_t *driver = (driver_t *)drv;
	initialize_running_driver(driver);	
	printf(NAME ": the %s driver was successfully initialized. \n", driver->name);
	return 0;
}

/** Function for handling connections to device manager.
 *
 */
static void devman_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	driver_t *driver = devman_driver_register();
	if (NULL == driver)
		return;
	
	fid_t fid = fibril_create(init_running_drv, driver);
	if (fid == 0) {
		printf(NAME ": Error creating fibril for the initialization of the newly registered running driver.\n");
		exit(1);
	}
	fibril_add_ready(fid);
	
	/*thread_id_t tid;
	if (0 != thread_create(init_running_drv, driver, "init_running_drv", &tid)) {
		printf(NAME ": failed to start the initialization of the newly registered running driver.\n");
	}*/
	
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
			devman_add_child(callid, &call, driver);
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

/** @}
 */