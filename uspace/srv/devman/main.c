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
#include <ipc/driver.h>
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
	
	if (NULL == driver) {
		printf(NAME ": no driver named %s was found.\n", drv_name);
		free(drv_name);
		drv_name = NULL;
		ipc_answer_0(iid, ENOENT);
		return NULL;
	}
	
	free(drv_name);
	drv_name = NULL;
	
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

/**
 * Receive device match ID from the device's parent driver and add it to the list of devices match ids.
 * 
 * @param match_ids the list of the device's match ids.
 * 
 * @return 0 on success, negative error code otherwise. 
 */
static int devman_receive_match_id(match_id_list_t *match_ids) {
	
	match_id_t *match_id = create_match_id();
	ipc_callid_t callid;
	ipc_call_t call;
	int rc = 0;
	
	callid = async_get_call(&call);
	if (DEVMAN_ADD_MATCH_ID != IPC_GET_METHOD(call)) {
		printf(NAME ": ERROR: devman_receive_match_id - invalid protocol.\n");
		ipc_answer_0(callid, EINVAL); 
		delete_match_id(match_id);
		return EINVAL;
	}
	
	if (NULL == match_id) {
		printf(NAME ": ERROR: devman_receive_match_id - failed to allocate match id.\n");
		ipc_answer_0(callid, ENOMEM);
		return ENOMEM;
	}
	
	ipc_answer_0(callid, EOK);
	
	match_id->score = IPC_GET_ARG1(call);
	
	rc = async_string_receive(&match_id->id, DEVMAN_NAME_MAXLEN, NULL);	
	if (EOK != rc) {
		delete_match_id(match_id);
		printf(NAME ": devman_receive_match_id - failed to receive match id string.\n");
		return rc;
	}
	
	list_append(&match_id->link, &match_ids->ids);
	
	printf(NAME ": received match id '%s', score = %d \n", match_id->id, match_id->score);
	return rc;
}

/**
 * Receive device match IDs from the device's parent driver 
 * and add them to the list of devices match ids.
 * 
 * @param match_count the number of device's match ids to be received.
 * @param match_ids the list of the device's match ids.
 * 
 * @return 0 on success, negative error code otherwise.
 */
static int devman_receive_match_ids(ipcarg_t match_count, match_id_list_t *match_ids) 
{	
	int ret = EOK;
	size_t i;
	for (i = 0; i < match_count; i++) {
		if (EOK != (ret = devman_receive_match_id(match_ids))) {
			return ret;
		}
	}
	return ret;
}

/** Handle child device registration. 
 * 
 * Child devices are registered by their parent's device driver.
 */
static void devman_add_child(ipc_callid_t callid, ipc_call_t *call)
{
	// printf(NAME ": devman_add_child\n");
	
	device_handle_t parent_handle = IPC_GET_ARG1(*call);
	ipcarg_t match_count = IPC_GET_ARG2(*call);
	
	node_t *parent =  find_dev_node(&device_tree, parent_handle);
	
	if (NULL == parent) {
		ipc_answer_0(callid, ENOENT);
		return;
	}	
	
	char *dev_name = NULL;
	int rc = async_string_receive(&dev_name, DEVMAN_NAME_MAXLEN, NULL);	
	if (EOK != rc) {
		ipc_answer_0(callid, rc);
		return;
	}
	// printf(NAME ": newly added child device's name is '%s'.\n", dev_name);
	
	node_t *node = create_dev_node();
	if (!insert_dev_node(&device_tree, node, dev_name, parent)) {
		delete_dev_node(node);
		ipc_answer_0(callid, ENOMEM);
		return;
	}	
	
	printf(NAME ": devman_add_child %s\n", node->pathname);
	
	devman_receive_match_ids(match_count, &node->match_ids);
	
	// return device handle to parent's driver
	ipc_answer_1(callid, EOK, node->handle);
	
	// try to find suitable driver and assign it to the device	
	assign_driver(node, &drivers_list);
}

/**
 * Initialize driver which has registered itself as running and ready.
 * 
 * The initialization is done in a separate fibril to avoid deadlocks 
 * (if the driver needed to be served by devman during the driver's initialization). 
 */
static int init_running_drv(void *drv)
{
	driver_t *driver = (driver_t *)drv;
	initialize_running_driver(driver);	
	printf(NAME ": the %s driver was successfully initialized. \n", driver->name);
	return 0;
}

/** Function for handling connections from a driver to the device manager.
 */
static void devman_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{	
	/* Accept the connection */
	ipc_answer_0(iid, EOK);
	
	driver_t *driver = devman_driver_register();
	if (NULL == driver)
		return;
	
	// Initialize the driver as running (e.g. pass assigned devices to it) in a separate fibril;
	// the separate fibril is used to enable the driver 
	// to use devman service during the driver's initialization.
	fid_t fid = fibril_create(init_running_drv, driver);
	if (fid == 0) {
		printf(NAME ": Error creating fibril for the initialization of the newly registered running driver.\n");
		return;
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
			devman_add_child(callid, &call);
			break;
		default:
			ipc_answer_0(callid, EINVAL); 
			break;
		}
	}
}

/** Find handle for the device instance identified by the device's path in the device tree.
 */
static void devman_device_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *pathname;
	
	/* Get fqdn */
	int rc = async_string_receive(&pathname, 0, NULL);
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return;
	}
	
	node_t * dev = find_dev_node_by_path(&device_tree, pathname); 
	
	free(pathname);

	if (NULL == dev) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	ipc_answer_1(iid, EOK, dev->handle);
}


/** Function for handling connections from a client to the device manager.
 */
static void devman_connection_client(ipc_callid_t iid, ipc_call_t *icall)
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
		case DEVMAN_DEVICE_GET_HANDLE:
			devman_device_get_handle(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
		}
	}	
}

static void devman_forward(ipc_callid_t iid, ipc_call_t *icall, bool drv_to_parent) {	
	
	device_handle_t handle = IPC_GET_ARG2(*icall);
	// printf(NAME ": devman_forward - trying to forward connection to device with handle %x.\n", handle);
	
	node_t *dev = find_dev_node(&device_tree, handle);
	if (NULL == dev) {
		printf(NAME ": devman_forward error - no device with handle %x was found.\n", handle);
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	driver_t *driver = NULL;
	
	if (drv_to_parent) {
		if (NULL != dev->parent) {
			driver = dev->parent->drv;		
		}
	} else if (DEVICE_USABLE == dev->state) {
		driver = dev->drv;		
		assert(NULL != driver);
	}
	
	if (NULL == driver) {	
		printf(NAME ": devman_forward error - the device is not in usable state.\n", handle);
		ipc_answer_0(iid, ENOENT);
		return;	
	}
	
	int method;	
	if (drv_to_parent) {
		method = DRIVER_DRIVER;
	} else {
		method = DRIVER_CLIENT;
	}
	
	if (driver->phone <= 0) {
		printf(NAME ": devman_forward: cound not forward to driver %s ", driver->name);
		printf("the driver's phone is %x).\n", driver->phone);
		return;
	}
	printf(NAME ": devman_forward: forward connection to device %s to driver %s.\n", dev->pathname, driver->name);
	ipc_forward_fast(iid, driver->phone, method, dev->handle, 0, IPC_FF_NONE);	
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
	case DEVMAN_CLIENT:
		devman_connection_client(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_DEVICE:
		// Connect client to selected device
		devman_forward(iid, icall, false);
		break;
	case DEVMAN_CONNECT_TO_PARENTS_DEVICE:
		// Connect client to selected device
		devman_forward(iid, icall, true);
		break;		
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