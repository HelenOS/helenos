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

#include <inttypes.h>
#include <assert.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <ipc/devman.h>
#include <ipc/driver.h>
#include <thread.h>
#include <devmap.h>

#include "devman.h"

#define DRIVER_DEFAULT_STORE  "/drv"

static driver_list_t drivers_list;
static dev_tree_t device_tree;
static class_list_t class_list;

/** Register running driver. */
static driver_t *devman_driver_register(void)
{
	ipc_call_t icall;
	ipc_callid_t iid;
	driver_t *driver = NULL;

	printf(NAME ": devman_driver_register \n");
	
	iid = async_get_call(&icall);
	if (IPC_GET_IMETHOD(icall) != DEVMAN_DRIVER_REGISTER) {
		async_answer_0(iid, EREFUSED);
		return NULL;
	}
	
	char *drv_name = NULL;
	
	/* Get driver name. */
	int rc = async_data_write_accept((void **) &drv_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return NULL;
	}

	printf(NAME ": the %s driver is trying to register by the service.\n",
	    drv_name);
	
	/* Find driver structure. */
	driver = find_driver(&drivers_list, drv_name);
	
	if (driver == NULL) {
		printf(NAME ": no driver named %s was found.\n", drv_name);
		free(drv_name);
		drv_name = NULL;
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	free(drv_name);
	drv_name = NULL;
	
	/* Create connection to the driver. */
	printf(NAME ":  creating connection to the %s driver.\n", driver->name);
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);
	if (IPC_GET_IMETHOD(call) != IPC_M_CONNECT_TO_ME) {
		async_answer_0(callid, ENOTSUP);
		async_answer_0(iid, ENOTSUP);
		return NULL;
	}
	
	/* Remember driver's phone. */
	set_driver_phone(driver, IPC_GET_ARG5(call));
	
	printf(NAME ": the %s driver was successfully registered as running.\n",
	    driver->name);
	
	async_answer_0(callid, EOK);
	async_answer_0(iid, EOK);
	
	return driver;
}

/** Receive device match ID from the device's parent driver and add it to the
 * list of devices match ids.
 *
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, negative error code otherwise.
 */
static int devman_receive_match_id(match_id_list_t *match_ids)
{
	match_id_t *match_id = create_match_id();
	ipc_callid_t callid;
	ipc_call_t call;
	int rc = 0;
	
	callid = async_get_call(&call);
	if (DEVMAN_ADD_MATCH_ID != IPC_GET_IMETHOD(call)) {
		printf(NAME ": ERROR: devman_receive_match_id - invalid "
		    "protocol.\n");
		async_answer_0(callid, EINVAL); 
		delete_match_id(match_id);
		return EINVAL;
	}
	
	if (match_id == NULL) {
		printf(NAME ": ERROR: devman_receive_match_id - failed to "
		    "allocate match id.\n");
		async_answer_0(callid, ENOMEM);
		return ENOMEM;
	}
	
	async_answer_0(callid, EOK);
	
	match_id->score = IPC_GET_ARG1(call);
	
	char *match_id_str;
	rc = async_data_write_accept((void **) &match_id_str, true, 0, 0, 0, 0);
	match_id->id = match_id_str;
	if (rc != EOK) {
		delete_match_id(match_id);
		printf(NAME ": devman_receive_match_id - failed to receive "
		    "match id string.\n");
		return rc;
	}
	
	list_append(&match_id->link, &match_ids->ids);
	
	printf(NAME ": received match id '%s', score = %d \n",
	    match_id->id, match_id->score);
	return rc;
}

/** Receive device match IDs from the device's parent driver and add them to the
 * list of devices match ids.
 *
 * @param match_count	The number of device's match ids to be received.
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, negative error code otherwise.
 */
static int devman_receive_match_ids(sysarg_t match_count,
    match_id_list_t *match_ids)
{
	int ret = EOK;
	size_t i;
	
	for (i = 0; i < match_count; i++) {
		if (EOK != (ret = devman_receive_match_id(match_ids)))
			return ret;
	}
	return ret;
}

static int assign_driver_fibril(void *arg)
{
	dev_node_t *dev_node = (dev_node_t *) arg;
	assign_driver(dev_node, &drivers_list, &device_tree);
	return EOK;
}

/** Handle function registration.
 *
 * Child devices are registered by their parent's device driver.
 */
static void devman_add_function(ipc_callid_t callid, ipc_call_t *call)
{
	fun_type_t ftype = (fun_type_t) IPC_GET_ARG1(*call);
	devman_handle_t dev_handle = IPC_GET_ARG2(*call);
	sysarg_t match_count = IPC_GET_ARG3(*call);
	dev_tree_t *tree = &device_tree;
	
	fibril_rwlock_write_lock(&tree->rwlock);

	dev_node_t *dev = NULL;
	dev_node_t *pdev = find_dev_node_no_lock(&device_tree, dev_handle);
	
	if (pdev == NULL) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	if (ftype != fun_inner && ftype != fun_exposed) {
		/* Unknown function type */
		printf(NAME ": Error, unknown function type provided by driver!\n");

		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	char *fun_name = NULL;
	int rc = async_data_write_accept((void **)&fun_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, rc);
		return;
	}
	
	fun_node_t *fun = create_fun_node();
	if (!insert_fun_node(&device_tree, fun, fun_name, pdev)) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		delete_fun_node(fun);
		async_answer_0(callid, ENOMEM);
		return;
	}

	if (ftype == fun_inner) {
		dev = create_dev_node();
		if (dev == NULL) {
			fibril_rwlock_write_unlock(&tree->rwlock);
			delete_fun_node(fun);
			async_answer_0(callid, ENOMEM);
			return;
		}

		insert_dev_node(tree, dev, fun);
	}

	fibril_rwlock_write_unlock(&tree->rwlock);
	
	printf(NAME ": devman_add_function %s\n", fun->pathname);
	
	devman_receive_match_ids(match_count, &fun->match_ids);

	if (ftype == fun_inner) {
		assert(dev != NULL);
		/*
		 * Try to find a suitable driver and assign it to the device.  We do
		 * not want to block the current fibril that is used for processing
		 * incoming calls: we will launch a separate fibril to handle the
		 * driver assigning. That is because assign_driver can actually include
		 * task spawning which could take some time.
		 */
		fid_t assign_fibril = fibril_create(assign_driver_fibril, dev);
		if (assign_fibril == 0) {
			/*
			 * Fallback in case we are out of memory.
			 * Probably not needed as we will die soon anyway ;-).
			 */
			(void) assign_driver_fibril(fun);
		} else {
			fibril_add_ready(assign_fibril);
		}
	} else {
		devmap_register_tree_function(fun, tree);
	}
	
	/* Return device handle to parent's driver. */
	async_answer_1(callid, EOK, fun->handle);
}

static void devmap_register_class_dev(dev_class_info_t *cli)
{
	/* Create devmap path and name for the device. */
	char *devmap_pathname = NULL;

	asprintf(&devmap_pathname, "%s/%s%c%s", DEVMAP_CLASS_NAMESPACE,
	    cli->dev_class->name, DEVMAP_SEPARATOR, cli->dev_name);
	if (devmap_pathname == NULL)
		return;
	
	/*
	 * Register the device by the device mapper and remember its devmap
	 * handle.
	 */
	devmap_device_register_with_iface(devmap_pathname,
	    &cli->devmap_handle, DEVMAN_CONNECT_FROM_DEVMAP);
	
	/*
	 * Add device to the hash map of class devices registered by device
	 * mapper.
	 */
	class_add_devmap_function(&class_list, cli);
	
	free(devmap_pathname);
}

static void devman_add_function_to_class(ipc_callid_t callid, ipc_call_t *call)
{
	devman_handle_t handle = IPC_GET_ARG1(*call);
	
	/* Get class name. */
	char *class_name;
	int rc = async_data_write_accept((void **) &class_name, true,
	    0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}	
	
	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(callid, ENOENT);
		return;
	}
	
	dev_class_t *cl = get_dev_class(&class_list, class_name);
	dev_class_info_t *class_info = add_function_to_class(fun, cl, NULL);
	
	/* Register the device's class alias by devmapper. */
	devmap_register_class_dev(class_info);
	
	printf(NAME ": function'%s' added to class '%s', class name '%s' was "
	    "asigned to it\n", fun->pathname, class_name, class_info->dev_name);

	async_answer_0(callid, EOK);
}

/** Initialize driver which has registered itself as running and ready.
 *
 * The initialization is done in a separate fibril to avoid deadlocks (if the
 * driver needed to be served by devman during the driver's initialization).
 */
static int init_running_drv(void *drv)
{
	driver_t *driver = (driver_t *) drv;
	
	initialize_running_driver(driver, &device_tree);
	printf(NAME ": the %s driver was successfully initialized. \n",
	    driver->name);
	return 0;
}

/** Function for handling connections from a driver to the device manager. */
static void devman_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept the connection. */
	async_answer_0(iid, EOK);
	
	driver_t *driver = devman_driver_register();
	if (driver == NULL)
		return;
	
	/*
	 * Initialize the driver as running (e.g. pass assigned devices to it)
	 * in a separate fibril; the separate fibril is used to enable the
	 * driver to use devman service during the driver's initialization.
	 */
	fid_t fid = fibril_create(init_running_drv, driver);
	if (fid == 0) {
		printf(NAME ": Error creating fibril for the initialization of "
		    "the newly registered running driver.\n");
		return;
	}
	fibril_add_ready(fid);
	
	ipc_callid_t callid;
	ipc_call_t call;
	bool cont = true;
	while (cont) {
		callid = async_get_call(&call);
		
		switch (IPC_GET_IMETHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			continue;
		case DEVMAN_ADD_FUNCTION:
			devman_add_function(callid, &call);
			break;
		case DEVMAN_ADD_DEVICE_TO_CLASS:
			devman_add_function_to_class(callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL); 
			break;
		}
	}
}

/** Find handle for the device instance identified by the device's path in the
 * device tree. */
static void devman_function_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *pathname;
	
	int rc = async_data_write_accept((void **) &pathname, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	fun_node_t *fun = find_fun_node_by_path(&device_tree, pathname);
	
	free(pathname);

	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	async_answer_1(iid, EOK, fun->handle);
}


/** Function for handling connections from a client to the device manager. */
static void devman_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection. */
	async_answer_0(iid, EOK);
	
	bool cont = true;
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch (IPC_GET_IMETHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			continue;
		case DEVMAN_DEVICE_GET_HANDLE:
			devman_function_get_handle(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}

static void devman_forward(ipc_callid_t iid, ipc_call_t *icall,
    bool drv_to_parent)
{
	devman_handle_t handle = IPC_GET_ARG2(*icall);
	devman_handle_t fwd_h;
	fun_node_t *fun = NULL;
	dev_node_t *dev = NULL;
	
	fun = find_fun_node(&device_tree, handle);
	if (fun == NULL)
		dev = find_dev_node(&device_tree, handle);
	else
		dev = fun->dev;

	if (fun == NULL && dev == NULL) {
		printf(NAME ": devman_forward error - no device or function with "
		    "handle %" PRIun " was found.\n", handle);
		async_answer_0(iid, ENOENT);
		return;
	}

	if (fun == NULL && !drv_to_parent) {
		printf(NAME ": devman_forward error - cannot connect to "
		    "handle %" PRIun ", refers to a device.\n", handle);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	driver_t *driver = NULL;
	
	if (drv_to_parent) {
		/* Connect to parent function of a device (or device function). */
		if (dev->pfun->dev != NULL)
			driver = dev->pfun->dev->drv;
		fwd_h = dev->pfun->handle;
	} else if (dev->state == DEVICE_USABLE) {
		/* Connect to the specified function */
		driver = dev->drv;
		assert(driver != NULL);

		fwd_h = handle;
	}
	
	if (driver == NULL) {
		printf(NAME ": devman_forward error - the device is not in %" PRIun
		    " usable state.\n", handle);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	int method;
	if (drv_to_parent)
		method = DRIVER_DRIVER;
	else
		method = DRIVER_CLIENT;
	
	if (driver->phone <= 0) {
		printf(NAME ": devman_forward: cound not forward to driver %s ",
		    driver->name);
		printf("the driver's phone is %" PRIun ").\n", driver->phone);
		async_answer_0(iid, EINVAL);
		return;
	}

	if (fun != NULL) {
		printf(NAME ": devman_forward: forward connection to function %s to "
		    "driver %s.\n", fun->pathname, driver->name);
	} else {
		printf(NAME ": devman_forward: forward connection to device %s to "
		    "driver %s.\n", dev->pfun->pathname, driver->name);
	}

	async_forward_fast(iid, driver->phone, method, fwd_h, 0, IPC_FF_NONE);
}

/** Function for handling connections from a client forwarded by the device
 * mapper to the device manager. */
static void devman_connection_devmapper(ipc_callid_t iid, ipc_call_t *icall)
{
	devmap_handle_t devmap_handle = IPC_GET_ARG2(*icall);
	fun_node_t *fun;
	dev_node_t *dev;

	fun = find_devmap_tree_function(&device_tree, devmap_handle);
	if (fun == NULL)
		fun = find_devmap_class_function(&class_list, devmap_handle);
	
	if (fun == NULL || fun->dev->drv == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	dev = fun->dev;
	
	if (dev->state != DEVICE_USABLE || dev->drv->phone <= 0) {
		async_answer_0(iid, EINVAL);
		return;
	}
	
	async_forward_fast(iid, dev->drv->phone, DRIVER_CLIENT, dev->handle, 0,
	    IPC_FF_NONE);
	printf(NAME ": devman_connection_devmapper: forwarded connection to "
	    "device %s to driver %s.\n", fun->pathname, dev->drv->name);
}

/** Function for handling connections to device manager. */
static void devman_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Select interface. */
	switch ((sysarg_t) (IPC_GET_ARG1(*icall))) {
	case DEVMAN_DRIVER:
		devman_connection_driver(iid, icall);
		break;
	case DEVMAN_CLIENT:
		devman_connection_client(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_DEVICE:
		/* Connect client to selected device. */
		devman_forward(iid, icall, false);
		break;
	case DEVMAN_CONNECT_FROM_DEVMAP:
		/* Someone connected through devmap node. */
		devman_connection_devmapper(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_PARENTS_DEVICE:
		/* Connect client to selected device. */
		devman_forward(iid, icall, true);
		break;
	default:
		/* No such interface */
		async_answer_0(iid, ENOENT);
	}
}

/** Initialize device manager internal structures. */
static bool devman_init(void)
{
	printf(NAME ": devman_init - looking for available drivers.\n");
	
	/* Initialize list of available drivers. */
	init_driver_list(&drivers_list);
	if (lookup_available_drivers(&drivers_list,
	    DRIVER_DEFAULT_STORE) == 0) {
		printf(NAME " no drivers found.");
		return false;
	}

	printf(NAME ": devman_init  - list of drivers has been initialized.\n");

	/* Create root device node. */
	if (!init_device_tree(&device_tree, &drivers_list)) {
		printf(NAME " failed to initialize device tree.");
		return false;
	}

	init_class_list(&class_list);
	
	/*
	 * !!! devman_connection ... as the device manager is not a real devmap
	 * driver (it uses a completely different ipc protocol than an ordinary
	 * devmap driver) forwarding a connection from client to the devman by
	 * devmapper would not work.
	 */
	devmap_driver_register(NAME, devman_connection);
	
	return true;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Manager\n");

	if (!devman_init()) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}
	
	/* Set a handler of incomming connections. */
	async_set_client_connection(devman_connection);

	/* Register device manager at naming service. */
	if (service_register(SERVICE_DEVMAN) != EOK)
		return -1;

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Never reached. */
	return 0;
}

/** @}
 */
