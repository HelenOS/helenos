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
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <io/log.h>
#include <ipc/devman.h>
#include <ipc/driver.h>
#include <thread.h>
#include <loc.h>

#include "devman.h"

#define DRIVER_DEFAULT_STORE  "/drv"

static driver_list_t drivers_list;
static dev_tree_t device_tree;

/** Register running driver. */
static driver_t *devman_driver_register(void)
{
	ipc_call_t icall;
	ipc_callid_t iid;
	driver_t *driver = NULL;

	log_msg(LVL_DEBUG, "devman_driver_register");
	
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

	log_msg(LVL_DEBUG, "The `%s' driver is trying to register.",
	    drv_name);
	
	/* Find driver structure. */
	driver = find_driver(&drivers_list, drv_name);
	if (driver == NULL) {
		log_msg(LVL_ERROR, "No driver named `%s' was found.", drv_name);
		free(drv_name);
		drv_name = NULL;
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	free(drv_name);
	drv_name = NULL;
	
	fibril_mutex_lock(&driver->driver_mutex);
	
	if (driver->sess) {
		/* We already have a connection to the driver. */
		log_msg(LVL_ERROR, "Driver '%s' already started.\n",
		    driver->name);
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(iid, EEXISTS);
		return NULL;
	}
	
	switch (driver->state) {
	case DRIVER_NOT_STARTED:
		/* Somebody started the driver manually. */
		log_msg(LVL_NOTE, "Driver '%s' started manually.\n",
		    driver->name);
		driver->state = DRIVER_STARTING;
		break;
	case DRIVER_STARTING:
		/* The expected case */
		break;
	case DRIVER_RUNNING:
		/* Should not happen since we do not have a connected session */
		assert(false);
	}
	
	/* Create connection to the driver. */
	log_msg(LVL_DEBUG, "Creating connection to the `%s' driver.",
	    driver->name);
	driver->sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (!driver->sess) {
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(iid, ENOTSUP);
		return NULL;
	}
	
	fibril_mutex_unlock(&driver->driver_mutex);
	
	log_msg(LVL_NOTE,
	    "The `%s' driver was successfully registered as running.",
	    driver->name);
	
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
		log_msg(LVL_ERROR, 
		    "Invalid protocol when trying to receive match id.");
		async_answer_0(callid, EINVAL); 
		delete_match_id(match_id);
		return EINVAL;
	}
	
	if (match_id == NULL) {
		log_msg(LVL_ERROR, "Failed to allocate match id.");
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
		log_msg(LVL_ERROR, "Failed to receive match id string: %s.",
		    str_error(rc));
		return rc;
	}
	
	list_append(&match_id->link, &match_ids->ids);
	
	log_msg(LVL_DEBUG, "Received match id `%s', score %d.",
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
		log_msg(LVL_ERROR, 
		    "Unknown function type %d provided by driver.",
		    (int) ftype);

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
	
	/* Check that function with same name is not there already. */
	if (find_fun_node_in_device(pdev, fun_name) != NULL) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, EEXISTS);
		printf(NAME ": Warning, driver tried to register `%s' twice.\n",
		    fun_name);
		free(fun_name);
		return;
	}
	
	fun_node_t *fun = create_fun_node();
	fun->ftype = ftype;
	
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
	
	log_msg(LVL_DEBUG, "devman_add_function(fun=\"%s\")", fun->pathname);
	
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
		loc_register_tree_function(fun, tree);
	}
	
	/* Return device handle to parent's driver. */
	async_answer_1(callid, EOK, fun->handle);
}

static void devman_add_function_to_cat(ipc_callid_t callid, ipc_call_t *call)
{
	devman_handle_t handle = IPC_GET_ARG1(*call);
	category_id_t cat_id;
	int rc;
	
	/* Get category name. */
	char *cat_name;
	rc = async_data_write_accept((void **) &cat_name, true,
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
	
	rc = loc_category_get_id(cat_name, &cat_id, IPC_FLAG_BLOCKING);
	if (rc == EOK) {
		loc_service_add_to_cat(fun->service_id, cat_id);
	} else {
		log_msg(LVL_ERROR, "Failed adding function `%s' to category "
		    "`%s'.", fun->pathname, cat_name);
	}
	
	log_msg(LVL_NOTE, "Function `%s' added to category `%s'.",
	    fun->pathname, cat_name);

	async_answer_0(callid, EOK);
}

/** Remove function. */
static void devman_remove_function(ipc_callid_t callid, ipc_call_t *call)
{
	devman_handle_t fun_handle = IPC_GET_ARG1(*call);
	dev_tree_t *tree = &device_tree;
	int rc;
	
	fibril_rwlock_write_lock(&tree->rwlock);
	
	fun_node_t *fun = find_fun_node_no_lock(&device_tree, fun_handle);
	if (fun == NULL) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	log_msg(LVL_DEBUG, "devman_remove_function(fun='%s')", fun->pathname);
	
	if (fun->ftype == fun_inner) {
		/* Handle possible descendants */
		/* TODO */
		log_msg(LVL_WARN, "devman_remove_function(): not handling "
		    "descendants\n");
	} else {
		/* Unregister from location service */
		rc = loc_service_unregister(fun->service_id);
		if (rc != EOK) {
			log_msg(LVL_ERROR, "Failed unregistering tree service.");
			fibril_rwlock_write_unlock(&tree->rwlock);
			async_answer_0(callid, EIO);
			return;
		}
	}
	
	remove_fun_node(&device_tree, fun);
	fibril_rwlock_write_unlock(&tree->rwlock);
	delete_fun_node(fun);
	
	log_msg(LVL_DEBUG, "devman_remove_function() succeeded.");
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
	log_msg(LVL_DEBUG, "The `%s` driver was successfully initialized.",
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
		log_msg(LVL_ERROR, "Failed to create initialization fibril " \
		    "for driver `%s'.", driver->name);
		return;
	}
	fibril_add_ready(fid);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAN_ADD_FUNCTION:
			devman_add_function(callid, &call);
			break;
		case DEVMAN_ADD_DEVICE_TO_CATEGORY:
			devman_add_function_to_cat(callid, &call);
			break;
		case DEVMAN_REMOVE_FUNCTION:
			devman_remove_function(callid, &call);
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

/** Get device name. */
static void devman_fun_get_name(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	size_t sent_length = str_size(fun->name);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, fun->name, sent_length);
	async_answer_0(iid, EOK);

	free(buffer);
}


/** Get device path. */
static void devman_fun_get_path(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	size_t sent_length = str_size(fun->pathname);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, fun->pathname, sent_length);
	async_answer_0(iid, EOK);

	free(buffer);
}

static void devman_dev_get_functions(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	size_t act_size;
	int rc;
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	dev_node_t *dev = find_dev_node_no_lock(&device_tree,
	    IPC_GET_ARG1(*icall));
	if (dev == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, ENOENT);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	devman_handle_t *hdl_buf = (devman_handle_t *) malloc(size);
	if (hdl_buf == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	rc = dev_get_functions(&device_tree, dev, hdl_buf, size, &act_size);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	sysarg_t retval = async_data_read_finalize(callid, hdl_buf, size);
	free(hdl_buf);
	
	async_answer_1(iid, retval, act_size);
}


/** Get handle for child device of a function. */
static void devman_fun_get_child(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	if (fun->child == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	async_answer_1(iid, EOK, fun->child->handle);
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
}

/** Find handle for the function instance identified by its service ID. */
static void devman_fun_sid_to_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;

	fun = find_loc_tree_function(&device_tree, IPC_GET_ARG1(*icall));
	
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
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAN_DEVICE_GET_HANDLE:
			devman_function_get_handle(callid, &call);
			break;
		case DEVMAN_DEV_GET_FUNCTIONS:
			devman_dev_get_functions(callid, &call);
			break;
		case DEVMAN_FUN_GET_CHILD:
			devman_fun_get_child(callid, &call);
			break;
		case DEVMAN_FUN_GET_NAME:
			devman_fun_get_name(callid, &call);
			break;
		case DEVMAN_FUN_GET_PATH:
			devman_fun_get_path(callid, &call);
			break;
		case DEVMAN_FUN_SID_TO_HANDLE:
			devman_fun_sid_to_handle(callid, &call);
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

	/*
	 * For a valid function to connect to we need a device. The root
	 * function, for example, has no device and cannot be connected to.
	 * This means @c dev needs to be valid regardless whether we are
	 * connecting to a device or to a function.
	 */
	if (dev == NULL) {
		log_msg(LVL_ERROR, "IPC forwarding failed - no device or "
		    "function with handle %" PRIun " was found.", handle);
		async_answer_0(iid, ENOENT);
		return;
	}

	if (fun == NULL && !drv_to_parent) {
		log_msg(LVL_ERROR, NAME ": devman_forward error - cannot "
		    "connect to handle %" PRIun ", refers to a device.",
		    handle);
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
		log_msg(LVL_ERROR, "IPC forwarding refused - " \
		    "the device %" PRIun " is not in usable state.", handle);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	int method;
	if (drv_to_parent)
		method = DRIVER_DRIVER;
	else
		method = DRIVER_CLIENT;
	
	if (!driver->sess) {
		log_msg(LVL_ERROR,
		    "Could not forward to driver `%s'.", driver->name);
		async_answer_0(iid, EINVAL);
		return;
	}

	if (fun != NULL) {
		log_msg(LVL_DEBUG, 
		    "Forwarding request for `%s' function to driver `%s'.",
		    fun->pathname, driver->name);
	} else {
		log_msg(LVL_DEBUG, 
		    "Forwarding request for `%s' device to driver `%s'.",
		    dev->pfun->pathname, driver->name);
	}
	
	async_exch_t *exch = async_exchange_begin(driver->sess);
	async_forward_fast(iid, exch, method, fwd_h, 0, IPC_FF_NONE);
	async_exchange_end(exch);
}

/** Function for handling connections from a client forwarded by the location
 * service to the device manager. */
static void devman_connection_loc(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t service_id = IPC_GET_ARG2(*icall);
	fun_node_t *fun;
	dev_node_t *dev;

	fun = find_loc_tree_function(&device_tree, service_id);
	
	if (fun == NULL || fun->dev->drv == NULL) {
		log_msg(LVL_WARN, "devman_connection_loc(): function "
		    "not found.\n");
		async_answer_0(iid, ENOENT);
		return;
	}
	
	dev = fun->dev;
	
	async_exch_t *exch = async_exchange_begin(dev->drv->sess);
	async_forward_fast(iid, exch, DRIVER_CLIENT, fun->handle, 0,
	    IPC_FF_NONE);
	async_exchange_end(exch);
	
	log_msg(LVL_DEBUG,
	    "Forwarding loc service request for `%s' function to driver `%s'.",
	    fun->pathname, dev->drv->name);
}

/** Function for handling connections to device manager. */
static void devman_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
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
	case DEVMAN_CONNECT_FROM_LOC:
		/* Someone connected through loc node. */
		devman_connection_loc(iid, icall);
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
	log_msg(LVL_DEBUG, "devman_init - looking for available drivers.");
	
	/* Initialize list of available drivers. */
	init_driver_list(&drivers_list);
	if (lookup_available_drivers(&drivers_list,
	    DRIVER_DEFAULT_STORE) == 0) {
		log_msg(LVL_FATAL, "No drivers found.");
		return false;
	}

	log_msg(LVL_DEBUG, "devman_init - list of drivers has been initialized.");

	/* Create root device node. */
	if (!init_device_tree(&device_tree, &drivers_list)) {
		log_msg(LVL_FATAL, "Failed to initialize device tree.");
		return false;
	}

	/*
	 * !!! devman_connection ... as the device manager is not a real loc
	 * driver (it uses a completely different ipc protocol than an ordinary
	 * loc driver) forwarding a connection from client to the devman by
	 * location service would not work.
	 */
	loc_server_register(NAME, devman_connection);
	
	return true;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Manager\n");

	if (log_init(NAME, LVL_WARN) != EOK) {
		printf(NAME ": Error initializing logging subsystem.\n");
		return -1;
	}

	if (!devman_init()) {
		log_msg(LVL_ERROR, "Error while initializing service.");
		return -1;
	}
	
	/* Set a handler of incomming connections. */
	async_set_client_connection(devman_connection);

	/* Register device manager at naming service. */
	if (service_register(SERVICE_DEVMAN) != EOK) {
		log_msg(LVL_ERROR, "Failed registering as a service.");
		return -1;
	}

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* Never reached. */
	return 0;
}

/** @}
 */
